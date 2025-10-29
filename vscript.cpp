
#include "vscript.hpp"

#include "scanner.hpp"
#include "globals.hpp"
#include "sdk.hpp"

#include "vscript/ivscript.h"

CBaseEntity* VScript::ToEnt(HSCRIPT ent) {
	if (!ent) {
		return NULL;
	}

	static uintptr_t func = (uintptr_t)Memory::Scanner::Scan<void*>(Memory::Modules::Get("server"), "E8 ?? ?? ?? ?? 8B 4D 18 8B 57 5C", 1);
	static auto GetCBaseEntityScriptDesc = reinterpret_cast<ScriptClassDesc_t * (*)()>(*reinterpret_cast<uintptr_t*>(func) + func + sizeof(func));
	return reinterpret_cast<CBaseEntity*>(g_pScriptVM->GetInstanceValue(ent, GetCBaseEntityScriptDesc()));
}

// nabbed from sdk 2013
static void TraceToScriptVM(HSCRIPT hTable, Vector vStart, Vector vEnd, trace_t& tr)
{
	g_pScriptVM->SetValue(hTable, "fraction", tr.fraction);
	g_pScriptVM->SetValue(hTable, "hit", (tr.fraction != 1.0 || tr.startsolid));
	g_pScriptVM->SetValue(hTable, "plane_normal", tr.plane.normal);
	g_pScriptVM->SetValue(hTable, "plane_dist", tr.plane.dist);
	g_pScriptVM->SetValue(hTable, "contents", tr.contents);
	if (tr.allsolid)
		g_pScriptVM->SetValue(hTable, "allsolid", tr.allsolid);
	if (tr.m_pEnt)
		g_pScriptVM->SetValue(hTable, "enthit", CBaseEntity__GetScriptInstance(tr.m_pEnt));
	if (tr.startsolid)
		g_pScriptVM->SetValue(hTable, "startsolid", tr.startsolid);
	Vector vPos = vStart + (tr.fraction * (vEnd - vStart));
	g_pScriptVM->SetValue(hTable, "pos", vPos);
	g_pScriptVM->SetValue(hTable, "startpos", tr.startpos);
	g_pScriptVM->SetValue(hTable, "endpos", tr.endpos);
	g_pScriptVM->SetValue(hTable, "surface_name", tr.surface.name ? tr.surface.name : "");
	g_pScriptVM->SetValue(hTable, "surface_flags", tr.surface.flags);
	g_pScriptVM->SetValue(hTable, "surface_props", tr.surface.surfaceProps);
}

// Inputs: start, end, mask, ignore  -- outputs: pos, fraction, hit, enthit, startsolid
static bool Script_TraceLineEx(HSCRIPT hTable)
{
	int mask = MASK_VISIBLE_AND_NPCS;
	int coll = COLLISION_GROUP_NONE;
	ScriptVariant_t rval;
	Vector vStart, vEnd;
	CBaseEntity* pIgnoreEnt = NULL;
	bool bNoParams = false;

	if (g_pScriptVM->GetValue(hTable, "start", &rval))
		vStart = rval;
	else
		bNoParams = true;
	if (g_pScriptVM->GetValue(hTable, "end", &rval))
		vEnd = rval;
	else
		bNoParams = true;
	if (g_pScriptVM->GetValue(hTable, "mask", &rval))
		mask = rval;
	if (bNoParams)
	{
		Warning("Didnt supply start and end to Script TraceLine call, failing, setting called to false\n");
		DevMsg("Inputs: start, end, mask, ignore  -- outputs: pos, fraction, hit, enthit, startsolid");
		return false;
	}
	if (g_pScriptVM->GetValue(hTable, "ignore", &rval))
	{
		pIgnoreEnt = VScript::ToEnt((HSCRIPT)rval);
	}
	trace_t tr;
	UTIL_TraceLine(vStart, vEnd, mask, reinterpret_cast<const IHandleEntity*>(pIgnoreEnt), coll, &tr);

	TraceToScriptVM(hTable, vStart, vEnd, tr);

	return true;
}

void VScript::RegisterFuncsAndRun()
{
	g_pScriptVM = **Memory::Scanner::Scan<IScriptVM***>(Memory::Modules::Get("server"), "8B 1D ?? ?? ?? ?? 57 85 DB", 2);
	if (!g_pScriptVM)
	{
		Log(WARNING, false, "Could not run or register our VScript functions!");
		return;
	}

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_TraceLineEx, "P2SM_TraceLineEx", "Pass table - Inputs: start, end, mask, ignore  -- outputs: pos, fraction, hit, enthit, allsolid, startpos, endpos, startsolid, plane_normal, plane_dist, surface_name, surface_flags, surface_props");
}