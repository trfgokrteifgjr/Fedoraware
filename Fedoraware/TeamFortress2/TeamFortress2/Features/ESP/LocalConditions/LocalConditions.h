#pragma once
#include "../../SDK/SDK.h"

class LCESP
{
	std::vector<std::wstring> GetPlayerConditions(CBaseEntity* pEntity) const;
};

ADD_FEATURE(LCESP, LocalConditions)