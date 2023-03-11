#pragma once
#include "../../SDK/SDK.h"

class CAimbot
{
private:
	bool ShouldntRun(bool yay = false);
	bool ShouldRun(CBaseEntity* pLocal, CBaseCombatWeapon* pWeapon);

public:
	void ReportRoundStatus(CGameEvent* pEvent, FNV1A_t uNameHash);
	void Run(CUserCmd* pCmd);
};

ADD_FEATURE(CAimbot, Aimbot)