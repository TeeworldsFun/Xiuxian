#include <engine/shared/config.h>

#include <game/server/ai.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>

#include "easy_xiesui_ai.h"

#include <thread>

CAIXieSui::CAIXieSui(CGameContext *pGameServer, CPlayer *pPlayer, int Type)
	: CAI(pGameServer, pPlayer)
{
	m_SkipMoveUpdate = 0;
	switch (Type)
	{
	case TYPE_L1:
		str_copy(pPlayer->m_TeeInfos.m_SkinName, "zombie1", sizeof(pPlayer->m_TeeInfos.m_SkinName));
		pPlayer->m_TeeInfos.m_UseCustomColor = 1;
		pPlayer->m_TeeInfos.m_ColorBody = 0;
		pPlayer->m_TeeInfos.m_ColorFeet = 0;
		break;

	case TYPE_L2:
		str_copy(pPlayer->m_TeeInfos.m_SkinName, "zombie1", sizeof(pPlayer->m_TeeInfos.m_SkinName));
		pPlayer->m_TeeInfos.m_UseCustomColor = 1;
		pPlayer->m_TeeInfos.m_ColorBody = 16776960;
		pPlayer->m_TeeInfos.m_ColorFeet = 16776960;
		break;

	case TYPE_L3:
		str_copy(pPlayer->m_TeeInfos.m_SkinName, "zombie2", sizeof(pPlayer->m_TeeInfos.m_SkinName));
		pPlayer->m_TeeInfos.m_UseCustomColor = 1;
		pPlayer->m_TeeInfos.m_ColorBody = 11599616;
		pPlayer->m_TeeInfos.m_ColorFeet = 65280;
		break;

	case TYPE_L4:
		str_copy(pPlayer->m_TeeInfos.m_SkinName, "zombie", sizeof(pPlayer->m_TeeInfos.m_SkinName));
		pPlayer->m_TeeInfos.m_UseCustomColor = 1;
		pPlayer->m_TeeInfos.m_ColorBody = 5308160;
		pPlayer->m_TeeInfos.m_ColorFeet = 12320512;
		break;

	default:
		break;
	}
}

void CAIXieSui::OnCharacterSpawn(CCharacter *pChr)
{
	CAI::OnCharacterSpawn(pChr);

	m_WaypointDir = vec2(0, 0);
}

void CAIXieSui::ThreadBotAction(void *pUser)
{
	CAIXieSui *pThis = (CAIXieSui *)pUser;

	pThis->HeadToMovingDirection();

	pThis->SeekClosestEnemyInSight();

	// if we see a player
	if (pThis->m_EnemiesInSight > 0)
	{
		pThis->ShootAtClosestEnemy();
		pThis->ReactToPlayer();
	}
	else
		pThis->m_AttackTimer = 0;

	// if (SeekClosestEnemy())
	if (pThis->SeekRandomEnemy())
	{
		pThis->m_TargetPos = pThis->m_PlayerPos;

		if (pThis->m_EnemiesInSight > 1)
		{
			// distance to the player
			if (pThis->m_PlayerPos.x < pThis->m_Pos.x)
				pThis->m_TargetPos.x = pThis->m_PlayerPos.x + pThis->WeaponShootRange() / 2 * (0.5f + frandom() * 1.0f);
			else
				pThis->m_TargetPos.x = pThis->m_PlayerPos.x - pThis->WeaponShootRange() / 2 * (0.5f + frandom() * 1.0f);
		}
	}

	if (pThis->UpdateWaypoint())
	{
		pThis->MoveTowardsWaypoint(20);
		pThis->HookMove();
		pThis->AirJump();

		// jump if waypoint is above us
		if (abs(pThis->m_WaypointPos.x - pThis->m_Pos.x) < 60 && pThis->m_WaypointPos.y < pThis->m_Pos.y - 100 && frandom() * 20 < 4)
			pThis->m_Jump = 1;
	}

	if(pThis->Player()->GetCharacter()->Hooking())
		pThis->m_HookTimer++;
	
	if(pThis->m_HookTimer >= 10*50)
	{
		pThis->m_HookTimer = 0;
		pThis->m_Hook = 0;
	}

	pThis->DoJumping();
	pThis->Unstuck();
	pThis->RandomlyStopShooting();
}

void CAIXieSui::DoBehavior()
{
	// reset jump and attack
	m_Jump = 0;
	m_Attack = 0;

	if(Player()->GetCharacter())
		Player()->GetCharacter()->SetWeapon(WEAPON_HAMMER);

	// std::thread thread = std::thread(&ThreadBotAction, this, Player()->GetCharacter());
	// thread.detach();
	
	ThreadBotAction(this);
	// next reaction in
	m_ReactionTime = 2 * frandom();
	HookMove();
}