#include "CritHack.h"
#define MASK_SIGNED 0x7FFFFFFF

// i hate crithack

/* Returns whether random crits are enabled on the server */
bool CCritHack::AreRandomCritsEnabled(CBaseCombatWeapon* pWeapon)
{
	if (static auto tf_weapon_criticals = g_ConVars.FindVar("tf_weapon_criticals"); tf_weapon_criticals)
	{
		return tf_weapon_criticals->GetBool();
	}
	return true;
}

/* Returns whether the crithack should run */
bool CCritHack::IsEnabled()
{
	if (!Vars::CritHack::Active.Value) { return false; }
	//if (!AreRandomCritsEnabled()) { return false; }
	if (!I::EngineClient->IsInGame()) { return false; }

	return true;
}

bool CCritHack::IsAttacking(const CUserCmd* pCmd, CBaseCombatWeapon* pWeapon)
{
	if (pWeapon->GetSlot() == SLOT_MELEE)
	{
		if (pWeapon->GetWeaponID() == TF_WEAPON_KNIFE)
			return ((pCmd->buttons & IN_ATTACK) && G::WeaponCanAttack);

		else return fabs(pWeapon->GetSmackTime() - I::GlobalVars->curtime) < I::GlobalVars->interval_per_tick * 2.0f;
	}

	else
	{
		if (G::CurItemDefIndex == Soldier_m_TheBeggarsBazooka)
		{
			static bool bLoading = false;

			if (pWeapon->GetClip1() > 0)
				bLoading = true;

			if (!(pCmd->buttons & IN_ATTACK) && bLoading) {
				bLoading = false;
				return true;
			}
		}

		else
		{
			int ID = pWeapon->GetWeaponID();
			switch (ID) {
			case TF_WEAPON_COMPOUND_BOW:
			case TF_WEAPON_PIPEBOMBLAUNCHER:
			{
				static bool bCharging = false;

				if (pWeapon->GetChargeBeginTime() > 0.0f)
					bCharging = true;

				if (!(pCmd->buttons & IN_ATTACK) && bCharging) {
					bCharging = false;
					return true;
				}
				break;
			}
			case TF_WEAPON_CANNON:
			{
				static bool bCharging = false;

				if (pWeapon->GetDetonateTime() > 0.0f)
					bCharging = true;

				if (!(pCmd->buttons & IN_ATTACK) && bCharging) {
					bCharging = false;
					return true;
				}
				break;
			}
			//ig below you can remove even tho you can crit hack with them..
			case TF_WEAPON_JAR:
			case TF_WEAPON_JAR_MILK:
			case TF_WEAPON_JAR_GAS:
			case TF_WEAPON_GRENADE_JAR_GAS:
			case TF_WEAPON_CLEAVER:
			{
				static float flThrowTime = 0.0f;

				if ((pCmd->buttons & IN_ATTACK) && G::WeaponCanAttack && !flThrowTime)
					flThrowTime = I::GlobalVars->curtime + I::GlobalVars->interval_per_tick;

				if (flThrowTime && I::GlobalVars->curtime >= flThrowTime) {
					flThrowTime = 0.0f;
					return true;
				}
				break;
			}
			default:
			{
				if ((pCmd->buttons & IN_ATTACK) && G::WeaponCanAttack)
				{
					return true;
				}
				break;
			}
			}
		}
	}

	return false;
}

bool CCritHack::NoRandomCrits(CBaseCombatWeapon* pWeapon)
{
	float CritChance = Utils::ATTRIB_HOOK_FLOAT(1, "mult_crit_chance", pWeapon);
	if (CritChance == 0)
	{
		return true;
	}
	//list of weapons that cant random crit, but dont have the attribute for it
	switch (pWeapon->GetWeaponID())
	{
		//scout
	case TF_WEAPON_JAR_MILK:
		//soldier
	case TF_WEAPON_BUFF_ITEM:
		//pyro
	case TF_WEAPON_JAR_GAS:
	case TF_WEAPON_FLAME_BALL:
	case TF_WEAPON_ROCKETPACK:
		//demo
	case TF_WEAPON_PARACHUTE: //also for soldier
		//heavy
	case TF_WEAPON_LUNCHBOX:
		//engineer
	case TF_WEAPON_PDA_ENGINEER_BUILD:
	case TF_WEAPON_PDA_ENGINEER_DESTROY:
	case TF_WEAPON_LASER_POINTER:
		//medic
	case TF_WEAPON_MEDIGUN:
		//sniper
	case TF_WEAPON_SNIPERRIFLE:
	case TF_WEAPON_SNIPERRIFLE_CLASSIC:
	case TF_WEAPON_SNIPERRIFLE_DECAP:
	case TF_WEAPON_COMPOUND_BOW:
	case TF_WEAPON_JAR:
		//spy
	case TF_WEAPON_KNIFE:
	case TF_WEAPON_PDA_SPY_BUILD:
	case TF_WEAPON_PDA_SPY:
		return true;
		break;
	default: return false; break;
	}
}

bool CCritHack::ShouldCrit()
{
	static KeyHelper critKey{ &Vars::CritHack::CritKey.Value };
	if (critKey.Down()) { return true; }
	if (G::CurWeaponType == EWeaponType::MELEE && Vars::CritHack::AlwaysMelee.Value) { return true; }

	return false;
}

int CCritHack::LastGoodCritTick(const CUserCmd* pCmd)
{
	int retVal = -1;
	bool popBack = false;

	for (const auto& tick : CritTicks)
	{
		if (tick >= pCmd->command_number)
		{
			retVal = tick;
		}
		else
		{
			popBack = true;
		}
	}

	if (popBack)
	{
		CritTicks.pop_back();
	}

	return retVal;
}

void CCritHack::ScanForCrits(const CUserCmd* pCmd, int loops)
{
	static int previousWeapon = 0;
	static int previousCrit = 0;
	static int startingNum = pCmd->command_number;

	const auto& pLocal = g_EntityCache.GetLocal();
	if (!pLocal) { return; }

	const auto& pWeapon = pLocal->GetActiveWeapon();
	if (!pWeapon) { return; }

	if (G::IsAttacking || IsAttacking(pCmd, pWeapon)/* || pCmd->buttons & IN_ATTACK*/)
	{
		return;
	}

	const bool bRescanRequired = previousWeapon != pWeapon->GetIndex();
	if (bRescanRequired)
	{
		startingNum = pCmd->command_number;
		previousWeapon = pWeapon->GetIndex();
		CritTicks.clear();
	}

	if (CritTicks.size() >= 256)
	{
		return;
	}

	//CritBucketBP = *reinterpret_cast<float*>(pWeapon + 0xA54);
	ProtectData = true; //	stop shit that interferes with our crit bucket because it will BREAK it
	const int seedBackup = MD5_PseudoRandom(pCmd->command_number) & MASK_SIGNED;
	for (int i = 0; i < loops; i++)
	{
		const int cmdNum = startingNum + i;
		*I::RandomSeed = MD5_PseudoRandom(cmdNum) & MASK_SIGNED;
		if (pWeapon->WillCrit())
		{
			CritTicks.push_back(cmdNum); //	store our wish command number for later reference
		}
	}
	startingNum += loops;
	ProtectData = false; //	we no longer need to be protecting important crit data

	//*reinterpret_cast<float*>(pWeapon + 0xA54) = CritBucketBP;
	*reinterpret_cast<int*>(pWeapon + 0xA5C) = 0; //	dont comment this out, makes sure our crit mult stays as low as possible
	//	crit mult can reach a maximum value of 3!! which means we expend 3 crits WORTH from our bucket
	//	by forcing crit mult to be its minimum value of 1, we can crit more without directly fucking our bucket
	//	yes ProtectData stops this value from changing artificially, but it still changes when you fire and this is worth it imo.

	*I::RandomSeed = seedBackup;
}

void CCritHack::Run(CUserCmd* pCmd)
{
	if (!IsEnabled()) { return; }

	const auto& pWeapon = g_EntityCache.GetWeapon();
	if (!pWeapon || !pWeapon->CanFireCriticalShot(false)) { return; }

	ScanForCrits(pCmd, 50); //	fill our vector slowly.

	const int closestGoodTick = LastGoodCritTick(pCmd); //	retrieve our wish
	if (IsAttacking(pCmd, pWeapon)) //	is it valid & should we even use it
	{
		if (ShouldCrit())
		{
			if (closestGoodTick < 0) { return; }
			pCmd->command_number = closestGoodTick; //	set our cmdnumber to our wish
			pCmd->random_seed = MD5_PseudoRandom(closestGoodTick) & MASK_SIGNED; //	trash poopy whatever who cares
		}
		else if (Vars::CritHack::AvoidRandom.Value) //	we don't want to crit
		{
			for (int tries = 1; tries < 25; tries++)
			{
				if (std::find(CritTicks.begin(), CritTicks.end(), pCmd->command_number + tries) != CritTicks.end())
				{
					continue; //	what a useless attempt
				}
				pCmd->command_number += tries;
				pCmd->random_seed = MD5_PseudoRandom(pCmd->command_number) & MASK_SIGNED;
				break; //	we found a seed that we can use to avoid a crit and have skipped to it, woohoo
			}
		}
	}
}

void CCritHack::Draw()
{
	if (!Vars::CritHack::Indicators.Value) { return; }
	if (!IsEnabled() || !G::CurrentUserCmd) { return; }

	const auto& pLocal = g_EntityCache.GetLocal();
	if (!pLocal || !pLocal->IsAlive()) { return; }

	const auto& pWeapon = pLocal->GetActiveWeapon();
	if (!pWeapon) { return; }

	const int x = Vars::CritHack::IndicatorPos.c;
	int currentY = Vars::CritHack::IndicatorPos.y;

	const float bucket = *reinterpret_cast<float*>(pWeapon + 0xA54);
	const int seedRequests = *reinterpret_cast<int*>(pWeapon + 0xA5C);

	int longestW = 40;

	if (Vars::Debug::DebugInfo.Value)
	{
		g_Draw.String(FONT_INDICATORS, x, currentY += 15, { 255, 255, 255, 255, }, ALIGN_CENTERHORIZONTAL, tfm::format("%x", reinterpret_cast<float*>(pWeapon + 0xA54)).c_str());
	}
	// Are we currently forcing crits?
	if (ShouldCrit() && AreRandomCritsEnabled(pWeapon) && NoRandomCrits(pWeapon) == false)
	{
		if (CritTicks.size() > 0)
		{
			g_Draw.String(FONT_INDICATORS, x, currentY += 15, { 0, 255, 0, 255 }, ALIGN_CENTERHORIZONTAL, "Forcing Crits");
		}
	}
	//Can this weapon do random crits?
	if (NoRandomCrits(pWeapon) == true)
	{
		g_Draw.String(FONT_INDICATORS, x, currentY += 15, { 255, 95, 95, 255 }, ALIGN_CENTERHORIZONTAL, L"No Random Crits");
	}
	if (AreRandomCritsEnabled(pWeapon) == false)
	{
		g_Draw.String(FONT_INDICATORS, x, currentY += 15, { 255, 95, 95, 255 }, ALIGN_CENTERHORIZONTAL, L"Server Disabled Crits");
	}
	//Crit banned check
	if (CritTicks.size() == 0 && AreRandomCritsEnabled(pWeapon) && NoRandomCrits(pWeapon) == false)
	{
		g_Draw.String(FONT_INDICATORS, x, currentY += 15, { 255,0,0,255 }, ALIGN_CENTERHORIZONTAL, L"Crit Banned");
	}
	static auto tf_weapon_criticals_bucket_cap = g_ConVars.FindVar("tf_weapon_criticals_bucket_cap");
	const float bucketCap = tf_weapon_criticals_bucket_cap->GetFloat();
	const std::wstring bucketstr = L"Bucket: " + std::to_wstring(static_cast<int>(bucket)) + L"/" + std::to_wstring(static_cast<int>(bucketCap));
	// crit bucket string (this sucks)
	if (AreRandomCritsEnabled(pWeapon) && NoRandomCrits(pWeapon) == false)
	{
		g_Draw.String(FONT_INDICATORS, x, currentY += 15, { 181, 181, 181, 255 }, ALIGN_CENTERHORIZONTAL, bucketstr.c_str());
	}

	int w, h;
	I::VGuiSurface->GetTextSize(g_Draw.m_vecFonts.at(FONT_INDICATORS).dwFont, bucketstr.c_str(), w, h);
	if (w > longestW)
	{
		longestW = w;
	}
	if (Vars::Debug::DebugInfo.Value)
	{
		const std::wstring seedText = L"m_nCritSeedRequests: " + std::to_wstring(seedRequests);
		const std::wstring FoundCrits = L"Found Crit Ticks: " + std::to_wstring(CritTicks.size());
		const std::wstring commandNumber = L"cmdNumber: " + std::to_wstring(G::CurrentUserCmd->command_number);
		g_Draw.String(FONT_INDICATORS, x, currentY += 15, { 181, 181, 181, 255 }, ALIGN_CENTERHORIZONTAL, seedText.c_str());
		I::VGuiSurface->GetTextSize(g_Draw.m_vecFonts.at(FONT_INDICATORS).dwFont, seedText.c_str(), w, h);
		if (w > longestW)
		{
			longestW = w;
		}
		g_Draw.String(FONT_INDICATORS, x, currentY += 15, { 181, 181, 181, 255 }, ALIGN_CENTERHORIZONTAL, FoundCrits.c_str());
		I::VGuiSurface->GetTextSize(g_Draw.m_vecFonts.at(FONT_INDICATORS).dwFont, FoundCrits.c_str(), w, h);
		if (w > longestW)
		{
			longestW = w;
		}
		g_Draw.String(FONT_INDICATORS, x, currentY += 15, { 181, 181, 181, 255 }, ALIGN_CENTERHORIZONTAL, commandNumber.c_str());
		I::VGuiSurface->GetTextSize(g_Draw.m_vecFonts.at(FONT_INDICATORS).dwFont, commandNumber.c_str(), w, h);
		if (w > longestW)
		{
			longestW = w;
		}
	}
	IndicatorW = longestW * 2;
	IndicatorH = currentY;
}
