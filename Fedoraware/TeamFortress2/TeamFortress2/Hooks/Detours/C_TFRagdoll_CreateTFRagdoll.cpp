#include "../Hooks.h"

void ClearEffects(C_TFRagdoll* pRagdoll)
{
	pRagdoll->m_bBurning() = false;
	pRagdoll->m_bElectrocuted() = false;
	pRagdoll->m_bBecomeAsh() = false;
	pRagdoll->m_bDissolving() = false;
	pRagdoll->m_bGoldRagdoll() = false;
	pRagdoll->m_bIceRagdoll() = false;
}

MAKE_HOOK(C_TFRagdoll_CreateTFRagdoll, g_Pattern.Find(L"client.dll", L"55 8B EC B8 ? ? ? ? E8 ? ? ? ? 53 56 57 8B F9 8B 8F ? ? ? ? 85 C9 0F 85"), void, __fastcall,
		  void* ecx, void* edx)
{
	if (Vars::Visuals::RemoveRagdolls.Value) { return; }

	if (const auto& pRagdoll = static_cast<C_TFRagdoll*>(ecx))
	{
		ConVar* tf_playergib = g_ConVars.FindVar("tf_playergib");
		if (Vars::Visuals::RagdollEffects::NoGib.Value)
		{
			tf_playergib->SetValue(0);
		}
	
		if (Vars::Visuals::RagdollEffects::EnemyOnly.Value)
		{
			if (const auto& pLocal = g_EntityCache.GetLocal())
			{
				if (pRagdoll->m_iTeam() == pLocal->GetTeamNum())
				{
					return;
				}
			}
		}

		ClearEffects(pRagdoll);

		if (Vars::Visuals::RagdollEffects::SeparateVectors.Value)
		{
			pRagdoll->m_vecForce().x *= Vars::Visuals::RagdollEffects::RagdollForceForwards.Value;
			pRagdoll->m_vecForce().y *= Vars::Visuals::RagdollEffects::RagdollForceSides.Value;
			pRagdoll->m_vecForce().z *= Vars::Visuals::RagdollEffects::RagdollForceUp.Value;
		}
		else
		{
			pRagdoll->m_vecForce() *= Vars::Visuals::RagdollEffects::RagdollForce.Value;
		}

		pRagdoll->m_bBurning() = Vars::Visuals::RagdollEffects::Burning.Value;
		pRagdoll->m_bElectrocuted() = Vars::Visuals::RagdollEffects::Electrocuted.Value;
		pRagdoll->m_bBecomeAsh() = Vars::Visuals::RagdollEffects::BecomeAsh.Value;
		pRagdoll->m_bDissolving() = Vars::Visuals::RagdollEffects::Dissolve.Value;
		pRagdoll->m_bGoldRagdoll() = Vars::Visuals::RagdollEffects::RagdollType.Value == 1;
		pRagdoll->m_bIceRagdoll() = Vars::Visuals::RagdollEffects::RagdollType.Value == 2;
	}

	Hook.Original<FN>()(ecx, edx);
}
