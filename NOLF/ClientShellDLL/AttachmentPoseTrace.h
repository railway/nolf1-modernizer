// Optional read-only pose measurement through the model SDK.
// +ShotAttachments 1 logs character, node, socket and attached-object transforms at +ShotPoses frames.
// Enable ErrorLog and AlwaysFlushLog to capture these console lines.
#pragma once

namespace AttachmentPoseTrace
{

static void Print(const char* kind, HOBJECT object, const char* name, LTransform& transform)
{
	g_pLTClient->CPrint("AttachmentPose %s %p %s pos %.9g %.9g %.9g rot %.9g %.9g %.9g %.9g",
		kind, object, name, transform.m_Pos.x, transform.m_Pos.y, transform.m_Pos.z,
		transform.m_Rot[0], transform.m_Rot[1], transform.m_Rot[2], transform.m_Rot[3]);
}

static void Object(HOBJECT object, const char* kind, const char* name)
{
	LTransform transform;
	g_pLTClient->GetObjectPos(object, &transform.m_Pos);
	g_pLTClient->GetObjectRotation(object, &transform.m_Rot);
	Print(kind, object, name, transform);
	LTVector scale;
	if (g_pLTClient->GetObjectScale(object, &scale) == LT_OK)
		g_pLTClient->CPrint("AttachmentPose scale %p %.9g %.9g %.9g", object, scale.x, scale.y, scale.z);
	LTAnimTracker* tracker = NULL;
	if (g_pModelLT->GetMainTracker(object, tracker) == LT_OK && tracker)
	{
		HMODELANIM animation = 0;
		uint32 time = 0, length = 0;
		if (g_pModelLT->GetCurAnim(tracker, animation) == LT_OK &&
			g_pModelLT->GetCurAnimTime(tracker, time) == LT_OK &&
			g_pModelLT->GetCurAnimLength(tracker, length) == LT_OK)
			g_pLTClient->CPrint("AttachmentPose animation %p %u %u %u", object, animation, time, length);
	}
}

static void Node(HOBJECT object, HMODELNODE node, const char* kind)
{
	char name[128];
	if (g_pLTClient->GetModelNodeName(object, node, name, sizeof(name)) != LT_OK) return;
	LTransform transform;
	if (g_pModelLT->GetNodeTransform(object, node, transform, LTFALSE) == LT_OK)
		Print(kind, object, name, transform);
	if (g_pModelLT->GetNodeTransform(object, node, transform, LTTRUE) == LT_OK)
		Print("node-world", object, name, transform);
}

static void Nodes(HOBJECT object)
{
	HMODELNODE node = INVALID_MODEL_NODE;
	unsigned count = 0;
	while (count++ < 256 && g_pLTClient->GetNextModelNode(object, node, &node) == LT_OK)
	{
		Node(object, node, "node-local");
	}
}

static void Write(const char* pose, CSpecialFXList* characters)
{
	HCONSOLEVAR enabled = g_pLTClient->GetConsoleVar("ShotAttachments");
	if (!enabled || g_pLTClient->GetVarValueFloat(enabled) <= 0.0f || !characters) return;
	g_pLTClient->CPrint("AttachmentPose begin %s", pose);
	const char* names[] = {"RightHand", "LeftHand", "Head", "Eyes", "Back", "Nose", "Chin",
		"LeftFoot", "RightFoot", "Motorcycle", "Snowmobile", "Mouth"};
	for (int index = 0; index < characters->GetSize(); ++index)
	{
		CCharacterFX* character = static_cast<CCharacterFX*>((*characters)[index]);
		if (!character) continue;
		HOBJECT parent = character->GetServerObj();
		HOBJECT children[32];
		uint32 stored = 0, total = 0;
		if (g_pLTClient->GetAttachments(parent, children, 32, &stored, &total) != LT_OK || !total) continue;
		Object(parent, "parent", g_pModelButeMgr->GetModelName(character->GetModelId()));
		g_pLTClient->CPrint("AttachmentPose children %p %u %u", parent, stored, total);
		Nodes(parent);
		for (unsigned index = 0; index < sizeof(names) / sizeof(names[0]); ++index)
		{
			HMODELSOCKET socket = INVALID_MODEL_SOCKET;
			if (g_pModelLT->GetSocket(parent, const_cast<char*>(names[index]), socket) != LT_OK) continue;
			LTransform transform;
			if (g_pModelLT->GetSocketTransform(parent, socket, transform, LTFALSE) == LT_OK)
				Print("socket-local", parent, names[index], transform);
			if (g_pModelLT->GetSocketTransform(parent, socket, transform, LTTRUE) == LT_OK)
				Print("socket-world", parent, names[index], transform);
		}
		for (uint32 index = 0; index < stored && index < 32; ++index)
		{
			g_pLTClient->CPrint("AttachmentPose child-of %p %p", children[index], parent);
			Object(children[index], "child", "attached");
			Nodes(children[index]);
		}
	}
	g_pLTClient->CPrint("AttachmentPose end %s", pose);
}

}
