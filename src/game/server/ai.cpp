#include <engine/shared/config.h>

#include "ai.h"
#include "entities/character.h"
#include "entities/flag.h"
#include <game/server/player.h>
#include "gamecontext.h"

#include <thread>

CAI::CAI(CGameContext *pGameServer, CPlayer *pPlayer)
{
	m_pGameServer = pGameServer;
	m_pPlayer = pPlayer;
	m_pTargetPlayer = 0;

	m_pPath = 0;
	m_pVisible = 0;

	Reset();
}

void CAI::Reset()
{
	if (m_pPath)
		delete m_pPath;

	m_WaypointUpdateNeeded = true;
	m_WayPointUpdateTick = 0;
	m_WayVisibleUpdateTick = 0;

	m_Sleep = 0;
	m_Stun = 0;
	m_ReactionTime = 20;
	m_NextReaction = m_ReactionTime;
	m_InputChanged = true;
	m_Move = 0;
	m_LastMove = 0;
	m_Jump = 0;
	m_LastJump = 0;
	m_LastAttack = 0;
	m_Hook = 0;
	m_LastHook = 0;
	m_LastPos = vec2(-9000, -9000);
	m_Direction = vec2(-1, 0);
	m_DisplayDirection = vec2(-1, 0);

	m_HookTick = 0;
	m_HookReleaseTick = 0;

	m_UnstuckCount = 0;

	m_WayPointUpdateWait = 0;
	m_WayFound = false;

	m_TargetTimer = 99;
	m_AttackTimer = 0;
	m_HookTimer = 0;
	m_HookReleaseTimer = 0;

	m_pTargetPlayer = 0;
	m_PlayerPos = vec2(0, 0);
	m_TargetPos = vec2(0, 0);
	m_PlayerSpotTimer = 0;
	m_PlayerSpotCount = 0;

	m_TurnSpeed = 0.2f;

	m_DontMoveTick = 0;

	m_OldTargetPos = vec2(0, 0);
	ClearEmotions();

	m_ThreadDone = true;
}

void CAI::OnCharacterSpawn(class CCharacter *pChr)
{
	// pChr->SetCustomWeapon(HAMMER_BASIC);
	Reset();
	m_WaypointPos = pChr->m_Pos;
}

void CAI::Zzz(int Time)
{
	if (!Player()->GetCharacter())
		return;

	if (m_Sleep < Time)
	{
		m_Sleep = Time;
		Player()->GetCharacter()->SetEmoteFor(EMOTE_HAPPY, Time * 17, Time * 17, true);
		GameServer()->SendEmoticon(Player()->GetCID(), EMOTICON_ZZZ);
	}
}

void CAI::Stun(int Time)
{
	if (!Player()->GetCharacter())
		return;

	if (m_Stun < Time)
	{
		m_Stun = Time;
		Player()->GetCharacter()->SetEmoteFor(EMOTE_SURPRISE, Time * 17, Time * 17, true);
	}
}

void CAI::Panic()
{
	Player()->GetCharacter()->SetEmoteFor(EMOTE_PAIN, 2, 2, false);
	m_DontMoveTick = GameServer()->Server()->Tick() + GameServer()->Server()->TickSpeed() * 1;

	if (frandom() * 10 < 7)
		m_Attack = 1;
	else
		m_Attack = 0;
}

void CAI::StandStill(int Time)
{
	m_Sleep = Time;
}

void CAI::UpdateInput(int *Data)
{
	m_InputChanged = false;
	Data[0] = m_Move;
	Data[1] = m_DisplayDirection.x;
	Data[2] = m_DisplayDirection.y;
	// Data[1] = m_Direction.x; Data[2] = m_Direction.y;

	Data[3] = m_Jump;
	Data[4] = m_Attack;
	Data[5] = m_Hook;
}

void CAI::AirJump()
{
	if (Player()->GetCharacter()->GetVel().y > 0 && m_WaypointPos.y + 80 < m_Pos.y)
	{
		if (!GameServer()->Collision()->FastIntersectLine(m_Pos, m_Pos + vec2(0, 120)) && frandom() * 10 < 5)
			m_Jump = 1;
	}
}

void CAI::DoJumping()
{
	if (abs(m_Pos.x - m_WaypointPos.x) < 20 && m_Pos.y > m_WaypointPos.y + 30)
		m_Jump = 1;

	if (Player()->GetCharacter()->IsGrounded() &&
		(GameServer()->Collision()->IsTileSolid(m_Pos.x + m_Move * 32, m_Pos.y) || GameServer()->Collision()->IsTileSolid(m_Pos.x + m_Move * 96, m_Pos.y)))
		m_Jump = 1;
}

bool CAI::UpdateWaypoint()
{
	if (m_WayPointUpdateTick + GameServer()->Server()->TickSpeed() * 5 < GameServer()->Server()->Tick())
		m_WaypointUpdateNeeded = true;

	// if (distance(m_WaypointPos, m_LastPos) < 100) // || m_TargetTimer++ > 30)// && m_WayPointUpdateWait > 10)
	if (m_TargetTimer++ > 40 && (!m_pVisible || m_WaypointUpdateNeeded))
	{
		m_TargetTimer = 0;

		m_WayFound = false;

		// old inefficient path finding
		// prepare waypoints for path finding
		// GameServer()->Collision()->SetWaypointCenter(m_Pos);
		// if (GameServer()->Collision()->FindWaypointPath(m_TargetPos))

		if (GameServer()->Collision()->AStar(m_Pos, m_TargetPos))
		{
			if (m_pPath)
			{
				delete m_pPath;
				m_pVisible = 0;
			}
			m_pPath = GameServer()->Collision()->GetPath();
			GameServer()->Collision()->ForgetAboutThePath();

			if (!m_pPath)
				return false;

			m_pVisible = m_pPath->GetVisible(GameServer(), m_Pos - vec2(0, 16));

			m_WayPointUpdateWait = 0;
			m_WayFound = true;

			m_WaypointPos = m_pPath->m_Pos;
			m_WaypointDir = m_WaypointPos - m_Pos;

			// char aBuf[128]; str_format(aBuf, sizeof(aBuf), "Path found, lenght: %d", m_pPath->Length());
			// GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "path", aBuf);

			vec2 LastPos = m_Pos;
			CWaypointPath *Next = m_pPath;

			m_WaypointUpdateNeeded = false;
			m_WayPointUpdateTick = GameServer()->Server()->Tick();
		}
	}

	m_WayPointUpdateWait++;

	if (m_pVisible)
	{
		m_WaypointPos = m_pVisible->m_Pos;
		m_WaypointDir = m_WaypointPos - m_Pos;

		m_pVisible = m_pVisible->GetVisible(GameServer(), m_Pos - vec2(0, 16));

		m_WayVisibleUpdateTick = GameServer()->Server()->Tick();

		// if (GameServer()->m_ShowWaypoints)
		//	new CStaticlaser(&GameServer()->m_World, m_Pos, m_WaypointPos, 10);
	}

	// left or right
	if (abs(m_WaypointDir.y) * 2 < abs(m_WaypointDir.x))
	{
		if (Player()->GetCharacter()->IsGrounded() && abs(m_WaypointDir.x) > 128)
		{
			int Dir = m_WaypointDir.x < 0 ? -1 : 1;

			// simple pits
			if (!GameServer()->Collision()->IsTileSolid(m_Pos.x + Dir * 32, m_Pos.y + 16) ||
				!GameServer()->Collision()->IsTileSolid(m_Pos.x + Dir * 64, m_Pos.y + 16))
			{
				if (GameServer()->Collision()->IsTileSolid(m_Pos.x + Dir * 128, m_Pos.y + 16) ||
					GameServer()->Collision()->IsTileSolid(m_Pos.x + Dir * 196, m_Pos.y + 16))
					m_Jump = 1;
			}
		}
	}

	// check target
	if (distance(m_Pos, m_TargetPos) < 800)
	{
		if (!GameServer()->Collision()->FastIntersectLine(m_Pos, m_TargetPos))
		{
			m_WaypointPos = m_TargetPos;
			m_WaypointDir = m_WaypointPos - m_Pos;
		}
	}

	if (!m_WayFound && !m_pVisible)
	{
		return false;
		// m_WaypointPos = m_TargetPos;
		// m_WaypointDir = m_WaypointPos - m_Pos;
	}

	return true;
}

void CAI::HookMove()
{
	// release hook if hooking an ally
	if (Player()->GetCharacter()->HookedPlayer() >= 0 && m_Hook == 1)
	{
		CPlayer *pPlayer = GameServer()->m_apPlayers[Player()->GetCharacter()->HookedPlayer()];
		if (pPlayer && pPlayer->GetTeam() == Player()->GetTeam())
			m_Hook = 0;
	}

	// ...or not hooking at all
	if (!Player()->GetCharacter()->Hooking())
	{
		if (m_Hook == 1)
			m_Hook = 0;
	}

	// hook
	if (m_LastHook == 0)
	{
		vec2 HookDir = m_WaypointPos - m_Pos;
		vec2 HookPos;

		bool TryHooking = false;

		float Distance = distance(m_Pos, m_WaypointPos);

		for (int i = 0; i < 6; i++)
		{
			HookPos = m_Pos;

			vec2 Random = vec2(frandom() - frandom(), frandom() - frandom()) * (Distance / 2.0f);

			int C = GameServer()->Collision()->IntersectLine(m_Pos, m_WaypointPos + Random, &HookPos, NULL);
			if (C & CCollision::COLFLAG_SOLID && !(C & CCollision::COLFLAG_NOHOOK) && m_LastHook == 0)
			{
				float Dist = distance(m_Pos, HookPos);
				if (abs(Dist - 230) < 170)
				{
					TryHooking = true;
					break;
				}
			}
		}

		if (TryHooking)
		{

			if (m_Pos.y < m_WaypointPos.y)
				TryHooking = false;

			if (m_Pos.y < m_WaypointPos.y - 100)
			{
				if (m_pVisible && m_pVisible->m_pNext && m_pVisible->m_pNext->m_Pos.y > m_Pos.y)
					TryHooking = false;
			}

			if (TryHooking)
			{
				// if (abs(atan2(m_Direction.x, m_Direction.y) - atan2(m_DisplayDirection.x, m_DisplayDirection.y)) < PI / 6.0f &&
				if (m_DisplayDirection.y < 0)
				{
					m_Hook = 1;
					m_HookTick = GameServer()->Server()->Tick();

					m_Direction = HookPos - m_Pos;
				}
				else if (m_PlayerSpotCount == 0)
					m_Direction = HookPos - m_Pos;
			}
		}
		else
			m_Hook = 0;
	}

	vec2 Vel = Player()->GetCharacter()->GetVel();
	vec2 HookPos = Player()->GetCharacter()->GetCore().m_HookPos;

	// lock move direction
	// if (m_Hook != 0 && Player()->GetCharacter()->Hooking()) // && m_HookMoveLock)
	if (m_Hook != 0) // && m_HookMoveLock)
	{

		// check hook point's surroundings
		for (int y = -2; y < 2; y++)
		{
			if (GameServer()->Collision()->IsTileSolid(HookPos.x + 32, HookPos.y + y * 32) &&
				GameServer()->Collision()->IsTileSolid(HookPos.x + 64, HookPos.y + y * 32) &&
				GameServer()->Collision()->IsTileSolid(HookPos.x + 96, HookPos.y + y * 32))
				m_Move = HookPos.y < m_Pos.y ? -1 : 1;
			if (GameServer()->Collision()->IsTileSolid(HookPos.x - 32, HookPos.y + y * 32) &&
				GameServer()->Collision()->IsTileSolid(HookPos.x - 64, HookPos.y + y * 32) &&
				GameServer()->Collision()->IsTileSolid(HookPos.x - 96, HookPos.y + y * 32))
				m_Move = HookPos.y < m_Pos.y ? 1 : -1;
		}
	}

	// check if we're next to a wall mid air
	if (!GameServer()->Collision()->IsTileSolid(m_Pos.x, m_Pos.y + 32) &&
		Vel.y > -5.0f)
	{
		/*
		if (GameServer()->Collision()->IsTileSolid(m_Pos.x + 48, m_Pos.y - 32) &&
			GameServer()->Collision()->IsTileSolid(m_Pos.x + 48, m_Pos.y + 0 ) &&
			GameServer()->Collision()->IsTileSolid(m_Pos.x + 48, m_Pos.y + 32))
			m_Move = -1;

		if (GameServer()->Collision()->IsTileSolid(m_Pos.x - 48, m_Pos.y - 32) &&
			GameServer()->Collision()->IsTileSolid(m_Pos.x - 48, m_Pos.y + 0 ) &&
			GameServer()->Collision()->IsTileSolid(m_Pos.x - 48, m_Pos.y + 32))
			m_Move = 1;
			*/

		if (GameServer()->Collision()->IsTileSolid(m_Pos.x + 48, m_Pos.y - 16) &&
			GameServer()->Collision()->IsTileSolid(m_Pos.x + 48, m_Pos.y + 16))
			m_Move = -1;

		if (GameServer()->Collision()->IsTileSolid(m_Pos.x - 48, m_Pos.y - 16) &&
			GameServer()->Collision()->IsTileSolid(m_Pos.x - 48, m_Pos.y + 16))
			m_Move = 1;
	}

	// release hook
	if (m_Hook == 1 && Player()->GetCharacter()->Hooking())
	{
		if ((distance(m_Pos, HookPos) < 50 && Vel.y > 0) || m_Pos.y < HookPos.y ||
			m_HookTick + GameServer()->Server()->TickSpeed() * 1 < GameServer()->Server()->Tick())
		{
			m_Hook = 0;
			m_HookReleaseTick = GameServer()->Server()->Tick();
		}
	}
}

void CAI::JumpIfPlayerIsAbove()
{
	if (abs(m_PlayerPos.x - m_Pos.x) < 100 && m_Pos.y > m_PlayerPos.y + 100)
	{
		if (frandom() * 10 < 4)
			m_Jump = 1;
	}
}

void CAI::HeadToMovingDirection()
{
	// if (m_Move != 0)
	//	m_Direction = vec2(m_Move, 0);
}

void CAI::Unstuck()
{
	if (abs(m_Pos.x - m_StuckPos.x) < 10)
	{
		if (++m_UnstuckCount > 10)
		{
			if (frandom() * 10 < 5)
				m_Move = -1;
			else
				m_Move = 1;

			/*
			if (frandom() * 10 < 4)
				m_Jump = 1;
			*/
		}

		if (m_UnstuckCount > 4)
		{
			if (frandom() * 10 < 4)
				m_Jump = 1;
		}
	}
	else
	{
		m_UnstuckCount = 0;
		m_StuckPos = m_Pos;
	}

	// death tile check
	if (Player()->GetCharacter()->GetVel().y > 0 && GameServer()->Collision()->GetCollisionAt(m_Pos.x, m_Pos.y + 32) & CCollision::COLFLAG_DEATH)
	{
		m_Jump = 1;
	}
}

bool CAI::SeekBombArea()
{
	return false;
}

bool CAI::SeekBomb()
{
	return false;
}

bool CAI::MoveTowardsPlayer(int Dist)
{
	if (abs(m_LastPos.x - m_PlayerPos.x) < Dist)
	{
		m_Move = 0;
		return true;
	}

	if (m_LastPos.x < m_PlayerPos.x)
		m_Move = 1;

	if (m_LastPos.x > m_PlayerPos.x)
		m_Move = -1;

	return false;
}

bool CAI::MoveTowardsTarget(int Dist)
{

	if (abs(m_LastPos.x - m_TargetPos.x) < Dist)
	{
		m_Move = 0;
		return true;
	}

	if (m_LastPos.x < m_TargetPos.x)
		m_Move = 1;

	if (m_LastPos.x > m_TargetPos.x)
		m_Move = -1;

	return false;
}

bool CAI::MoveTowardsWaypoint(int Dist)
{

	if (distance(m_LastPos, m_WaypointPos) < Dist)
	{
		m_Move = 0;
		return true;
	}

	if (m_LastPos.x < m_WaypointPos.x)
		m_Move = 1;

	if (m_LastPos.x > m_WaypointPos.x)
		m_Move = -1;

	return false;
}

void CAI::ReceiveDamage(int CID, int Dmg)
{
	if (CID >= 0 && CID < 16)
	{
		m_aAnger[CID] += Dmg;
		m_aAnger[CID] *= 1.1f;

		m_aAttachment[CID] *= 0.9f;

		if (frandom() * 25 < 2 && m_EnemiesInSight > 1)
			m_PanicTick = GameServer()->Server()->Tick() + GameServer()->Server()->TickSpeed() * (2 + frandom() * 2);
	}
	else
	{
		// world damage
		for (int i = 0; i < 16; i++)
		{
			m_aAnger[i] += Dmg / 2;
			m_aAnger[i] *= 1.1f;
		}
	}
}

void CAI::HandleEmotions()
{
	m_TotalAnger = 0.0f;

	for (int i = 0; i < 16; i++)
	{
		m_aAnger[i] *= 0.97f;
		m_aAttachment[i] *= 0.97f;

		m_TotalAnger += m_aAnger[i];
	}

	if (m_TotalAnger > 35.0f)
		Player()->GetCharacter()->SetEmoteFor(EMOTE_ANGRY, 40, 40, false);

	if (m_PanicTick > GameServer()->Server()->Tick())
		Panic();
}

void CAI::ClearEmotions()
{
	m_PanicTick = 0;

	for (int i = 0; i < 16; i++)
	{
		m_aAnger[i] = 0.0f;
		m_aAttachment[i] = 0.0f;
	}
}

int CAI::WeaponShootRange()
{
	if(!Player()->GetCharacter())
		return 40;

	int Weapon = Player()->GetCharacter()->GetActiveWeapon();
	int Range = 40;

	if (Weapon > 0 && Weapon < NUM_WEAPONS)
		Range = 1200;
	else
		Range = 100;

	return Range;
}

void CAI::ReactToPlayer()
{
	// angry face
	// if (m_PlayerSpotCount > 20)
	//	Player()->GetCharacter()->SetEmoteFor(EMOTE_ANGRY, 0, 1200);

	if (m_PlayerSpotCount == 20 && m_TotalAnger > 35.0f)
	{
		switch (rand() % 3)
		{
		case 0:
			GameServer()->SendEmoticon(Player()->GetCID(), EMOTICON_SPLATTEE);
			break;
		case 1:
			GameServer()->SendEmoticon(Player()->GetCID(), EMOTICON_EXCLAMATION);
			break;
		default: /*__*/;
		}
	}

	if (m_PlayerSpotCount == 80)
	{
		switch (rand() % 3)
		{
		case 0:
			GameServer()->SendEmoticon(Player()->GetCID(), EMOTICON_ZOMG);
			break;
		case 1:
			GameServer()->SendEmoticon(Player()->GetCID(), EMOTICON_WTF);
			break;
		default: /*__*/;
		}
	}
}

void CAI::ShootAtClosestEnemy()
{
	CCharacter *pClosestCharacter = NULL;
	int ClosestDistance = 0;

	int ID = m_pPlayer->GetCID();

	// FIRST_BOT_ID, fix
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pPlayer = GameServer()->m_apPlayers[i];
		if (!pPlayer)
			continue;

		if (pPlayer == Player())
			continue;

		if (pPlayer->GetTeam() == Player()->GetTeam() && GameServer()->m_pController->IsTeamplay())
			continue;

		CCharacter *pCharacter = pPlayer->GetCharacter();
		if (!pCharacter)
			continue;

		if (!pCharacter->IsAlive())
			continue;

		if (!GameServer()->Server()->Translate(ID, i))
			continue;

		int Distance = distance(pCharacter->m_Pos, m_LastPos);
		if (Distance < 800 &&
			!GameServer()->Collision()->FastIntersectLine(pCharacter->m_Pos, m_LastPos))
		{
			if (!pClosestCharacter || Distance < ClosestDistance)
			{
				pClosestCharacter = pCharacter;
				ClosestDistance = Distance;
				m_PlayerDirection = pCharacter->m_Pos - m_LastPos;
				m_PlayerPos = pCharacter->m_Pos;
			}
		}
	}

	if (pClosestCharacter && ClosestDistance < WeaponShootRange() * 1.2f)
	{
		vec2 AttackDirection = vec2(m_PlayerDirection.x + ClosestDistance * (frandom() * 0.3f - frandom() * 0.3f), m_PlayerDirection.y + ClosestDistance * (frandom() * 0.3f - frandom() * 0.3f));

		if (m_HookTick < GameServer()->Server()->Tick() - 20)
			m_Direction = AttackDirection;

		// shooting part
		if (m_AttackTimer++ > g_Config.m_SvBotReactTime)
		{
			if (ClosestDistance < WeaponShootRange() && abs(atan2(m_Direction.x, m_Direction.y) - atan2(AttackDirection.x, AttackDirection.y)) < PI / 4.0f)
			{
				m_Attack = 1;
				if (frandom() * 30 < 2 && !Player()->GetCharacter()->GetActiveWeapon() == WEAPON_HAMMER)
					m_DontMoveTick = GameServer()->Server()->Tick() + GameServer()->Server()->TickSpeed() * (1 + frandom());
			}
		}
	}

	Player()->GetCharacter()->AutoWeaponChange();
}

void CAI::RandomlyStopShooting()
{
	if (frandom() * 20 < 4 && m_Attack == 1)
	{
		m_Attack = 0;

		m_AttackTimer = g_Config.m_SvBotReactTime * 0.5f;
	}
}

bool CAI::SeekRandomEnemy()
{
	if (m_pTargetPlayer && m_pTargetPlayer->GetCharacter() && m_pTargetPlayer->GetCharacter()->IsAlive())
	{
		m_PlayerDirection = m_pTargetPlayer->GetCharacter()->m_Pos - m_Pos;
		m_PlayerPos = m_pTargetPlayer->GetCharacter()->m_Pos;
		m_PlayerDistance = distance(m_pTargetPlayer->GetCharacter()->m_Pos, m_Pos);
		return true;
	}

	int ID = m_pPlayer->GetCID();
	int i = 0;
	while (i++ < 9)
	{
		int p = rand() % MAX_CLIENTS;

		CPlayer *pPlayer = GameServer()->m_apPlayers[p];
		if (!pPlayer)
			continue;

		if (pPlayer == Player())
			continue;

		if (pPlayer->GetTeam() == Player()->GetTeam() && GameServer()->m_pController->IsTeamplay())
			continue;

		CCharacter *pCharacter = pPlayer->GetCharacter();
		if (!pCharacter)
			continue;

		if (!pCharacter->IsAlive())
			continue;

		if (!GameServer()->Server()->Translate(ID, p))
			continue;

		m_pTargetPlayer = pPlayer;
		m_PlayerDirection = m_pTargetPlayer->GetCharacter()->m_Pos - m_Pos;
		m_PlayerPos = m_pTargetPlayer->GetCharacter()->m_Pos;
		m_PlayerDistance = distance(m_pTargetPlayer->GetCharacter()->m_Pos, m_Pos);
		return true;
	}

	return false;
}

bool CAI::SeekClosestFriend()
{
	CCharacter *pClosestCharacter = NULL;
	int ClosestDistance = 0;

	int ID = m_pPlayer->GetCID();
	// FIRST_BOT_ID, fix
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pPlayer = GameServer()->m_apPlayers[i];
		if (!pPlayer)
			continue;

		if (pPlayer == Player())
			continue;

		if (pPlayer->GetTeam() != Player()->GetTeam() || !GameServer()->m_pController->IsTeamplay())
			continue;

		CCharacter *pCharacter = pPlayer->GetCharacter();
		if (!pCharacter)
			continue;

		if (!pCharacter->IsAlive())
			continue;

		if (!GameServer()->Server()->Translate(ID, i))
			continue;

		int Distance = distance(pCharacter->m_Pos, m_LastPos);
		if ((!pClosestCharacter || Distance < ClosestDistance))
		{
			pClosestCharacter = pCharacter;
			ClosestDistance = Distance;
			m_PlayerDirection = pCharacter->m_Pos - m_LastPos;
			m_PlayerPos = pCharacter->m_Pos;
		}
	}

	if (pClosestCharacter)
	{
		m_PlayerDistance = ClosestDistance;
		return true;
	}

	return false;
}

bool CAI::SeekClosestEnemy()
{
	CCharacter *pClosestCharacter = NULL;
	int ClosestDistance = 0;

	int ID = m_pPlayer->GetCID();
	// FIRST_BOT_ID, fix
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pPlayer = GameServer()->m_apPlayers[i];
		if (!pPlayer)
			continue;

		if (pPlayer == Player())
			continue;

		if (pPlayer->GetTeam() == Player()->GetTeam() && GameServer()->m_pController->IsTeamplay())
			continue;

		CCharacter *pCharacter = pPlayer->GetCharacter();
		if (!pCharacter)
			continue;

		if (!pCharacter->IsAlive())
			continue;

		if (!GameServer()->Server()->Translate(ID, i))
			continue;

		int Distance = distance(pCharacter->m_Pos, m_LastPos);
		if ((!pClosestCharacter || Distance < ClosestDistance))
		{
			pClosestCharacter = pCharacter;
			ClosestDistance = Distance;
			m_PlayerDirection = pCharacter->m_Pos - m_LastPos;
			m_PlayerPos = pCharacter->m_Pos;
		}
	}

	if (pClosestCharacter)
	{
		m_PlayerDistance = ClosestDistance;
		return true;
	}

	return false;
}

bool CAI::SeekClosestEnemyInSight()
{
	CCharacter *pClosestCharacter = NULL;
	int ClosestDistance = 0;

	m_EnemiesInSight = 0;

	int ID = m_pPlayer->GetCID();
	// FIRST_BOT_ID, fix
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pPlayer = GameServer()->m_apPlayers[i];
		if (!pPlayer)
			continue;

		if (pPlayer == Player())
			continue;

		if (pPlayer->GetTeam() == Player()->GetTeam() && GameServer()->m_pController->IsTeamplay())
			continue;

		CCharacter *pCharacter = pPlayer->GetCharacter();
		if (!pCharacter)
			continue;

		if (!pCharacter->IsAlive())
			continue;

		if (!GameServer()->Server()->Translate(ID, i))
			continue;

		int Distance = distance(pCharacter->m_Pos, m_LastPos);
		if (Distance < 900 &&
			!GameServer()->Collision()->FastIntersectLine(pCharacter->m_Pos, m_LastPos))
		//! GameServer()->Collision()->IntersectLine(pCharacter->m_Pos, m_LastPos, NULL, NULL))
		{
			m_EnemiesInSight++;

			if (!pClosestCharacter || Distance < ClosestDistance)
			{
				pClosestCharacter = pCharacter;
				ClosestDistance = Distance;
				m_PlayerDirection = pCharacter->m_Pos - m_LastPos;
				m_PlayerPos = pCharacter->m_Pos;
			}
		}
	}

	if (m_EnemiesInSight == 0)
		m_DontMoveTick = 0;

	if (pClosestCharacter)
	{
		m_PlayerSpotCount++;
		m_PlayerDistance = ClosestDistance;
		return true;
	}

	m_PlayerSpotCount = 0;
	return false;
}

void CAI::Tick()
{
	// 睡着的AI不能动
	if(m_pPlayer->m_InSleep)
	{
		Reset();
		Player()->GetCharacter()->GetCore().m_Vel = vec2(0, 0);
		Waking();
		m_WakeTimer++;
		if(m_WakeTimer >= m_QiChuangQi)
			WakeAI();

		return;
	}
	m_NextReaction--;

	// character check & position update
	if (m_pPlayer->GetCharacter())
		m_Pos = m_pPlayer->GetCharacter()->m_Pos;
	else
		return;

	// skip if sleeping or stunned
	if (m_Sleep > 0 || m_Stun > 0)
	{
		if (m_Sleep > 0)
			m_Sleep--;

		if (m_Stun > 0)
			m_Stun--;

		m_Move = 0;
		m_Jump = 0;
		m_Hook = 0;
		m_Attack = 0;
		m_InputChanged = true;

		return;
	}

	HandleEmotions();

	// stupid AI can't even react to events every frame
	if (m_NextReaction <= 0)
	{
		m_NextReaction = m_ReactionTime;

		if(m_ThreadDone)
			DoBehavior();

		if (m_DontMoveTick > GameServer()->Server()->Tick())
		{
			m_Move = 0;
			m_Hook = m_LastHook;
			m_Jump = 0;
		}

		if (m_pPlayer->GetCharacter())
			m_LastPos = m_pPlayer->GetCharacter()->m_Pos;
		m_LastMove = m_Move;
		m_LastJump = m_Jump;
		m_LastAttack = m_Attack;
		m_LastHook = m_Hook;

		if (m_OldTargetPos != m_TargetPos)
		{
			m_OldTargetPos = m_TargetPos;
			m_WaypointUpdateNeeded = true;
		}

		m_InputChanged = true;
	}
	else
	{
		m_Attack = 0;
	}
	m_InputChanged = true;

	m_DisplayDirection.x += (m_Direction.x - m_DisplayDirection.x) / 4.0f;
	m_DisplayDirection.y += (m_Direction.y - m_DisplayDirection.y) / 4.0f;
}

void CAI::WakeAI()
{
	Player()->m_InSleep = false;
	m_WakeTimer = 0;
	Player()->GetCharacter()->IncreaseHealth(10);
}
