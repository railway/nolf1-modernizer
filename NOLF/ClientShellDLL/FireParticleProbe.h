// Opt-in measurements of one stationary particle and its actual D3D7 draw inputs.
// No engine implementation is reproduced here; the device location is version-gated.
#ifndef FIRE_PARTICLE_PROBE_H
#define FIRE_PARTICLE_PROBE_H

#include <d3d.h>

static float FireProbeValue(const char* name, float fallback)
{
	HCONSOLEVAR value = g_pLTClient->GetConsoleVar(const_cast<char*>(name));
	return value ? g_pLTClient->GetVarValueFloat(value) : fallback;
}

static HLOCALOBJ s_fireParticleProbe = LTNULL;

static void ResetFireParticleProbe()
{
	s_fireParticleProbe = LTNULL; // The old world's objects have already gone.
}

static HLOCALOBJ PrepareFireParticleProbe()
{
	HCONSOLEVAR option = g_pLTClient->GetConsoleVar("FireProbeTexture");
	char* texture = option ? g_pLTClient->GetVarValueString(option) : LTNULL;
	if (!texture || !texture[0]) return LTNULL;
	HLOCALOBJ& object = s_fireParticleProbe;
	uint32 width, height;
	g_pLTClient->GetSurfaceDims(g_pLTClient->GetScreenSurface(), &width, &height);
	if (!width) return LTNULL;
	float radius = FireProbeValue("FireProbeRadius", 4000.0f) / width;
	if (!object)
	{
		ObjectCreateStruct create;
		INIT_OBJECTCREATESTRUCT(create);
		create.m_ObjectType = OT_PARTICLESYSTEM;
		create.m_Flags = FLAG_VISIBLE | FLAG_UPDATEUNSEEN | FLAG_FOGDISABLE | FLAG_NOGLOBALLIGHTSCALE;
		create.m_Pos.Init(FireProbeValue("FireProbeX", 0), FireProbeValue("FireProbeY", 0), FireProbeValue("FireProbeZ", 0));
		object = g_pLTClient->CreateObject(&create);
		if (!object) return LTNULL;
		g_pLTClient->SetupParticleSystem(object, texture, 0, 0, radius);
		uint32 flags;
		g_pLTClient->Common()->GetObjectFlags(object, OFT_Flags2, flags);
		g_pLTClient->Common()->SetObjectFlags(object, OFT_Flags2, flags | FLAG2_ADDITIVE);
	}
	LTParticle *particle, *tail;
	if (g_pLTClient->GetParticles(object, &particle, &tail))
	{
		while (particle && particle != tail)
		{
			LTParticle* next = particle->m_pNext;
			g_pLTClient->RemoveParticle(object, particle);
			particle = next;
		}
	}
	LTVector zero(0, 0, 0), color;
	color.Init(FireProbeValue("FireProbeTint", 100), FireProbeValue("FireProbeTint", 100), FireProbeValue("FireProbeTint", 100));
	particle = g_pLTClient->AddParticle(object, &zero, &zero, &color, 1.0f);
	if (particle) { particle->m_Size = radius * FireProbeValue("FireProbeScale", 1); particle->m_Alpha = 1; }
	return object;
}

typedef HRESULT (STDMETHODCALLTYPE *FireDrawPrimitiveFn)(IDirect3DDevice7*, D3DPRIMITIVETYPE, DWORD, void*, DWORD, DWORD);
typedef HRESULT (STDMETHODCALLTYPE *FireDrawIndexedFn)(IDirect3DDevice7*, D3DPRIMITIVETYPE, DWORD, void*, DWORD, WORD*, DWORD, DWORD);
static FireDrawPrimitiveFn s_fireDrawPrimitive;
static FireDrawIndexedFn s_fireDrawIndexed;
static const char* s_fireDrawShot;
static void** s_fireOriginalVtable;
static void* s_fireProbeVtable[49];

static void TraceFireDraw(IDirect3DDevice7* device, DWORD fvf, void* vertices, DWORD count)
{
	if ((fvf & D3DFVF_POSITION_MASK) != D3DFVF_XYZRHW || (fvf & D3DFVF_TEXCOUNT_MASK) != D3DFVF_TEX1 || count > 1024) return;
	DWORD source = 0, destination = 0, minFilter = 0, magFilter = 0, mipFilter = 0;
	device->GetRenderState(D3DRENDERSTATE_SRCBLEND, &source);
	device->GetRenderState(D3DRENDERSTATE_DESTBLEND, &destination);
	if (source != D3DBLEND_ONE || destination != D3DBLEND_ONE) return;
	device->GetTextureStageState(0, D3DTSS_MINFILTER, &minFilter);
	device->GetTextureStageState(0, D3DTSS_MAGFILTER, &magFilter);
	device->GetTextureStageState(0, D3DTSS_MIPFILTER, &mipFilter);
	IDirectDrawSurface7* texture = LTNULL;
	device->GetTexture(0, &texture);
	DDSURFACEDESC2 desc = {};
	desc.dwSize = sizeof(desc);
	if (texture) texture->GetSurfaceDesc(&desc);
	printf("{\"kind\":\"fire-draw\",\"shot\":\"%s\",\"game\":%.7f,\"fvf\":%u,\"texture_id\":\"%p\",\"dimensions\":[%u,%u],"
		"\"bits\":%u,\"fourcc\":%u,\"masks\":[%u,%u,%u,%u],\"filters\":[%u,%u,%u],\"blend\":[%u,%u],\"vertices\":[",
		s_fireDrawShot, g_pLTClient->GetGameTime(), fvf, texture, desc.dwWidth, desc.dwHeight,
		desc.ddpfPixelFormat.dwRGBBitCount, desc.ddpfPixelFormat.dwFourCC, desc.ddpfPixelFormat.dwRBitMask, desc.ddpfPixelFormat.dwGBitMask,
		desc.ddpfPixelFormat.dwBBitMask, desc.ddpfPixelFormat.dwRGBAlphaBitMask, minFilter, magFilter, mipFilter, source, destination);
	DWORD stride = 16 + ((fvf & D3DFVF_DIFFUSE) ? 4 : 0) + ((fvf & D3DFVF_SPECULAR) ? 4 : 0) + 8;
	for (DWORD i = 0; i < count; ++i)
	{
		BYTE* data = static_cast<BYTE*>(vertices) + i * stride;
		float* point = reinterpret_cast<float*>(data);
		DWORD diffuse = (fvf & D3DFVF_DIFFUSE) ? *reinterpret_cast<DWORD*>(data + 16) : 0xffffffff;
		float* uv = reinterpret_cast<float*>(data + stride - 8);
		printf("%s[%.7f,%.7f,%.7f,%.7f,%u,%.7f,%.7f]", i ? "," : "", point[0], point[1], point[2], point[3], diffuse, uv[0], uv[1]);
	}
	fputs("]", stdout);
	// One-particle measurements retain the actual loaded top-level texels. This
	// can stall the GPU, so it is never used for population or frame-time claims.
	if (texture && FireProbeValue("FireProbePixels", 0) != 0 && desc.dwWidth <= 256 && desc.dwHeight <= 256)
	{
		DDSURFACEDESC2 locked = {};
		locked.dwSize = sizeof(locked);
		HRESULT result = texture->Lock(LTNULL, &locked, DDLOCK_READONLY | DDLOCK_WAIT, LTNULL);
		printf(",\"lock_result\":%ld", result);
		if (SUCCEEDED(result))
		{
			fputs(",\"pixels\":\"", stdout);
			DWORD rowBytes = locked.dwWidth * locked.ddpfPixelFormat.dwRGBBitCount / 8;
			for (DWORD y = 0; y < locked.dwHeight; ++y)
			{
				BYTE* row = static_cast<BYTE*>(locked.lpSurface) + y * locked.lPitch;
				for (DWORD x = 0; x < rowBytes; ++x) printf("%02x", row[x]);
			}
			fputs("\"", stdout);
			texture->Unlock(LTNULL);
		}
	}
	fputs("}\n", stdout);
	fflush(stdout);
	if (texture) texture->Release();
}

static HRESULT STDMETHODCALLTYPE FireDrawPrimitive(IDirect3DDevice7* device, D3DPRIMITIVETYPE type, DWORD fvf, void* vertices, DWORD count, DWORD flags)
{
	TraceFireDraw(device, fvf, vertices, count);
	return s_fireDrawPrimitive(device, type, fvf, vertices, count, flags);
}

static HRESULT STDMETHODCALLTYPE FireDrawIndexed(IDirect3DDevice7* device, D3DPRIMITIVETYPE type, DWORD fvf, void* vertices, DWORD count, WORD* indices, DWORD indexCount, DWORD flags)
{
	TraceFireDraw(device, fvf, vertices, count);
	return s_fireDrawIndexed(device, type, fvf, vertices, count, indices, indexCount, flags);
}

static IDirect3DDevice7* BeginFireDrawTrace(const char* shot)
{
	if (!shot[0] || FireProbeValue("FireDrawTrace", 0) == 0) return LTNULL;
	BYTE* module = reinterpret_cast<BYTE*>(GetModuleHandleA("d3d.ren"));
	if (!module) return LTNULL;
	IMAGE_DOS_HEADER* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(module);
	IMAGE_NT_HEADERS* pe = reinterpret_cast<IMAGE_NT_HEADERS*>(module + dos->e_lfanew);
	// The retail renderer's device is read at RVA 0x8d988. Check both the PE
	// identity and the relocated reference from its particle draw path first.
	if (pe->FileHeader.TimeDateStamp != 0x3ae73306 || pe->OptionalHeader.SizeOfImage != 0xb5000 ||
		module[0xe20c] != 0x8b || module[0xe20d] != 0x15 ||
		*reinterpret_cast<DWORD*>(module + 0xe20e) != reinterpret_cast<DWORD>(module + 0x8d988))
	{
		SDL_Log("FireDrawTrace: unsupported renderer; no device access");
		return LTNULL;
	}
	IDirect3DDevice7* device = *reinterpret_cast<IDirect3DDevice7**>(module + 0x8d988);
	if (!device) return LTNULL;
	void*** address = reinterpret_cast<void***>(device);
	s_fireOriginalVtable = *address;
	memcpy(s_fireProbeVtable, s_fireOriginalVtable, sizeof(s_fireProbeVtable));
	s_fireDrawPrimitive = reinterpret_cast<FireDrawPrimitiveFn>(s_fireProbeVtable[25]);
	s_fireDrawIndexed = reinterpret_cast<FireDrawIndexedFn>(s_fireProbeVtable[26]);
	s_fireProbeVtable[25] = reinterpret_cast<void*>(FireDrawPrimitive);
	s_fireProbeVtable[26] = reinterpret_cast<void*>(FireDrawIndexed);
	s_fireDrawShot = shot;
	*address = s_fireProbeVtable;
	return device;
}

static void EndFireDrawTrace(IDirect3DDevice7* device)
{
	if (device) *reinterpret_cast<void***>(device) = s_fireOriginalVtable;
}

#endif
