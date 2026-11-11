// Optional pose-frame measurement of world-model collection and translucent draws.
// Enable with +WorldModelTrace 1 together with +ShotPoses. Does not change draw decisions.
#pragma once

#include <detours.h>
#include <intrin.h>
#include <stdint.h>

namespace WorldModelTrace
{

typedef void (__cdecl *CollectFn)(void*);
typedef void (__cdecl *DrawFn)(void*, void*);
static CollectFn s_Collect = NULL;
static DrawFn s_Draw = NULL;
static unsigned char* s_Module = NULL;
static bool s_Attempted = false;
static bool s_Active = false;

// This layout is specific to the GOTY D3D renderer, file FNV-1a d4780064 (638976 bytes).
// RVAs identify the world-model collector, translucent draw wrapper, frame code and list.
// These are observation points, not replacement renderer code.
static const unsigned COLLECT_RVA = 0x64b70;
static const unsigned DRAW_RVA = 0x68910;
static const unsigned FRAME_RVA = 0x8bea4;
static const unsigned LIST_COUNT_RVA = 0x91c18;

struct Event
{
	char kind;
	void* object;
	unsigned caller, frame, stamp, flags, before, after;
	LTVector position;
};
static Event s_Events[8192];
static unsigned s_Count = 0;
static unsigned s_Dropped = 0;

static unsigned ReadWord(const void* address, unsigned offset)
{
	return *reinterpret_cast<const unsigned*>(static_cast<const unsigned char*>(address) + offset);
}

static Event* Record(char kind, void* object, void* caller)
{
	if (!s_Active) return NULL;
	if (s_Count == sizeof(s_Events) / sizeof(s_Events[0]))
	{
		++s_Dropped;
		return NULL;
	}
	Event* event = &s_Events[s_Count++];
	event->kind = kind;
	event->object = object;
	event->caller = static_cast<unsigned>(reinterpret_cast<uintptr_t>(caller) - reinterpret_cast<uintptr_t>(s_Module));
	event->frame = ReadWord(s_Module, FRAME_RVA);
	event->stamp = ReadWord(object, 0x58);
	event->flags = ReadWord(object, 0x88);
	event->before = event->after = ReadWord(s_Module, LIST_COUNT_RVA);
	g_pLTClient->GetObjectPos(static_cast<HLOCALOBJ>(object), &event->position);
	return event;
}

static void __cdecl Collect(void* object)
{
	Event* event = Record('C', object, _ReturnAddress());
	s_Collect(object);
	if (event) event->after = ReadWord(s_Module, LIST_COUNT_RVA);
}

static void __cdecl Draw(void* state, void* object)
{
	Record('D', object, _ReturnAddress());
	s_Draw(state, object);
}

static bool SupportedRenderer(HMODULE module)
{
	char path[MAX_PATH];
	DWORD length = GetModuleFileNameA(module, path, sizeof(path));
	if (!length || length >= sizeof(path)) return false;
	FILE* file = fopen(path, "rb");
	if (!file) return false;
	unsigned hash = 2166136261u, size = 0;
	unsigned char bytes[4096];
	size_t count;
	while ((count = fread(bytes, 1, sizeof(bytes), file)) != 0)
	{
		for (size_t i = 0; i < count; ++i) hash = (hash ^ bytes[i]) * 16777619u;
		size += static_cast<unsigned>(count);
	}
	bool good = !ferror(file) && size == 638976 && hash == 0xd4780064u;
	fclose(file);
	return good;
}

static bool Install()
{
	if (s_Collect) return true;
	if (s_Attempted) return false;
	HMODULE module = GetModuleHandleA("d3d.ren");
	if (!module) return false;
	s_Attempted = true;
	if (!SupportedRenderer(module))
	{
		g_pLTClient->CPrint("WMTRACE unsupported renderer; no hooks installed");
		return false;
	}
	s_Module = reinterpret_cast<unsigned char*>(module);
	s_Collect = reinterpret_cast<CollectFn>(s_Module + COLLECT_RVA);
	s_Draw = reinterpret_cast<DrawFn>(s_Module + DRAW_RVA);
	LONG result = DetourTransactionBegin();
	if (result == NO_ERROR)
	{
		result = DetourUpdateThread(GetCurrentThread());
		if (result == NO_ERROR) result = DetourAttach(reinterpret_cast<PVOID*>(&s_Collect), Collect);
		if (result == NO_ERROR) result = DetourAttach(reinterpret_cast<PVOID*>(&s_Draw), Draw);
		if (result == NO_ERROR) result = DetourTransactionCommit();
		else DetourTransactionAbort();
	}
	if (result != NO_ERROR)
	{
		s_Collect = NULL;
		s_Draw = NULL;
		g_pLTClient->CPrint("WMTRACE install failed %ld; no hooks installed", result);
		return false;
	}
	g_pLTClient->CPrint("WMTRACE installed base=%p collector=%x draw=%x", module, COLLECT_RVA, DRAW_RVA);
	return true;
}

static void Begin(const char* shot)
{
	s_Active = false;
	if (!shot || !shot[0]) return;
	HCONSOLEVAR enabled = g_pLTClient->GetConsoleVar("WorldModelTrace");
	if (!enabled || g_pLTClient->GetVarValueFloat(enabled) <= 0.0f || !Install()) return;
	s_Count = s_Dropped = 0;
	g_pLTClient->CPrint("WMTRACE BEGIN %s", shot);
	s_Active = true;
}

static void End()
{
	if (!s_Active) return;
	s_Active = false;
	for (unsigned i = 0; i < s_Count; ++i)
	{
		const Event& event = s_Events[i];
		g_pLTClient->CPrint("WMTRACE %c obj=%p caller=%x frame=%u stamp=%u flags=%x list=%u:%u pos=%.3f,%.3f,%.3f",
			event.kind, event.object, event.caller, event.frame, event.stamp, event.flags,
			event.before, event.after, event.position.x, event.position.y, event.position.z);
	}
	g_pLTClient->CPrint("WMTRACE END events=%u dropped=%u", s_Count, s_Dropped);
}

static void Shutdown()
{
	s_Active = false;
	if (!s_Collect || GetModuleHandleA("d3d.ren") != reinterpret_cast<HMODULE>(s_Module)) return;
	LONG result = DetourTransactionBegin();
	if (result == NO_ERROR)
	{
		result = DetourUpdateThread(GetCurrentThread());
		if (result == NO_ERROR) result = DetourDetach(reinterpret_cast<PVOID*>(&s_Collect), Collect);
		if (result == NO_ERROR) result = DetourDetach(reinterpret_cast<PVOID*>(&s_Draw), Draw);
		if (result == NO_ERROR) result = DetourTransactionCommit();
		else DetourTransactionAbort();
	}
	if (result != NO_ERROR) g_pLTClient->CPrint("WMTRACE detach failed %ld", result);
	else s_Collect = NULL;
}

}
