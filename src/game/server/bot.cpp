#include <game/generated/protocol.h>
#include <game/gamecore.h>
#include <engine/shared/config.h>
#include <game/layers.h>
#include "gamecontext.h"

#include "botengine.h"

#include "bot.h"
#include "player.h"
#include "entities/character.h"
#include "entities/flag.h"
#include "entities/pickup.h"

#include "ai/defence.h"

CBot::CBot(CBotEngine *pBotEngine, CPlayer *pPlayer) : m_Genetics(CTarget::NUM_TARGETS,10)
{
	m_pBotEngine = pBotEngine;
	m_pPlayer = pPlayer;
	m_pGameServer = pBotEngine->GameServer();
	m_Flags = 0;
	mem_zero(&m_InputData, sizeof(m_InputData));
	m_LastData = m_InputData;

	m_SnapID = GameServer()->Server()->SnapNewID();
	m_ComputeTarget.m_Type = CTarget::TARGET_EMPTY;

	m_pPath = &(pBotEngine->m_aPaths[pPlayer->GetCID()]);
	UpdateTargetOrder();

	BotEngine()->RegisterBot(m_pPlayer->GetCID(), this);

	m_pStrategyPosition = NULL;
}

CBot::~CBot()
{
	delete m_pStrategyPosition;
	BotEngine()->UnRegisterBot(m_pPlayer->GetCID());
	m_pPath->m_Size = 0;
	GameServer()->Server()->SnapFreeID(m_SnapID);
}

void CBot::OnReset()
{
	m_Flags = 0;
	m_pPath->m_Size = 0;
	m_ComputeTarget.m_Type = CTarget::TARGET_HEALTH;
}

void CBot::UpdateTargetOrder()
{
	//int *pGenome = m_Genetics.GetGenome();
	const int *pGenome = &g_aBotPriority[m_pPlayer->GetCID()%8][0];
	for(int i = 0 ; i < CTarget::NUM_TARGETS ; i++)
	{
		int j = i;
		while(j > 0 && pGenome[i] > pGenome[m_aTargetOrder[j-1]])
		{
			m_aTargetOrder[j] = m_aTargetOrder[j-1];
			j--;
		}
		m_aTargetOrder[j] = i;
	}
}

vec2 CBot::ClosestCharacter()
{
	float d = -1;
	vec2 Pos = vec2(0,0);
	for(int c = 0; c < MAX_CLIENTS; c++)
		if(c != m_pPlayer->GetCID() && GameServer()->m_apPlayers[c] && GameServer()->m_apPlayers[c]->GetCharacter() && (d < -1 || d > distance(m_pPlayer->GetCharacter()->GetPos(),GameServer()->m_apPlayers[c]->GetCharacter()->GetPos())))
		{
			d = distance(m_pPlayer->GetCharacter()->GetPos(),GameServer()->m_apPlayers[c]->GetCharacter()->GetPos());
			Pos = GameServer()->m_apPlayers[c]->GetCharacter()->GetPos();
		}
	return Pos;
}

void CBot::UpdateTarget()
{
	//m_GenomeTick++;
	bool FindNewTarget = m_ComputeTarget.m_Type == CTarget::TARGET_EMPTY;// || !m_pPath->m_Size;
	if(m_ComputeTarget.m_Type == CTarget::TARGET_PLAYER && !(GameServer()->m_apPlayers[m_ComputeTarget.m_PlayerCID] && GameServer()->m_apPlayers[m_ComputeTarget.m_PlayerCID]->GetCharacter()))
		FindNewTarget = true;
	// Timeout: 30s
	if(m_ComputeTarget.m_StartTick + 30 * GameServer()->Server()->TickSpeed() < GameServer()->Server()->Tick())
		FindNewTarget = true;

	if(m_ComputeTarget.m_Type == CTarget::TARGET_AIR)
	{
		float dist = distance(m_pPlayer->GetCharacter()->GetPos(), m_ComputeTarget.m_Pos);
		if(dist < 60)
			FindNewTarget = true;
	}
	if(m_ComputeTarget.m_Type > CTarget::TARGET_PLAYER)
	{
		float dist = distance(m_pPlayer->GetCharacter()->GetPos(), m_ComputeTarget.m_Pos);
		if(dist < 28)
			FindNewTarget = true;
	}
	if(FindNewTarget)
	{
		m_ComputeTarget.m_StartTick = GameServer()->Server()->Tick();
		m_ComputeTarget.m_NeedUpdate = true;
		m_ComputeTarget.m_Type = CTarget::TARGET_EMPTY;
		vec2 NewTarget;
		for(int i = 0 ; i < CTarget::NUM_TARGETS ; i++)
		{
			switch(m_aTargetOrder[i])
			{
			case CTarget::TARGET_ARMOR:
			case CTarget::TARGET_HEALTH:
			case CTarget::TARGET_WEAPON_SHOTGUN:
			case CTarget::TARGET_WEAPON_GRENADE:
			case CTarget::TARGET_WEAPON_LASER:
				{
					float Radius = distance(m_pPlayer->GetCharacter()->GetPos(), ClosestCharacter());
					if(NeedPickup(m_aTargetOrder[i]) && FindPickup(m_aTargetOrder[i], &m_ComputeTarget.m_Pos, Radius))
					{
						m_ComputeTarget.m_Type = m_aTargetOrder[i];
						return;
					}
				}
				break;
			case CTarget::TARGET_PLAYER:
				{
					int Team = m_pPlayer->GetTeam();
					int Count = 0;
					for(int c = 0; c < MAX_CLIENTS; c++)
						if(c != m_pPlayer->GetCID() && GameServer()->m_apPlayers[c] && GameServer()->m_apPlayers[c]->GetCharacter() && (GameServer()->m_apPlayers[c]->GetTeam() != Team))
							Count++;
					if(Count)
					{
						Count = rand()%Count+1;
						int c = 0;
						for(; Count; c++)
							if(c != m_pPlayer->GetCID() && GameServer()->m_apPlayers[c] && GameServer()->m_apPlayers[c]->GetCharacter() && (GameServer()->m_apPlayers[c]->GetTeam() != Team))
								Count--;
						c--;
						m_ComputeTarget.m_Pos = GameServer()->m_apPlayers[c]->GetCharacter()->GetPos();
						m_ComputeTarget.m_Type = CTarget::TARGET_PLAYER;
						m_ComputeTarget.m_PlayerCID = c;
						return;
					}
				}
				break;
			case CTarget::TARGET_AIR:
				{
					// Random destination
					int Count = 0;
					for(int v = 0 ; v < BotEngine()->GetGraph()->m_NumVertices ; v++)
						if(m_pStrategyPosition->IsInsideZone(BotEngine()->GetGraph()->m_pVertices[v].m_Pos))
							Count++;
					if(Count)
					{
						Count = rand()%Count+1;
						int v = 0;
						for(; Count ; v++)
							if(m_pStrategyPosition->IsInsideZone(BotEngine()->GetGraph()->m_pVertices[v].m_Pos))
								Count--;
						m_ComputeTarget.m_Pos = BotEngine()->GetGraph()->m_pVertices[--v].m_Pos;
						m_ComputeTarget.m_Type = CTarget::TARGET_AIR;
						return;
					}
				}
			}
		}
	}

	if(m_ComputeTarget.m_Type == CTarget::TARGET_PLAYER && m_pPlayer->GetCharacter()->GetHealth() > 7 && m_pPlayer->GetCharacter()->GetArmor() > 7)
	{
		CPlayer *pPlayer = GameServer()->m_apPlayers[m_ComputeTarget.m_PlayerCID];
		if(Collision()->FastIntersectLine(m_ComputeTarget.m_Pos, pPlayer->GetCharacter()->GetPos(), 0, 0))
		{
			m_ComputeTarget.m_NeedUpdate = true;
			m_ComputeTarget.m_Pos = pPlayer->GetCharacter()->GetPos();
		}
	}
}

bool CBot::NeedPickup(int Type)
{
	switch(Type)
	{
	case CTarget::TARGET_HEALTH:
		return m_pPlayer->GetCharacter()->GetHealth() < 7;
	case CTarget::TARGET_ARMOR:
		return m_pPlayer->GetCharacter()->GetArmor() < 7;
	case CTarget::TARGET_WEAPON_SHOTGUN:
		return m_pPlayer->GetCharacter()->GetAmmoCount(WEAPON_SHOTGUN) < 4;
	case CTarget::TARGET_WEAPON_GRENADE:
		return m_pPlayer->GetCharacter()->GetAmmoCount(WEAPON_GRENADE) < 2;
	case CTarget::TARGET_WEAPON_LASER:
		return m_pPlayer->GetCharacter()->GetAmmoCount(WEAPON_RIFLE) < 2;
	}
	return false;
}

bool CBot::FindPickup(int Type, vec2 *pPos, float Radius)
{
	int SubType = 0;
	switch(Type)
	{
		case CTarget::TARGET_ARMOR:
			Type = POWERUP_ARMOR;
			break;
		case CTarget::TARGET_HEALTH:
			Type = POWERUP_HEALTH;
			break;
		case CTarget::TARGET_WEAPON_SHOTGUN:
			Type = POWERUP_WEAPON;
			SubType = WEAPON_SHOTGUN;
			break;
		case CTarget::TARGET_WEAPON_GRENADE:
			Type = POWERUP_WEAPON;
			SubType = WEAPON_GRENADE;
			break;
		case CTarget::TARGET_WEAPON_LASER:
			Type = POWERUP_WEAPON;
			SubType = WEAPON_RIFLE;
			break;
	}
	CEntity *pEnt = GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_PICKUP);
	bool Found = false;
	for(;	pEnt; pEnt = pEnt->TypeNext())
	{
		CPickup *pPickup = (CPickup *) pEnt;
		if(pPickup->GetType() == Type && pPickup->GetSubType() == SubType && pPickup->IsSpawned() && Radius > distance(pPickup->GetPos(),m_pPlayer->GetCharacter()->GetPos()) )
		{
			*pPos = pPickup->GetPos();
			Radius = distance(pPickup->GetPos(),m_pPlayer->GetCharacter()->GetPos());
			Found = true;
		}
	}
	return Found;
}

bool CBot::IsGrounded()
{
	return m_pPlayer->GetCharacter()->IsGrounded();
}

void CBot::Tick()
{
	if(!m_pPlayer->GetCharacter())
		return;

	if(m_pStrategyPosition == NULL)
	{
		m_pStrategyPosition = new CDefence(BotEngine());
		m_pStrategyPosition->SetTeam(0);
	}

	const CCharacterCore *pMe = m_pPlayer->GetCharacter()->GetCore();

	UpdateTarget();
	UpdateEdge();

	mem_zero(&m_InputData, sizeof(m_InputData));

	m_InputData.m_WantedWeapon = m_LastData.m_WantedWeapon;

	vec2 Pos = pMe->m_Pos;

	bool InSight = false;
	if(m_ComputeTarget.m_Type == CTarget::TARGET_PLAYER)
	{
		const CCharacterCore *pClosest = GameServer()->m_apPlayers[m_ComputeTarget.m_PlayerCID]->GetCharacter()->GetCore();
		InSight = !Collision()->FastIntersectLine(Pos, pClosest->m_Pos, 0, 0);
		m_Target = pClosest->m_Pos - Pos;
		m_RealTarget = pClosest->m_Pos;
	}

	if(g_Config.m_SvBotAllowMove)
		MakeChoice(InSight);

	m_RealTarget = m_Target + Pos;

	if(g_Config.m_SvBotAllowFire && m_pPlayer->GetCharacter()->CanFire())
		HandleWeapon(InSight);

	if(g_Config.m_SvBotAllowMove && g_Config.m_SvBotAllowHook)
		HandleHook(InSight);

	if(m_Flags & BFLAG_LEFT)
			m_InputData.m_Direction = -1;
	if(m_Flags & BFLAG_RIGHT)
			m_InputData.m_Direction = 1;
	if(m_Flags & BFLAG_JUMP)
			m_InputData.m_Jump = 1;
	// else if(!m_InputData.m_Fire && m_Flags & BFLAG_FIRE && m_LastData.m_Fire == 0)
	// 	m_InputData.m_Fire = 1;

	// if(InSight && diffPos.y < - Close && diffVel.y < 0)
	// 	m_InputData.m_Jump = 1;

	m_InputData.m_TargetX = m_LastData.m_TargetX;
	m_InputData.m_TargetY = m_LastData.m_TargetY;
	if(m_InputData.m_Hook || m_InputData.m_Fire) {
		m_InputData.m_TargetX = m_Target.x;
		m_InputData.m_TargetY = m_Target.y;
	}


	if(!g_Config.m_SvBotAllowMove) {
		m_InputData.m_Direction = 0;
		m_InputData.m_Jump = 0;
		m_InputData.m_Hook = 0;
	}
	if(!g_Config.m_SvBotAllowHook)
		m_InputData.m_Hook = 0;

	m_LastData = m_InputData;
	return;
}

void CBot::HandleHook(bool SeeTarget)
{
	CCharacterCore *pMe = m_pPlayer->GetCharacter()->GetCore();

	if(!pMe)
		return;
	int CurTile = GetTile(pMe->m_Pos.x, pMe->m_Pos.y);
	if(pMe->m_HookState == HOOK_FLYING)
	{
		m_InputData.m_Hook = 1;
		return;
	}
	if(SeeTarget)
	{
		const CCharacterCore *pClosest = GameServer()->m_apPlayers[m_ComputeTarget.m_PlayerCID]->GetCharacter()->GetCore();
		float dist = distance(pClosest->m_Pos,pMe->m_Pos);
		if(pMe->m_HookState == HOOK_GRABBED && pMe->m_HookedPlayer == m_ComputeTarget.m_PlayerCID)
			m_InputData.m_Hook = 1;
		else if(!m_InputData.m_Fire)
		{
			if(dist < Tuning()->m_HookLength*0.9f)
				m_InputData.m_Hook = m_LastData.m_Hook^1;
			SeeTarget = dist < Tuning()->m_HookLength*0.9f;
		}
	}
	if(!SeeTarget)
	{
		if(pMe->m_HookState == HOOK_GRABBED && pMe->m_HookedPlayer == -1)
		{
			vec2 HookVel = normalize(pMe->m_HookPos-pMe->m_Pos)*GameServer()->Tuning()->m_HookDragAccel;

			// from gamecore;cpp
			if(HookVel.y > 0)
				HookVel.y *= 0.3f;
			if((HookVel.x < 0 && pMe->m_Input.m_Direction < 0) || (HookVel.x > 0 && pMe->m_Input.m_Direction > 0))
				HookVel.x *= 0.95f;
			else
				HookVel.x *= 0.75f;

			HookVel += vec2(0,1)*GameServer()->Tuning()->m_Gravity;

			vec2 Target = m_Target;
			float ps = dot(Target, HookVel);
			if(ps > 0 || (CurTile & BTILE_HOLE && m_Target.y < 0 && pMe->m_Vel.y > 0.f && pMe->m_HookTick < SERVER_TICK_SPEED + SERVER_TICK_SPEED/2))
				m_InputData.m_Hook = 1;
			if(pMe->m_HookTick > 4*SERVER_TICK_SPEED || length(pMe->m_HookPos-pMe->m_Pos) < 20.0f)
				m_InputData.m_Hook = 0;
			// if(Flags & BFLAG_HOOK && ps < dot(Target,HookVel-Accel))
			// 	Flags ^= BFLAG_RIGHT | BFLAG_LEFT;
		}
		if(pMe->m_HookState == HOOK_FLYING)
			m_InputData.m_Hook = 1;
		// do random hook
		if(!m_InputData.m_Fire && m_LastData.m_Hook == 0 && pMe->m_HookState == HOOK_IDLE && (rand()%10 == 0 || (CurTile & BTILE_HOLE && rand()%4 == 0)))
		{
			int NumDir = BOT_HOOK_DIRS;
			vec2 HookDir(0.0f,0.0f);
			float MaxForce = (CurTile & BTILE_HOLE) ? -10000.0f : 0;
			vec2 Target = m_Target;
			for(int i = 0 ; i < NumDir; i++)
			{
				float a = 2*i*pi / NumDir;
				vec2 dir = direction(a);
				vec2 Pos = pMe->m_Pos+dir*Tuning()->m_HookLength;

				if((Collision()->FastIntersectLine(pMe->m_Pos,Pos,&Pos,0) & (CCollision::COLFLAG_SOLID | CCollision::COLFLAG_NOHOOK)) == CCollision::COLFLAG_SOLID)
				{
					vec2 HookVel = dir*GameServer()->Tuning()->m_HookDragAccel;

					// from gamecore.cpp
					if(HookVel.y > 0)
						HookVel.y *= 0.3f;
					if((HookVel.x < 0 && pMe->m_Input.m_Direction < 0) || (HookVel.x > 0 && pMe->m_Input.m_Direction > 0))
						HookVel.x *= 0.95f;
					else
						HookVel.x *= 0.75f;

					HookVel += vec2(0,1)*GameServer()->Tuning()->m_Gravity;

					float ps = dot(Target, HookVel);
					if( ps > MaxForce)
					{
						MaxForce = ps;
						HookDir = Pos - pMe->m_Pos;
					}
				}
			}
			if(length(HookDir) > 32.f)
			{
				m_Target = HookDir;
				m_InputData.m_Hook = 1;
				// if(Collision()->CheckPoint(pMe->m_Pos+normalize(vec2(0,m_Target.y))*28) && absolute(Target.x) < 30)
				// 	Flags = (Flags & (~BFLAG_LEFT)) | BFLAG_RIGHT;
			}
		}
	}
}

void CBot::HandleWeapon(bool SeeTarget)
{
	CCharacter *pMe = m_pPlayer->GetCharacter();

	if(!pMe)
		return;

	int Team = m_pPlayer->GetTeam();
	vec2 Pos = pMe->GetCore()->m_Pos;

	CCharacterCore* apTarget[MAX_CLIENTS];
	int Count = 0;

	for(int c = 0 ; c < MAX_CLIENTS ; c++)
	{
		if(c == m_pPlayer->GetCID())
			continue;
		if(SeeTarget && c == m_ComputeTarget.m_PlayerCID)
		{
			apTarget[Count++] = apTarget[0];
			apTarget[0] =	GameServer()->m_apPlayers[c]->GetCharacter()->GetCore();
		}
		else if(GameServer()->m_apPlayers[c] && GameServer()->m_apPlayers[c]->GetCharacter() && (GameServer()->m_apPlayers[c]->GetTeam() != Team))
			apTarget[Count++] = GameServer()->m_apPlayers[c]->GetCharacter()->GetCore();
	}
	int Weapon = -1;
	vec2 Target;
	for(int c = 0; c < Count; c++)
	{
		float ClosestRange = distance(Pos, apTarget[c]->m_Pos);
		float Close = 65.0f;
		Target = apTarget[c]->m_Pos - Pos;
		if(ClosestRange < Close)
		{
			Weapon = WEAPON_HAMMER;
			break;
		}
		else if(pMe->GetAmmoCount(WEAPON_RIFLE) != 0 && ClosestRange < GameServer()->Tuning()->m_LaserReach && !Collision()->FastIntersectLine(Pos, apTarget[c]->m_Pos, 0, 0))
		{
			Weapon = WEAPON_RIFLE;
			break;
		}
	}
	if(Weapon < 0)
	{
		int GoodDir = -1;

		vec2 aProjectilePos[BOT_HOOK_DIRS];

		const int NbLoops = 10;

		vec2 aTargetPos[MAX_CLIENTS];
		vec2 aTargetVel[MAX_CLIENTS];

		const int Weapons[] = {WEAPON_GRENADE, WEAPON_SHOTGUN, WEAPON_GUN};
		for(int j = 0 ; j < 3 ; j++)
		{
			if(!pMe->GetAmmoCount(Weapons[j]))
				continue;
			float Curvature = 0, Speed = 0, Time = 0;
			switch(Weapons[j])
			{
				case WEAPON_GRENADE:
					Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
					Speed = GameServer()->Tuning()->m_GrenadeSpeed;
					Time = GameServer()->Tuning()->m_GrenadeLifetime;
					break;

				case WEAPON_SHOTGUN:
					Curvature = GameServer()->Tuning()->m_ShotgunCurvature;
					Speed = GameServer()->Tuning()->m_ShotgunSpeed;
					Time = GameServer()->Tuning()->m_ShotgunLifetime;
					break;

				case WEAPON_GUN:
					Curvature = GameServer()->Tuning()->m_GunCurvature;
					Speed = GameServer()->Tuning()->m_GunSpeed;
					Time = GameServer()->Tuning()->m_GunLifetime;
					break;
			}
			//DTime /= NbLoops;
			int DTick = (int) (Time * GameServer()->Server()->TickSpeed() / NbLoops);
			// DTime *= Speed;
			// Curvature *= 0.00001f;

			for(int c = 0; c < Count; c++)
			{
				aTargetPos[c] = apTarget[c]->m_Pos;
				aTargetVel[c] = apTarget[c]->m_Vel*DTick;
			}

			for(int i = 0 ; i < BOT_HOOK_DIRS ; i++) {
				vec2 dir = direction(2*i*pi / BOT_HOOK_DIRS);
				aProjectilePos[i] = Pos + dir*28.*0.75;
			}

			int aIsDead[BOT_HOOK_DIRS] = {0};

			for(int k = 0; k < NbLoops && GoodDir == -1; k++) {
				for(int i = 0; i < BOT_HOOK_DIRS; i++) {
					if(aIsDead[i])
						continue;
					vec2 dir = direction(2*i*pi / BOT_HOOK_DIRS);
					vec2 NextPos = CalcPos(Pos + dir*28.*0.75, dir, Curvature, Speed, (k+1) * Time / NbLoops);
					// vec2 NextPos = aProjectilePos[i];
					// NextPos.x += dir.x*DTime;
					// NextPos.y += dir.y*DTime + Curvature*(DTime*DTime)*(2*k+1);
					aIsDead[i] = Collision()->FastIntersectLine(aProjectilePos[i], NextPos, &NextPos, 0);
					for(int c = 0; c < Count; c++)
					{
						vec2 InterPos = closest_point_on_line(aProjectilePos[i],NextPos, aTargetPos[c]);
						if(distance(aTargetPos[c], InterPos)< 28) {
							GoodDir = i;
							break;
						}
					}
					aProjectilePos[i] = NextPos;
				}
				for(int c = 0; c < Count; c++)
				{
					//Collision()->MoveBox(&aTargetPos[c], &aTargetVel[c], vec2(28.f,28.f), 0);
					Collision()->FastIntersectLine(aTargetPos[c], aTargetPos[c]+aTargetVel[c], 0, &aTargetPos[c]);
					aTargetVel[c].y += GameServer()->Tuning()->m_Gravity*DTick*DTick;
				}
			}
			if(GoodDir != -1)
			{
				Target = direction(2*GoodDir*pi / BOT_HOOK_DIRS)*50;
				Weapon = Weapons[j];
				break;
			}
		}
	}
	if(Weapon > -1)
	{
		m_InputData.m_WantedWeapon = Weapon+1;
		m_InputData.m_Fire = m_LastData.m_Fire^1;
		if(m_InputData.m_Fire)
			m_Target = Target;
	}
	else if(pMe->GetAmmoCount(WEAPON_GUN) != 10)
		m_InputData.m_WantedWeapon = WEAPON_GUN+1;


	// Accuracy
	float Angle = angle(m_Target) + (rand()%64-32)*pi / 1024.0f;
	m_Target = direction(Angle)*length(m_Target);
}

void CBot::UpdateEdge()
{
	vec2 Pos = m_pPlayer->GetCharacter()->GetPos();
	if(m_ComputeTarget.m_Type == CTarget::TARGET_EMPTY)
	{
		dbg_msg("bot", "no edge");
		return;
	}
	if(m_ComputeTarget.m_NeedUpdate)
	{
		m_pPath->m_Size = 0;
		BotEngine()->GetPath(Pos, m_ComputeTarget.m_Pos, m_pPath);
		m_ComputeTarget.m_NeedUpdate = false;
		// dbg_msg("bot", "%d new path of size=%d for type=%d cid=%d", m_pPlayer->GetCID(), m_pPath->m_Size, m_ComputeTarget.m_Type, m_ComputeTarget.m_PlayerCID);
	}
}

void CBot::MakeChoice(bool UseTarget)
{
	if(!UseTarget)
	{
		vec2 Pos = m_pPlayer->GetCharacter()->GetPos();

		if(m_pPath->m_Size)
		{
			int dist = BotEngine()->FarestPointOnEdge(m_pPath, Pos, &m_Target);
			if(dist >= 0)
			{
				UseTarget = true;
				m_Target -= Pos;
			}
			else
				m_Target = BotEngine()->NextPoint(Pos,m_ComputeTarget.m_Pos) - Pos;
		}
	}

	int Flags = 0;
	CCharacterCore *pMe = m_pPlayer->GetCharacter()->GetCore();
	CCharacterCore TempChar = *pMe;
	TempChar.m_Input = m_InputData;
	vec2 CurPos = TempChar.m_Pos;

	int CurTile = GetTile(TempChar.m_Pos.x, TempChar.m_Pos.y);
	bool Grounded = IsGrounded();

	TempChar.m_Input.m_Direction = (m_Target.x > 28.f) ? 1 : (m_Target.x < -28.f) ? -1:0;
	CWorldCore TempWorld;
	TempWorld.m_Tuning = *GameServer()->Tuning();
	TempChar.Init(&TempWorld, Collision());
	TempChar.Tick(true);
	TempChar.Move();
	TempChar.Quantize();

	int NextTile = GetTile(TempChar.m_Pos.x, TempChar.m_Pos.y);
	vec2 NextPos = TempChar.m_Pos;

	if(TempChar.m_Input.m_Direction > 0)
		Flags |= BFLAG_RIGHT;

	if(TempChar.m_Input.m_Direction < 0)
		Flags |= BFLAG_LEFT;

	if(m_Target.y < 0)
	{
		if(CurTile & BTILE_SAFE && NextTile & BTILE_HOLE && (Grounded || TempChar.m_Vel.y > 0))
			Flags |= BFLAG_JUMP;
		if(CurTile & BTILE_SAFE && NextTile & BTILE_SAFE)
		{
			static bool tried = false;
			if(absolute(CurPos.x - NextPos.x) < 1.0f && TempChar.m_Input.m_Direction)
			{
				if(Grounded)
				{
					Flags |= BFLAG_JUMP;
					tried = true;
				}
				else if(tried && !(TempChar.m_Jumped) && TempChar.m_Vel.y > 0)
					Flags |= BFLAG_JUMP;
				else if(tried && TempChar.m_Jumped & 2 && TempChar.m_Vel.y > 0)
					Flags ^= BFLAG_RIGHT | BFLAG_LEFT;
			}
			else
				tried = false;
			// if(m_Target.y < 0 && TempChar.m_Vel.y > 1.f && !(TempChar.m_Jumped) && !Grounded)
			// 	Flags |= BFLAG_JUMP;
		}

		if(!(pMe->m_Jumped))
		{
			vec2 Vel(pMe->m_Vel.x, min(pMe->m_Vel.y, 0.0f));
			if(Collision()->FastIntersectLine(pMe->m_Pos,pMe->m_Pos+Vel*10.0f,0,0) && !Collision()->FastIntersectLine(pMe->m_Pos,pMe->m_Pos+(Vel-vec2(0,TempWorld.m_Tuning.m_AirJumpImpulse))*10.0f,0,0))
				Flags |= BFLAG_JUMP;
			if(absolute(m_Target.x) < 28.f && pMe->m_Vel.y > -1.f)
				Flags |= BFLAG_JUMP;
		}
	}
	// if(Flags & BFLAG_JUMP || pMe->m_Vel.y < 0)
	// 	m_InputData.m_WantedWeapon = WEAPON_GRENADE +1;
	// if(m_Target.y < -400 && pMe->m_Vel.y < 0 && absolute(m_Target.x) < 30 && Collision()->CheckPoint(pMe->m_Pos+vec2(0,50)))
	// {
	// 	Flags &= ~BFLAG_HOOK;
	// 	Flags |= BFLAG_FIRE;
	// 	m_Target = vec2(0,28);
	// }
	// else if(m_Target.y < -300 && pMe->m_Vel.y < 0 && absolute(m_Target.x) < 30 && Collision()->CheckPoint(pMe->m_Pos+vec2(32,48)))
	// {
	// 	Flags &= ~BFLAG_HOOK;
	// 	Flags |= BFLAG_FIRE;
	// 	m_Target = vec2(14,28);
	// }
	// else if(m_Target.y < -300 && pMe->m_Vel.y < 0 && absolute(m_Target.x) < 30 && Collision()->CheckPoint(pMe->m_Pos+vec2(-32,48)))
	// {
	// 	Flags &= ~BFLAG_HOOK;
	// 	Flags |= BFLAG_FIRE;
	// 	m_Target = vec2(-14,28);
	// }
	m_Flags = Flags;
}

void CBot::Snap(int SnappingClient)
{
	if(SnappingClient == -1)
		return;

	CCharacter *pMe = m_pPlayer->GetCharacter();
	if(!pMe)
		return;

	vec2 Pos = pMe->GetCore()->m_Pos;
	{
		CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(GameServer()->Server()->SnapNewItem(NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser)));
		if(!pObj)
			return;

		pObj->m_X = (int)(m_RealTarget.x);
		pObj->m_Y = (int)(m_RealTarget.y);
		pObj->m_FromX = (int)Pos.x;
		pObj->m_FromY = (int)Pos.y;
		pObj->m_StartTick = GameServer()->Server()->Tick();
	}
	for(int l = 0 ; l < m_pPath->m_Size-1 ; l++)
	{
		vec2 From = m_pPath->m_pVertices[l];
		vec2 To = m_pPath->m_pVertices[l+1];
		if(BotEngine()->NetworkClipped(SnappingClient, To) && BotEngine()->NetworkClipped(SnappingClient, From))
			continue;
		CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(GameServer()->Server()->SnapNewItem(NETOBJTYPE_LASER, m_pPath->m_pSnapID[l], sizeof(CNetObj_Laser)));
		if(!pObj)
			return;
		pObj->m_X = (int) To.x;
		pObj->m_Y = (int) To.y;
		pObj->m_FromX = (int) From.x;
		pObj->m_FromY = (int) From.y;
		pObj->m_StartTick = GameServer()->Server()->Tick();
	}
}
