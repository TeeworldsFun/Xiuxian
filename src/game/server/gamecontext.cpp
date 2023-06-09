/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <base/math.h>
#include <engine/shared/config.h>
#include <engine/map.h>
#include <engine/console.h>
#include "gamecontext.h"
#include <game/version.h>
#include <game/collision.h>
#include <game/gamecore.h>
#include "Data/botdata.h"
#include "Data/tizhidata.h"
#include "Data/zuowangdao_names.h"
#include <string>

#include <teeuniverses/components/localization.h>
#include <thread>

#include "bot.h"

#ifdef CONF_SQLITE
void CQueryRegister::OnData()
{
	if (Next())
	{
		m_pGameServer->SendChatTarget(m_ClientID, _("⚠此方世界容不下两个相同的肉体，还请道友取个别的血肉之名⚠"));
	}
	else
	{
		if (m_pDatabase->Register(Username, Password, Language, m_ClientID))
		{
			char aBuf[256];
			m_pGameServer->SendChatTarget(m_ClientID, _("⚠⚠⚠⚠⚠⚠⚠⚠ ! ~血⚠肉⚠苦⚠痛~ ! ⚠⚠⚠⚠⚠⚠⚠⚠"));
			m_pGameServer->SendChatTarget(m_ClientID, _("血肉之名: {str:Username}"), "Username", Username);
			m_pGameServer->SendChatTarget(m_ClientID, _("血肉之匙: {str:Password}"), "Password", Password);
			m_pGameServer->SendChatTarget(m_ClientID, _("使用 /login {str:u} {str:p} 来与你的血肉之体共鸣"), "u", Username, "p", Password);
			m_pGameServer->SendChatTarget(m_ClientID, _("⚠⚠⚠⚠⚠⚠⚠⚠ ! ~魂⚠体⚠飞⚠升~ ! ⚠⚠⚠⚠⚠⚠⚠⚠"));
			m_pGameServer->Login(Username, Password, m_ClientID);
		}
	}
}

void CQueryLogin::OnData()
{
	if (Next())
	{
		//		if(m_pDatabase->Login(Username, Password, m_ClientID))
		//		{
		m_pGameServer->m_apPlayers[m_ClientID]->m_AccData.m_UserID = GetInt(GetID("ID"));
		str_format(m_pGameServer->m_apPlayers[m_ClientID]->m_AccData.m_Username, sizeof(m_pGameServer->m_apPlayers[m_ClientID]->m_AccData.m_Username), GetText(GetID("Username")));
		str_format(m_pGameServer->m_apPlayers[m_ClientID]->m_AccData.m_Password, sizeof(m_pGameServer->m_apPlayers[m_ClientID]->m_AccData.m_Password), GetText(GetID("Password")));
		m_pGameServer->m_apPlayers[m_ClientID]->m_AccData.m_Po = GetInt(GetID("Po"));
		m_pGameServer->m_apPlayers[m_ClientID]->m_AccData.m_Xiuwei = GetInt(GetID("Xiuwei"));
		m_pGameServer->Server()->SetClientLanguage(m_ClientID, GetText(GetID("Language")));

		for (int j = 0; j < MAX_CLIENTS; j++)
		{
			if ((j != m_ClientID) && m_pGameServer->m_apPlayers[m_ClientID] && m_pGameServer->m_apPlayers[j] && m_pGameServer->m_apPlayers[j]->m_AccData.m_UserID == m_pGameServer->m_apPlayers[m_ClientID]->m_AccData.m_UserID)
			{
				dbg_msg("account", "Account login failed ('%s' - already in use (local))", Username);
				m_pGameServer->SendChatTarget(m_ClientID, _("⚠ 共鸣失败 ⚠ 此血肉之体已被魂体共鸣⚠"));
				return;
			}
		}

		m_pGameServer->m_apPlayers[m_ClientID]->SetTeam(TEAM_RED);
		m_pGameServer->SendChatTarget(m_ClientID, _("⚠ 共鸣成功 ⚠ 欢迎来到异仙界 ⚠"));
		//		}
	}
	else
	{
		m_pGameServer->SendChatTarget(m_ClientID, _("⚠ 共鸣失败 ⚠ 此方世界没有找到对应血肉体 ⚠"));
	}
}

void CQueryApply::OnData()
{
	if (Next())
		m_pDatabase->Apply(Username, Password, m_Data);
}

#endif

enum
{
	RESET,
	NO_RESET
};

void CGameContext::Construct(int Resetting)
{
	m_Resetting = 0;
	m_pServer = 0;

	for (int i = 0; i < MAX_CLIENTS; i++)
		m_apPlayers[i] = 0;

	m_pController = 0;
	m_VoteCloseTime = 0;
	m_pVoteOptionFirst = 0;
	m_pVoteOptionLast = 0;
	m_NumVoteOptions = 0;
	m_LockTeams = 0;
	m_ConsoleOutputHandle_ChatPrint = -1;
	m_ConsoleOutput_Target = -1;

	if (Resetting == NO_RESET)
		m_pVoteOptionHeap = new CHeap();

	m_pBotEngine = new CBotEngine(this);

#ifdef CONF_SQLITE
	m_pDatabase = new CSql();
#endif

#ifdef CONF_SQL
	/* SQL */
	m_Sql = new CSQL(this);
#endif
}

CGameContext::CGameContext(int Resetting)
{
	Construct(Resetting);
}

CGameContext::CGameContext()
{
	Construct(NO_RESET);
}

CGameContext::~CGameContext()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
		delete m_apPlayers[i];
	if (!m_Resetting)
		delete m_pVoteOptionHeap;

	delete m_pBotEngine;
}

void CGameContext::OnSetAuthed(int ClientID, int Level)
{
	if (m_apPlayers[ClientID])
		m_apPlayers[ClientID]->m_Authed = Level;
}

void CGameContext::Clear()
{
	CHeap *pVoteOptionHeap = m_pVoteOptionHeap;
	CVoteOptionServer *pVoteOptionFirst = m_pVoteOptionFirst;
	CVoteOptionServer *pVoteOptionLast = m_pVoteOptionLast;
	int NumVoteOptions = m_NumVoteOptions;
	CTuningParams Tuning = m_Tuning;

#ifdef CONF_SQL
	delete m_Sql;
#endif

	m_Resetting = true;
	this->~CGameContext();
	mem_zero(this, sizeof(*this));
	new (this) CGameContext(RESET);

	m_pVoteOptionHeap = pVoteOptionHeap;
	m_pVoteOptionFirst = pVoteOptionFirst;
	m_pVoteOptionLast = pVoteOptionLast;
	m_NumVoteOptions = NumVoteOptions;
	m_Tuning = Tuning;
}

class CCharacter *CGameContext::GetPlayerChar(int ClientID)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS || !m_apPlayers[ClientID])
		return 0;
	return m_apPlayers[ClientID]->GetCharacter();
}

void CGameContext::CreateDamageInd(vec2 Pos, float Angle, int Amount, int64_t Mask)
{
	float a = 3 * 3.14159f / 2 + Angle;
	// float a = get_angle(dir);
	float s = a - pi / 3;
	float e = a + pi / 3;
	for (int i = 0; i < Amount; i++)
	{
		float f = mix(s, e, float(i + 1) / float(Amount + 2));
		CNetEvent_DamageInd *pEvent = (CNetEvent_DamageInd *)m_Events.Create(NETEVENTTYPE_DAMAGEIND, sizeof(CNetEvent_DamageInd), Mask);
		if (pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
			pEvent->m_Angle = (int)(f * 256.0f);
		}
	}
}

void CGameContext::CreateHammerHit(vec2 Pos, int64_t Mask)
{
	// create the event
	CNetEvent_HammerHit *pEvent = (CNetEvent_HammerHit *)m_Events.Create(NETEVENTTYPE_HAMMERHIT, sizeof(CNetEvent_HammerHit), Mask);
	if (pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}
}

void CGameContext::CreateExplosion(vec2 Pos, int Owner, int Weapon, bool NoDamage, int64_t Mask)
{
	// create the event
	CNetEvent_Explosion *pEvent = (CNetEvent_Explosion *)m_Events.Create(NETEVENTTYPE_EXPLOSION, sizeof(CNetEvent_Explosion), Mask);
	if (pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}

	if (!NoDamage)
	{
		// deal damage
		CCharacter *apEnts[MAX_CLIENTS];
		float Radius = 135.0f;
		float InnerRadius = 48.0f;
		int Num = m_World.FindEntities(Pos, Radius, (CEntity **)apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
		for (int i = 0; i < Num; i++)
		{
			vec2 Diff = apEnts[i]->m_Pos - Pos;
			vec2 ForceDir(0, 1);
			float l = length(Diff);
			if (l)
				ForceDir = normalize(Diff);
			l = 1 - clamp((l - InnerRadius) / (Radius - InnerRadius), 0.0f, 1.0f);
			float Dmg = 6 * l;
			if ((int)Dmg)
				apEnts[i]->TakeDamage(ForceDir * Dmg * 2, (int)Dmg, Owner, Weapon);
		}
	}
}

/*
void create_smoke(vec2 Pos)
{
	// create the event
	EV_EXPLOSION *pEvent = (EV_EXPLOSION *)events.create(EVENT_SMOKE, sizeof(EV_EXPLOSION));
	if(pEvent)
	{
		pEvent->x = (int)Pos.x;
		pEvent->y = (int)Pos.y;
	}
}*/

void CGameContext::CreatePlayerSpawn(vec2 Pos, int64_t Mask)
{
	// create the event
	CNetEvent_Spawn *ev = (CNetEvent_Spawn *)m_Events.Create(NETEVENTTYPE_SPAWN, sizeof(CNetEvent_Spawn), Mask);
	if (ev)
	{
		ev->m_X = (int)Pos.x;
		ev->m_Y = (int)Pos.y;
	}
}

void CGameContext::CreateDeath(vec2 Pos, int ClientID, int64_t Mask)
{
	// create the event
	CNetEvent_Death *pEvent = (CNetEvent_Death *)m_Events.Create(NETEVENTTYPE_DEATH, sizeof(CNetEvent_Death), Mask);
	if (pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
		pEvent->m_ClientID = ClientID;
	}
}

void CGameContext::CreateSound(vec2 Pos, int Sound, int64_t Mask)
{
	if (Sound < 0)
		return;

	// create a sound
	CNetEvent_SoundWorld *pEvent = (CNetEvent_SoundWorld *)m_Events.Create(NETEVENTTYPE_SOUNDWORLD, sizeof(CNetEvent_SoundWorld), Mask);
	if (pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
		pEvent->m_SoundID = Sound;
	}
}

void CGameContext::CreateSoundGlobal(int Sound, int Target)
{
	if (Sound < 0)
		return;

	CNetMsg_Sv_SoundGlobal Msg;
	Msg.m_SoundID = Sound;
	if (Target == -2)
		Server()->SendPackMsg(&Msg, MSGFLAG_NOSEND, -1);
	else
	{
		int Flag = MSGFLAG_VITAL;
		if (Target != -1)
			Flag |= MSGFLAG_NORECORD;
		Server()->SendPackMsg(&Msg, Flag, Target);
	}
}

void CGameContext::SendChatTarget(int To, const char *pText, ...)
{
	int Start = (To < 0 ? 0 : To);
	int End = (To < 0 ? MAX_CLIENTS : To + 1);

	CNetMsg_Sv_Chat Msg;
	Msg.m_Team = 0;
	Msg.m_ClientID = -1;

	dynamic_string Buffer;

	va_list VarArgs;
	va_start(VarArgs, pText);

	for (int i = Start; i < End; i++)
	{
		if (m_apPlayers[i])
		{
			Buffer.clear();
			Server()->Localization()->Format_VL(Buffer, m_apPlayers[i]->GetLanguage(), pText, VarArgs);

			Msg.m_pMessage = Buffer.buffer();
			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
		}
	}

	va_end(VarArgs);
}

void CGameContext::SendChat(int ChatterClientID, int Team, const char *pText)
{
	char aBuf[256];
	if (ChatterClientID >= 0 && ChatterClientID < MAX_CLIENTS)
		str_format(aBuf, sizeof(aBuf), "%d:%d:%s: %s", ChatterClientID, Team, Server()->ClientName(ChatterClientID), pText);
	else
		str_format(aBuf, sizeof(aBuf), "*** %s", pText);
	Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, Team != CHAT_ALL ? "teamchat" : "chat", aBuf);

	if (Team == CHAT_ALL)
	{
		CNetMsg_Sv_Chat Msg;
		Msg.m_Team = 0;
		Msg.m_ClientID = ChatterClientID;
		Msg.m_pMessage = pText;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
	}
	else
	{
		CNetMsg_Sv_Chat Msg;
		Msg.m_Team = 1;
		Msg.m_ClientID = ChatterClientID;
		Msg.m_pMessage = pText;

		// pack one for the recording only
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NOSEND, -1);

		// send to the clients
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (m_apPlayers[i] && m_apPlayers[i]->GetTeam() == Team)
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, i);
		}
	}
}

void CGameContext::SendEmoticon(int ClientID, int Emoticon)
{
	CNetMsg_Sv_Emoticon Msg;
	Msg.m_ClientID = ClientID;
	Msg.m_Emoticon = Emoticon;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
}

void CGameContext::SendWeaponPickup(int ClientID, int Weapon)
{
	CNetMsg_Sv_WeaponPickup Msg;
	Msg.m_Weapon = Weapon;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::SendBroadcast(const char *pText, int ClientID)
{
	CNetMsg_Sv_Broadcast Msg;
	Msg.m_pMessage = pText;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

//
void CGameContext::StartVote(const char *pDesc, const char *pCommand, const char *pReason)
{
	// check if a vote is already running
	if (m_VoteCloseTime)
		return;

	// reset votes
	m_VoteEnforce = VOTE_ENFORCE_UNKNOWN;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_apPlayers[i])
		{
			m_apPlayers[i]->m_Vote = 0;
			m_apPlayers[i]->m_VotePos = 0;
		}
	}

	// start vote
	m_VoteCloseTime = time_get() + time_freq() * 25;
	str_copy(m_aVoteDescription, pDesc, sizeof(m_aVoteDescription));
	str_copy(m_aVoteCommand, pCommand, sizeof(m_aVoteCommand));
	str_copy(m_aVoteReason, pReason, sizeof(m_aVoteReason));
	SendVoteSet(-1);
	m_VoteUpdate = true;
}

void CGameContext::EndVote()
{
	m_VoteCloseTime = 0;
	SendVoteSet(-1);
}

void CGameContext::SendVoteSet(int ClientID)
{
	CNetMsg_Sv_VoteSet Msg;
	if (m_VoteCloseTime)
	{
		Msg.m_Timeout = (m_VoteCloseTime - time_get()) / time_freq();
		Msg.m_pDescription = m_aVoteDescription;
		Msg.m_pReason = m_aVoteReason;
	}
	else
	{
		Msg.m_Timeout = 0;
		Msg.m_pDescription = "";
		Msg.m_pReason = "";
	}
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::SendVoteStatus(int ClientID, int Total, int Yes, int No)
{
	CNetMsg_Sv_VoteStatus Msg = {0};
	Msg.m_Total = Total;
	Msg.m_Yes = Yes;
	Msg.m_No = No;
	Msg.m_Pass = Total - (Yes + No);

	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::AbortVoteKickOnDisconnect(int ClientID)
{
	if (m_VoteCloseTime && ((!str_comp_num(m_aVoteCommand, "kick ", 5) && str_toint(&m_aVoteCommand[5]) == ClientID) ||
							(!str_comp_num(m_aVoteCommand, "set_team ", 9) && str_toint(&m_aVoteCommand[9]) == ClientID)))
		m_VoteCloseTime = -1;
}

void CGameContext::CheckPureTuning()
{
	// might not be created yet during start up
	if (!m_pController)
		return;
}

void CGameContext::SendTuningParams(int ClientID)
{
	CheckPureTuning();

	CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
	int *pParams = (int *)&m_Tuning;
	for (unsigned i = 0; i < sizeof(m_Tuning) / sizeof(int); i++)
		Msg.AddInt(pParams[i]);
	Server()->SendMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::SwapTeams()
{
	if (!m_pController->IsTeamplay())
		return;

	SendChat(-1, CGameContext::CHAT_ALL, "Teams were swapped");

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (m_apPlayers[i] && m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			m_apPlayers[i]->SetTeam(m_apPlayers[i]->GetTeam() ^ 1, false);
	}

	(void)m_pController->CheckTeamBalance();
}

void CGameContext::OnTick()
{
	// check tuning
	CheckPureTuning();

	// Check bot number
	CheckBotNumber();

	// Test basic move for bots
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (!m_apPlayers[i] || !m_apPlayers[i]->m_IsBot)
			continue;
		CNetObj_PlayerInput Input = m_apPlayers[i]->m_pBot->GetLastInputData();
		m_apPlayers[i]->OnPredictedInput(&Input);
	}

	// copy tuning
	m_World.m_Core.m_Tuning = m_Tuning;
	m_World.Tick();

	// if(world.paused) // make sure that the game object always updates
	m_pController->Tick();

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_apPlayers[i])
		{
			m_apPlayers[i]->Tick();
			m_apPlayers[i]->PostTick();
		}
	}

	// update voting
	if (m_VoteCloseTime)
	{
		// abort the kick-vote on player-leave
		if (m_VoteCloseTime == -1)
		{
			SendChat(-1, CGameContext::CHAT_ALL, "Vote aborted");
			EndVote();
		}
		else
		{
			int Total = 0, Yes = 0, No = 0;
			if (m_VoteUpdate)
			{
				// count votes
				char aaBuf[MAX_CLIENTS][NETADDR_MAXSTRSIZE] = {{0}};
				for (int i = 0; i < MAX_CLIENTS; i++)
					if (m_apPlayers[i])
						Server()->GetClientAddr(i, aaBuf[i], NETADDR_MAXSTRSIZE);
				bool aVoteChecked[MAX_CLIENTS] = {0};
				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					if (!m_apPlayers[i] || m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS || aVoteChecked[i] || m_apPlayers[i]->m_IsBot) // don't count in votes by spectators
						continue;

					int ActVote = m_apPlayers[i]->m_Vote;
					int ActVotePos = m_apPlayers[i]->m_VotePos;

					// check for more players with the same ip (only use the vote of the one who voted first)
					for (int j = i + 1; j < MAX_CLIENTS; ++j)
					{
						if (!m_apPlayers[j] || aVoteChecked[j] || str_comp(aaBuf[j], aaBuf[i]))
							continue;

						aVoteChecked[j] = true;
						if (m_apPlayers[j]->m_Vote && (!ActVote || ActVotePos > m_apPlayers[j]->m_VotePos))
						{
							ActVote = m_apPlayers[j]->m_Vote;
							ActVotePos = m_apPlayers[j]->m_VotePos;
						}
					}

					Total++;
					if (ActVote > 0)
						Yes++;
					else if (ActVote < 0)
						No++;
				}

				if (Yes >= Total / 2 + 1)
					m_VoteEnforce = VOTE_ENFORCE_YES;
				else if (No >= (Total + 1) / 2)
					m_VoteEnforce = VOTE_ENFORCE_NO;
			}

			if (m_VoteEnforce == VOTE_ENFORCE_YES)
			{
				Server()->SetRconCID(IServer::RCON_CID_VOTE);
				Console()->ExecuteLine(m_aVoteCommand, -1);
				Server()->SetRconCID(IServer::RCON_CID_SERV);
				EndVote();
				SendChat(-1, CGameContext::CHAT_ALL, "Vote passed");

				if (m_apPlayers[m_VoteCreator])
					m_apPlayers[m_VoteCreator]->m_LastVoteCall = 0;
			}
			else if (m_VoteEnforce == VOTE_ENFORCE_NO || time_get() > m_VoteCloseTime)
			{
				EndVote();
				SendChat(-1, CGameContext::CHAT_ALL, "Vote failed");
			}
			else if (m_VoteUpdate)
			{
				m_VoteUpdate = false;
				SendVoteStatus(-1, Total, Yes, No);
			}
		}
	}
	// Test basic move for bots
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (!m_apPlayers[i] || !m_apPlayers[i]->m_IsBot)
			continue;
		CNetObj_PlayerInput Input = m_apPlayers[i]->m_pBot->GetInputData();
		m_apPlayers[i]->OnDirectInput(&Input);
	}
}

// Server hooks
void CGameContext::OnClientDirectInput(int ClientID, void *pInput)
{
	if (!m_World.m_Paused)
		m_apPlayers[ClientID]->OnDirectInput((CNetObj_PlayerInput *)pInput);
}

void CGameContext::OnClientPredictedInput(int ClientID, void *pInput)
{
	if (!m_World.m_Paused)
		m_apPlayers[ClientID]->OnPredictedInput((CNetObj_PlayerInput *)pInput);
}

void CGameContext::OnClientEnter(int ClientID)
{
	// world.insert_entity(&players[client_id]);
	m_apPlayers[ClientID]->Respawn();
	char aBuf[512];

	SendChatTarget(ClientID, _("服务器：TeeFun"));
	SendChatTarget(ClientID, _("当前版本没有什么玩法，可以当成电子斗蛐蛐，只是测试一下AI会不会导致服务器崩溃（64个AI同时)"));
	SendChatTarget(ClientID, _("投票里有一些属性，但是要注册账号才能看到，想要改宗门改修炼体质的联系管理员"));
	SendChatTarget(ClientID, _("模式世界观为道诡异仙小说的BE结局（纯开发者自己瞎即把扯的）"));
	SendChatTarget(ClientID, _("服主QQ：1562151175  服务器交流群：895105949"));
	SendChatTarget(ClientID, _("——————————————————————————"));
	SendChatTarget(ClientID, _("按F1看全部内容"));
	SendChatTarget(ClientID, _("——————————————————————————"));
	SendChatTarget(-1, _("魂体 '{str:p}' 共鸣进了此方小世界"), "p", Server()->ClientName(ClientID));

	SendChatTarget(ClientID, _("使用指令 /register <肉体之名> <肉体之匙> 来创造你在异仙界的血肉体(不用加上 '<' 和 '>' )"));

	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' team=%d", ClientID, Server()->ClientName(ClientID), m_apPlayers[ClientID]->GetTeam());
	Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	m_VoteUpdate = true;
}

void CGameContext::OnClientConnected(int ClientID, bool AI)
{
	// Check if the slot is used by a bot
	if (m_apPlayers[ClientID] && m_apPlayers[ClientID]->m_IsBot)
	{
		delete m_apPlayers[ClientID];
		m_apPlayers[ClientID] = 0;
	}

	if (AI)
		m_apPlayers[ClientID] = new (ClientID) CPlayer(this, ClientID, 10);
	else
		m_apPlayers[ClientID] = new (ClientID) CPlayer(this, ClientID, BOTTYPE_PLAYER);

	m_apPlayers[ClientID]->m_IsBot = AI;

	(void)m_pController->CheckTeamBalance();

	CheckBotNumber();

#ifdef CONF_DEBUG
	if (g_Config.m_DbgDummies)
	{
		if (ClientID >= MAX_CLIENTS - g_Config.m_DbgDummies)
			return;
	}
#endif

	// send active vote
	if (m_VoteCloseTime)
		SendVoteSet(ClientID);

	// send motd
	CNetMsg_Sv_Motd Msg;
	Msg.m_pMessage = g_Config.m_SvMotd;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::OnClientDrop(int ClientID, const char *pReason)
{
	AbortVoteKickOnDisconnect(ClientID);

#ifdef CONF_SQLITE
	if (m_apPlayers[ClientID]->m_AccData.m_UserID)
		m_apPlayers[ClientID]->m_pAccount->Apply();
#endif
#ifdef CONF_SQL
	Apply(ClientID);
#endif
	m_apPlayers[ClientID]->OnDisconnect(pReason);
	delete m_apPlayers[ClientID];
	m_apPlayers[ClientID] = 0;

	(void)m_pController->CheckTeamBalance();
	m_VoteUpdate = true;

	// update spectator modes
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (m_apPlayers[i] && m_apPlayers[i]->m_SpectatorID == ClientID)
			m_apPlayers[i]->m_SpectatorID = SPEC_FREEVIEW;
	}
}

void CGameContext::OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{
	void *pRawMsg = m_NetObjHandler.SecureUnpackMsg(MsgID, pUnpacker);
	CPlayer *pPlayer = m_apPlayers[ClientID];

	if (!pRawMsg)
	{
		if (g_Config.m_Debug)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "dropped weird message '%s' (%d), failed on '%s'", m_NetObjHandler.GetMsgName(MsgID), MsgID, m_NetObjHandler.FailedMsgOn());
			Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "server", aBuf);
		}
		return;
	}

	if (Server()->ClientIngame(ClientID))
	{
		if (MsgID == NETMSGTYPE_CL_SAY)
		{
			if (g_Config.m_SvSpamprotection && pPlayer->m_LastChat && pPlayer->m_LastChat + Server()->TickSpeed() > Server()->Tick())
				return;

			CNetMsg_Cl_Say *pMsg = (CNetMsg_Cl_Say *)pRawMsg;
			if (!str_utf8_check(pMsg->m_pMessage))
			{
				return;
			}
			int Team = pMsg->m_Team ? pPlayer->GetTeam() : CGameContext::CHAT_ALL;

			// trim right and set maximum length to 128 utf8-characters
			int Length = 0;
			const char *p = pMsg->m_pMessage;
			const char *pEnd = 0;
			while (*p)
			{
				const char *pStrOld = p;
				int Code = str_utf8_decode(&p);

				// check if unicode is not empty
				if (Code > 0x20 && Code != 0xA0 && Code != 0x034F && (Code < 0x2000 || Code > 0x200F) && (Code < 0x2028 || Code > 0x202F) &&
					(Code < 0x205F || Code > 0x2064) && (Code < 0x206A || Code > 0x206F) && (Code < 0xFE00 || Code > 0xFE0F) &&
					Code != 0xFEFF && (Code < 0xFFF9 || Code > 0xFFFC))
				{
					pEnd = 0;
				}
				else if (pEnd == 0)
					pEnd = pStrOld;

				if (++Length >= 127)
				{
					*(const_cast<char *>(p)) = 0;
					break;
				}
			}
			if (pEnd != 0)
				*(const_cast<char *>(pEnd)) = 0;

			// drop empty and autocreated spam messages (more than 16 characters per second)
			if (Length == 0 || (pMsg->m_pMessage[0] != '/' && g_Config.m_SvSpamprotection && pPlayer->m_LastChat && pPlayer->m_LastChat + Server()->TickSpeed() * ((15 + Length) / 16) > Server()->Tick()))
				return;

			pPlayer->m_LastChat = Server()->Tick();

			if (pMsg->m_pMessage[0] == '/' || pMsg->m_pMessage[0] == '\\')
			{
				switch (m_apPlayers[ClientID]->m_Authed)
				{
				case IServer::AUTHED_ADMIN:
					Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_ADMIN);
					break;
				case IServer::AUTHED_MOD:
					Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_MOD);
					break;
				default:
					Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_USER);
				}

				Console()->ExecuteLineFlag(pMsg->m_pMessage + 1, ClientID, CFGFLAG_CHAT);

				Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_ADMIN);
			}
			else
			{
				SendChat(ClientID, Team, pMsg->m_pMessage);
			}
		}
		else if (MsgID == NETMSGTYPE_CL_CALLVOTE)
		{
			if (g_Config.m_SvSpamprotection && pPlayer->m_LastVoteTry && pPlayer->m_LastVoteTry + Server()->TickSpeed() * 3 > Server()->Tick())
				return;

			int64 Now = Server()->Tick();
			pPlayer->m_LastVoteTry = Now;
			if (pPlayer->GetTeam() == TEAM_SPECTATORS)
			{
				SendChatTarget(ClientID, "Spectators aren't allowed to start a vote.");
				return;
			}

			if (m_VoteCloseTime)
			{
				SendChatTarget(ClientID, "Wait for current vote to end before calling a new one.");
				return;
			}

			int Timeleft = pPlayer->m_LastVoteCall + Server()->TickSpeed() * 60 - Now;
			if (pPlayer->m_LastVoteCall && Timeleft > 0)
			{
				char aChatmsg[512] = {0};
				str_format(aChatmsg, sizeof(aChatmsg), "You must wait %d seconds before making another vote", (Timeleft / Server()->TickSpeed()) + 1);
				SendChatTarget(ClientID, aChatmsg);
				return;
			}

			char aChatmsg[512] = {0};
			char aDesc[VOTE_DESC_LENGTH] = {0};
			char aCmd[VOTE_CMD_LENGTH] = {0};
			CNetMsg_Cl_CallVote *pMsg = (CNetMsg_Cl_CallVote *)pRawMsg;
			const char *pReason = pMsg->m_Reason[0] ? pMsg->m_Reason : "No reason given";

			if (str_comp_nocase(pMsg->m_Type, "option") == 0)
			{
				for (int i = 0; i < m_PlayerVotes[ClientID].size(); ++i)
				{
					if (str_comp_nocase(pMsg->m_Value, m_PlayerVotes[ClientID][i].m_aDescription) == 0)
					{
						str_format(aDesc, sizeof(aDesc), "%s", m_PlayerVotes[ClientID][i].m_aDescription);
						str_format(aCmd, sizeof(aCmd), "%s", m_PlayerVotes[ClientID][i].m_aCommand);
					}
				}
			}
			else if (str_comp_nocase(pMsg->m_Type, "kick") == 0)
			{
				if (!g_Config.m_SvVoteKick)
				{
					SendChatTarget(ClientID, "Server does not allow voting to kick players");
					return;
				}

				if (g_Config.m_SvVoteKickMin)
				{
					int PlayerNum = 0;
					for (int i = 0; i < MAX_CLIENTS; ++i)
						if (m_apPlayers[i] && m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
							++PlayerNum;

					if (PlayerNum < g_Config.m_SvVoteKickMin)
					{
						str_format(aChatmsg, sizeof(aChatmsg), "Kick voting requires %d players on the server", g_Config.m_SvVoteKickMin);
						SendChatTarget(ClientID, aChatmsg);
						return;
					}
				}

				int KickID = str_toint(pMsg->m_Value);
				if (KickID < 0 || KickID >= MAX_CLIENTS || !m_apPlayers[KickID])
				{
					SendChatTarget(ClientID, "Invalid client id to kick");
					return;
				}
				if (KickID == ClientID)
				{
					SendChatTarget(ClientID, "You can't kick yourself");
					return;
				}

				if (!Server()->ReverseTranslate(KickID, ClientID))
					return;

				if (Server()->IsAuthed(KickID))
				{
					SendChatTarget(ClientID, "You can't kick admins");
					char aBufKick[128];
					str_format(aBufKick, sizeof(aBufKick), "'%s' called for vote to kick you", Server()->ClientName(ClientID));
					SendChatTarget(KickID, aBufKick);
					return;
				}

				str_format(aChatmsg, sizeof(aChatmsg), "'%s' called for vote to kick '%s' (%s)", Server()->ClientName(ClientID), Server()->ClientName(KickID), pReason);
				str_format(aDesc, sizeof(aDesc), "Kick '%s'", Server()->ClientName(KickID));
				if (!g_Config.m_SvVoteKickBantime)
					str_format(aCmd, sizeof(aCmd), "kick %d Kicked by vote", KickID);
				else
				{
					char aAddrStr[NETADDR_MAXSTRSIZE] = {0};
					Server()->GetClientAddr(KickID, aAddrStr, sizeof(aAddrStr));
					str_format(aCmd, sizeof(aCmd), "ban %s %d Banned by vote", aAddrStr, g_Config.m_SvVoteKickBantime);
				}
			}
			else if (str_comp_nocase(pMsg->m_Type, "spectate") == 0)
			{
				if (!g_Config.m_SvVoteSpectate)
				{
					SendChatTarget(ClientID, "Server does not allow voting to move players to spectators");
					return;
				}

				int SpectateID = str_toint(pMsg->m_Value);
				if (SpectateID < 0 || SpectateID >= MAX_CLIENTS || !m_apPlayers[SpectateID] || m_apPlayers[SpectateID]->GetTeam() == TEAM_SPECTATORS)
				{
					SendChatTarget(ClientID, "Invalid client id to move");
					return;
				}
				if (SpectateID == ClientID)
				{
					SendChatTarget(ClientID, "You can't move yourself");
					return;
				}

				if (!Server()->ReverseTranslate(SpectateID, ClientID))
					return;

				str_format(aChatmsg, sizeof(aChatmsg), "'%s' called for vote to move '%s' to spectators (%s)", Server()->ClientName(ClientID), Server()->ClientName(SpectateID), pReason);
				str_format(aDesc, sizeof(aDesc), "move '%s' to spectators", Server()->ClientName(SpectateID));
				str_format(aCmd, sizeof(aCmd), "set_team %d -1 %d", SpectateID, g_Config.m_SvVoteSpectateRejoindelay);
			}

			if (aCmd[0])
			{
				SendChat(-1, CGameContext::CHAT_ALL, aChatmsg);
				StartVote(aDesc, aCmd, pReason);
				pPlayer->m_Vote = 1;
				pPlayer->m_VotePos = m_VotePos = 1;
				m_VoteCreator = ClientID;
				pPlayer->m_LastVoteCall = Now;
			}
		}
		else if (MsgID == NETMSGTYPE_CL_VOTE)
		{
			if (!m_VoteCloseTime)
				return;

			if (pPlayer->m_Vote == 0)
			{
				CNetMsg_Cl_Vote *pMsg = (CNetMsg_Cl_Vote *)pRawMsg;
				if (!pMsg->m_Vote)
					return;

				pPlayer->m_Vote = pMsg->m_Vote;
				pPlayer->m_VotePos = ++m_VotePos;
				m_VoteUpdate = true;
			}
		}
		else if (MsgID == NETMSGTYPE_CL_SETTEAM && !m_World.m_Paused)
		{
			CNetMsg_Cl_SetTeam *pMsg = (CNetMsg_Cl_SetTeam *)pRawMsg;

			if (pPlayer->GetTeam() == pMsg->m_Team || (g_Config.m_SvSpamprotection && pPlayer->m_LastSetTeam && pPlayer->m_LastSetTeam + Server()->TickSpeed() * 3 > Server()->Tick()))
				return;

			if (pMsg->m_Team != TEAM_SPECTATORS && m_LockTeams)
			{
				pPlayer->m_LastSetTeam = Server()->Tick();
				SendBroadcast("Teams are locked", ClientID);
				return;
			}

			if (pPlayer->m_TeamChangeTick > Server()->Tick())
			{
				pPlayer->m_LastSetTeam = Server()->Tick();
				int TimeLeft = (pPlayer->m_TeamChangeTick - Server()->Tick()) / Server()->TickSpeed();
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "Time to wait before changing team: %02d:%02d", TimeLeft / 60, TimeLeft % 60);
				SendBroadcast(aBuf, ClientID);
				return;
			}

			// Switch team on given client and kill/respawn him
			if (m_pController->CanJoinTeam(pMsg->m_Team, ClientID))
			{
				if (m_pController->CanChangeTeam(pPlayer, pMsg->m_Team))
				{
					pPlayer->m_LastSetTeam = Server()->Tick();
					if (pPlayer->GetTeam() == TEAM_SPECTATORS || pMsg->m_Team == TEAM_SPECTATORS)
						m_VoteUpdate = true;
					pPlayer->SetTeam(pMsg->m_Team);
					(void)m_pController->CheckTeamBalance();
					pPlayer->m_TeamChangeTick = Server()->Tick();
				}
				else
					SendBroadcast("Teams must be balanced, please join other team", ClientID);
			}
			else
			{
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "Only %d active players are allowed", Server()->MaxClients() - g_Config.m_SvSpectatorSlots);
				SendBroadcast(aBuf, ClientID);
			}
		}
		else if (MsgID == NETMSGTYPE_CL_SETSPECTATORMODE && !m_World.m_Paused)
		{
			CNetMsg_Cl_SetSpectatorMode *pMsg = (CNetMsg_Cl_SetSpectatorMode *)pRawMsg;

			if (g_Config.m_SvSpamprotection && pPlayer->m_LastSetSpectatorMode && pPlayer->m_LastSetSpectatorMode + Server()->TickSpeed() * 3 > Server()->Tick())
				return;

			if (pMsg->m_SpectatorID != SPEC_FREEVIEW)
				if (!Server()->ReverseTranslate(pMsg->m_SpectatorID, ClientID))
					return;

			if (pPlayer->GetTeam() != TEAM_SPECTATORS || pPlayer->m_SpectatorID == pMsg->m_SpectatorID || ClientID == pMsg->m_SpectatorID)
				return;

			pPlayer->m_LastSetSpectatorMode = Server()->Tick();
			if (pMsg->m_SpectatorID != SPEC_FREEVIEW && (!m_apPlayers[pMsg->m_SpectatorID] || m_apPlayers[pMsg->m_SpectatorID]->GetTeam() == TEAM_SPECTATORS))
				SendChatTarget(ClientID, "Invalid spectator id used");
			else
				pPlayer->m_SpectatorID = pMsg->m_SpectatorID;
		}
		else if (MsgID == NETMSGTYPE_CL_CHANGEINFO)
		{
			if (g_Config.m_SvSpamprotection && pPlayer->m_LastChangeInfo && pPlayer->m_LastChangeInfo + Server()->TickSpeed() * 5 > Server()->Tick())
				return;

			CNetMsg_Cl_ChangeInfo *pMsg = (CNetMsg_Cl_ChangeInfo *)pRawMsg;
			pPlayer->m_LastChangeInfo = Server()->Tick();

			// set infos
			char aOldName[MAX_NAME_LENGTH];
			str_copy(aOldName, Server()->ClientName(ClientID), sizeof(aOldName));
			Server()->SetClientName(ClientID, pMsg->m_pName);
			if (str_comp(aOldName, Server()->ClientName(ClientID)) != 0)
			{
				char aChatText[256];
				str_format(aChatText, sizeof(aChatText), "'%s' changed name to '%s'", aOldName, Server()->ClientName(ClientID));
				SendChat(-1, CGameContext::CHAT_ALL, aChatText);
			}
			Server()->SetClientClan(ClientID, pMsg->m_pClan);
			Server()->SetClientCountry(ClientID, pMsg->m_Country);
			str_copy(pPlayer->m_TeeInfos.m_SkinName, pMsg->m_pSkin, sizeof(pPlayer->m_TeeInfos.m_SkinName));
			pPlayer->m_TeeInfos.m_UseCustomColor = true;
			pPlayer->m_TeeInfos.m_ColorBody = 65280;
			pPlayer->m_TeeInfos.m_ColorFeet = 65280;
			m_pController->OnPlayerInfoChange(pPlayer);
		}
		else if (MsgID == NETMSGTYPE_CL_EMOTICON && !m_World.m_Paused)
		{
			CNetMsg_Cl_Emoticon *pMsg = (CNetMsg_Cl_Emoticon *)pRawMsg;

			if (g_Config.m_SvSpamprotection && pPlayer->m_LastEmote && pPlayer->m_LastEmote + Server()->TickSpeed() * 3 > Server()->Tick())
				return;

			pPlayer->m_LastEmote = Server()->Tick();

			SendEmoticon(ClientID, pMsg->m_Emoticon);
		}
		else if (MsgID == NETMSGTYPE_CL_KILL && !m_World.m_Paused)
		{
			if (pPlayer->m_LastKill && pPlayer->m_LastKill + Server()->TickSpeed() * 3 > Server()->Tick())
				return;

			pPlayer->m_LastKill = Server()->Tick();
			pPlayer->KillCharacter(WEAPON_SELF);
		}
	}
	else
	{
		if (MsgID == NETMSGTYPE_CL_STARTINFO)
		{
			if (pPlayer->m_IsReady)
				return;

			CNetMsg_Cl_StartInfo *pMsg = (CNetMsg_Cl_StartInfo *)pRawMsg;
			pPlayer->m_LastChangeInfo = Server()->Tick();

			// set start infos
			Server()->SetClientName(ClientID, pMsg->m_pName);
			Server()->SetClientClan(ClientID, pMsg->m_pClan);
			Server()->SetClientCountry(ClientID, pMsg->m_Country);
			str_copy(pPlayer->m_TeeInfos.m_SkinName, pMsg->m_pSkin, sizeof(pPlayer->m_TeeInfos.m_SkinName));
			pPlayer->m_TeeInfos.m_UseCustomColor = true;
			pPlayer->m_TeeInfos.m_ColorBody = 65280;
			pPlayer->m_TeeInfos.m_ColorFeet = 65280;
			m_pController->OnPlayerInfoChange(pPlayer);

			// send vote options
			CNetMsg_Sv_VoteClearOptions ClearMsg;
			Server()->SendPackMsg(&ClearMsg, MSGFLAG_VITAL, ClientID);

			CNetMsg_Sv_VoteOptionListAdd OptionMsg;
			int NumOptions = 0;
			OptionMsg.m_pDescription0 = "";
			OptionMsg.m_pDescription1 = "";
			OptionMsg.m_pDescription2 = "";
			OptionMsg.m_pDescription3 = "";
			OptionMsg.m_pDescription4 = "";
			OptionMsg.m_pDescription5 = "";
			OptionMsg.m_pDescription6 = "";
			OptionMsg.m_pDescription7 = "";
			OptionMsg.m_pDescription8 = "";
			OptionMsg.m_pDescription9 = "";
			OptionMsg.m_pDescription10 = "";
			OptionMsg.m_pDescription11 = "";
			OptionMsg.m_pDescription12 = "";
			OptionMsg.m_pDescription13 = "";
			OptionMsg.m_pDescription14 = "";
			CVoteOptionServer *pCurrent = m_pVoteOptionFirst;
			while (pCurrent)
			{
				switch (NumOptions++)
				{
				case 0:
					OptionMsg.m_pDescription0 = pCurrent->m_aDescription;
					break;
				case 1:
					OptionMsg.m_pDescription1 = pCurrent->m_aDescription;
					break;
				case 2:
					OptionMsg.m_pDescription2 = pCurrent->m_aDescription;
					break;
				case 3:
					OptionMsg.m_pDescription3 = pCurrent->m_aDescription;
					break;
				case 4:
					OptionMsg.m_pDescription4 = pCurrent->m_aDescription;
					break;
				case 5:
					OptionMsg.m_pDescription5 = pCurrent->m_aDescription;
					break;
				case 6:
					OptionMsg.m_pDescription6 = pCurrent->m_aDescription;
					break;
				case 7:
					OptionMsg.m_pDescription7 = pCurrent->m_aDescription;
					break;
				case 8:
					OptionMsg.m_pDescription8 = pCurrent->m_aDescription;
					break;
				case 9:
					OptionMsg.m_pDescription9 = pCurrent->m_aDescription;
					break;
				case 10:
					OptionMsg.m_pDescription10 = pCurrent->m_aDescription;
					break;
				case 11:
					OptionMsg.m_pDescription11 = pCurrent->m_aDescription;
					break;
				case 12:
					OptionMsg.m_pDescription12 = pCurrent->m_aDescription;
					break;
				case 13:
					OptionMsg.m_pDescription13 = pCurrent->m_aDescription;
					break;
				case 14:
				{
					OptionMsg.m_pDescription14 = pCurrent->m_aDescription;
					OptionMsg.m_NumOptions = NumOptions;
					Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);
					OptionMsg = CNetMsg_Sv_VoteOptionListAdd();
					NumOptions = 0;
					OptionMsg.m_pDescription1 = "";
					OptionMsg.m_pDescription2 = "";
					OptionMsg.m_pDescription3 = "";
					OptionMsg.m_pDescription4 = "";
					OptionMsg.m_pDescription5 = "";
					OptionMsg.m_pDescription6 = "";
					OptionMsg.m_pDescription7 = "";
					OptionMsg.m_pDescription8 = "";
					OptionMsg.m_pDescription9 = "";
					OptionMsg.m_pDescription10 = "";
					OptionMsg.m_pDescription11 = "";
					OptionMsg.m_pDescription12 = "";
					OptionMsg.m_pDescription13 = "";
					OptionMsg.m_pDescription14 = "";
				}
				}
				pCurrent = pCurrent->m_pNext;
			}
			if (NumOptions > 0)
			{
				OptionMsg.m_NumOptions = NumOptions;
				Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);
			}

			// send tuning parameters to client
			SendTuningParams(ClientID);

			// client is ready to enter
			pPlayer->m_IsReady = true;
			CNetMsg_Sv_ReadyToEnter m;
			Server()->SendPackMsg(&m, MSGFLAG_VITAL | MSGFLAG_FLUSH, ClientID);
		}
	}
}

void CGameContext::ConTuneParam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pParamName = pResult->GetString(0);
	float NewValue = pResult->GetFloat(1);

	if (pSelf->Tuning()->Set(pParamName, NewValue))
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "%s changed to %.2f", pParamName, NewValue);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", aBuf);
		pSelf->SendTuningParams(-1);
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", "No such tuning parameter");
}

void CGameContext::ConTuneReset(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CTuningParams TuningParams;
	*pSelf->Tuning() = TuningParams;
	pSelf->SendTuningParams(-1);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", "Tuning reset");
}

void CGameContext::ConTuneDump(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	char aBuf[256];
	for (int i = 0; i < pSelf->Tuning()->Num(); i++)
	{
		float v;
		pSelf->Tuning()->Get(i, &v);
		str_format(aBuf, sizeof(aBuf), "%s %.2f", pSelf->Tuning()->m_apNames[i], v);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", aBuf);
	}
}

void CGameContext::ConPause(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_pController->TogglePause();
}

void CGameContext::ConChangeMap(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_pController->ChangeMap(pResult->NumArguments() ? pResult->GetString(0) : "");
}

void CGameContext::ConRestart(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (pResult->NumArguments())
		pSelf->m_pController->DoWarmup(pResult->GetInteger(0));
	else
		pSelf->m_pController->StartRound();
}

void CGameContext::ConBroadcast(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SendBroadcast(pResult->GetString(0), -1);
}

void CGameContext::ConSay(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SendChat(-1, CGameContext::CHAT_ALL, pResult->GetString(0));
}

void CGameContext::ConSetTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS - 1);
	int Team = clamp(pResult->GetInteger(1), -1, 1);
	int Delay = pResult->NumArguments() > 2 ? pResult->GetInteger(2) : 0;
	if (!pSelf->m_apPlayers[ClientID])
		return;

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "moved client %d to team %d", ClientID, Team);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	pSelf->m_apPlayers[ClientID]->m_TeamChangeTick = pSelf->Server()->Tick() + pSelf->Server()->TickSpeed() * Delay * 60;
	pSelf->m_apPlayers[ClientID]->SetTeam(Team);
	(void)pSelf->m_pController->CheckTeamBalance();
}

void CGameContext::ConSetTeamAll(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Team = clamp(pResult->GetInteger(0), -1, 1);

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "All players were moved to the %s", pSelf->m_pController->GetTeamName(Team));
	pSelf->SendChat(-1, CGameContext::CHAT_ALL, aBuf);

	for (int i = 0; i < MAX_CLIENTS; ++i)
		if (pSelf->m_apPlayers[i])
			pSelf->m_apPlayers[i]->SetTeam(Team, false);

	(void)pSelf->m_pController->CheckTeamBalance();
}

void CGameContext::ConSwapTeams(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SwapTeams();
}

void CGameContext::ConShuffleTeams(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!pSelf->m_pController->IsTeamplay())
		return;

	int CounterRed = 0;
	int CounterBlue = 0;
	int PlayerTeam = 0;
	for (int i = 0; i < MAX_CLIENTS; ++i)
		if (pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			++PlayerTeam;
	PlayerTeam = (PlayerTeam + 1) / 2;

	pSelf->SendChat(-1, CGameContext::CHAT_ALL, "Teams were shuffled");

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
		{
			if (CounterRed == PlayerTeam)
				pSelf->m_apPlayers[i]->SetTeam(TEAM_BLUE, false);
			else if (CounterBlue == PlayerTeam)
				pSelf->m_apPlayers[i]->SetTeam(TEAM_RED, false);
			else
			{
				if (rand() % 2)
				{
					pSelf->m_apPlayers[i]->SetTeam(TEAM_BLUE, false);
					++CounterBlue;
				}
				else
				{
					pSelf->m_apPlayers[i]->SetTeam(TEAM_RED, false);
					++CounterRed;
				}
			}
		}
	}

	(void)pSelf->m_pController->CheckTeamBalance();
}

void CGameContext::ConLockTeams(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_LockTeams ^= 1;
	if (pSelf->m_LockTeams)
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "Teams were locked");
	else
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, "Teams were unlocked");
}

void CGameContext::ConAddVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pDescription = pResult->GetString(0);
	const char *pCommand = pResult->GetString(1);

	if (pSelf->m_NumVoteOptions == MAX_VOTE_OPTIONS)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "maximum number of vote options reached");
		return;
	}

	// check for valid option
	if (!pSelf->Console()->LineIsValid(pCommand) || str_length(pCommand) >= VOTE_CMD_LENGTH)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "skipped invalid command '%s'", pCommand);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}
	while (*pDescription && *pDescription == ' ')
		pDescription++;
	if (str_length(pDescription) >= VOTE_DESC_LENGTH || *pDescription == 0)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "skipped invalid option '%s'", pDescription);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	// check for duplicate entry
	CVoteOptionServer *pOption = pSelf->m_pVoteOptionFirst;
	while (pOption)
	{
		if (str_comp_nocase(pDescription, pOption->m_aDescription) == 0)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "option '%s' already exists", pDescription);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
			return;
		}
		pOption = pOption->m_pNext;
	}

	// add the option
	++pSelf->m_NumVoteOptions;
	int Len = str_length(pCommand);

	pOption = (CVoteOptionServer *)pSelf->m_pVoteOptionHeap->Allocate(sizeof(CVoteOptionServer) + Len);
	pOption->m_pNext = 0;
	pOption->m_pPrev = pSelf->m_pVoteOptionLast;
	if (pOption->m_pPrev)
		pOption->m_pPrev->m_pNext = pOption;
	pSelf->m_pVoteOptionLast = pOption;
	if (!pSelf->m_pVoteOptionFirst)
		pSelf->m_pVoteOptionFirst = pOption;

	str_copy(pOption->m_aDescription, pDescription, sizeof(pOption->m_aDescription));
	mem_copy(pOption->m_aCommand, pCommand, Len + 1);
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "added option '%s' '%s'", pOption->m_aDescription, pOption->m_aCommand);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	// inform clients about added option
	CNetMsg_Sv_VoteOptionAdd OptionMsg;
	OptionMsg.m_pDescription = pOption->m_aDescription;
	pSelf->Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, -1);
}

void CGameContext::ConRemoveVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pDescription = pResult->GetString(0);

	// check for valid option
	CVoteOptionServer *pOption = pSelf->m_pVoteOptionFirst;
	while (pOption)
	{
		if (str_comp_nocase(pDescription, pOption->m_aDescription) == 0)
			break;
		pOption = pOption->m_pNext;
	}
	if (!pOption)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "option '%s' does not exist", pDescription);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	// inform clients about removed option
	CNetMsg_Sv_VoteOptionRemove OptionMsg;
	OptionMsg.m_pDescription = pOption->m_aDescription;
	pSelf->Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, -1);

	// TODO: improve this
	// remove the option
	--pSelf->m_NumVoteOptions;
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "removed option '%s' '%s'", pOption->m_aDescription, pOption->m_aCommand);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	CHeap *pVoteOptionHeap = new CHeap();
	CVoteOptionServer *pVoteOptionFirst = 0;
	CVoteOptionServer *pVoteOptionLast = 0;
	int NumVoteOptions = pSelf->m_NumVoteOptions;
	for (CVoteOptionServer *pSrc = pSelf->m_pVoteOptionFirst; pSrc; pSrc = pSrc->m_pNext)
	{
		if (pSrc == pOption)
			continue;

		// copy option
		int Len = str_length(pSrc->m_aCommand);
		CVoteOptionServer *pDst = (CVoteOptionServer *)pVoteOptionHeap->Allocate(sizeof(CVoteOptionServer) + Len);
		pDst->m_pNext = 0;
		pDst->m_pPrev = pVoteOptionLast;
		if (pDst->m_pPrev)
			pDst->m_pPrev->m_pNext = pDst;
		pVoteOptionLast = pDst;
		if (!pVoteOptionFirst)
			pVoteOptionFirst = pDst;

		str_copy(pDst->m_aDescription, pSrc->m_aDescription, sizeof(pDst->m_aDescription));
		mem_copy(pDst->m_aCommand, pSrc->m_aCommand, Len + 1);
	}

	// clean up
	delete pSelf->m_pVoteOptionHeap;
	pSelf->m_pVoteOptionHeap = pVoteOptionHeap;
	pSelf->m_pVoteOptionFirst = pVoteOptionFirst;
	pSelf->m_pVoteOptionLast = pVoteOptionLast;
	pSelf->m_NumVoteOptions = NumVoteOptions;
}

void CGameContext::ConForceVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pType = pResult->GetString(0);
	const char *pValue = pResult->GetString(1);
	const char *pReason = pResult->NumArguments() > 2 && pResult->GetString(2)[0] ? pResult->GetString(2) : "No reason given";
	char aBuf[128] = {0};

	if (str_comp_nocase(pType, "option") == 0)
	{
		CVoteOptionServer *pOption = pSelf->m_pVoteOptionFirst;
		while (pOption)
		{
			if (str_comp_nocase(pValue, pOption->m_aDescription) == 0)
			{
				str_format(aBuf, sizeof(aBuf), "admin forced server option '%s' (%s)", pValue, pReason);
				pSelf->SendChatTarget(-1, aBuf);
				pSelf->Console()->ExecuteLine(pOption->m_aCommand, -1);
				break;
			}

			pOption = pOption->m_pNext;
		}

		if (!pOption)
		{
			str_format(aBuf, sizeof(aBuf), "'%s' isn't an option on this server", pValue);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
			return;
		}
	}
	else if (str_comp_nocase(pType, "kick") == 0)
	{
		int KickID = str_toint(pValue);
		if (KickID < 0 || KickID >= MAX_CLIENTS || !pSelf->m_apPlayers[KickID])
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid client id to kick");
			return;
		}

		if (!g_Config.m_SvVoteKickBantime)
		{
			str_format(aBuf, sizeof(aBuf), "kick %d %s", KickID, pReason);
			pSelf->Console()->ExecuteLine(aBuf, -1);
		}
		else
		{
			char aAddrStr[NETADDR_MAXSTRSIZE] = {0};
			pSelf->Server()->GetClientAddr(KickID, aAddrStr, sizeof(aAddrStr));
			str_format(aBuf, sizeof(aBuf), "ban %s %d %s", aAddrStr, g_Config.m_SvVoteKickBantime, pReason);
			pSelf->Console()->ExecuteLine(aBuf, -1);
		}
	}
	else if (str_comp_nocase(pType, "spectate") == 0)
	{
		int SpectateID = str_toint(pValue);
		if (SpectateID < 0 || SpectateID >= MAX_CLIENTS || !pSelf->m_apPlayers[SpectateID] || pSelf->m_apPlayers[SpectateID]->GetTeam() == TEAM_SPECTATORS)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid client id to move");
			return;
		}

		str_format(aBuf, sizeof(aBuf), "admin moved '%s' to spectator (%s)", pSelf->Server()->ClientName(SpectateID), pReason);
		pSelf->SendChatTarget(-1, aBuf);
		str_format(aBuf, sizeof(aBuf), "set_team %d -1 %d", SpectateID, g_Config.m_SvVoteSpectateRejoindelay);
		pSelf->Console()->ExecuteLine(aBuf, -1);
	}
}

void CGameContext::ConClearVotes(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "cleared votes");
	CNetMsg_Sv_VoteClearOptions VoteClearOptionsMsg;
	pSelf->Server()->SendPackMsg(&VoteClearOptionsMsg, MSGFLAG_VITAL, -1);
	pSelf->m_pVoteOptionHeap->Reset();
	pSelf->m_pVoteOptionFirst = 0;
	pSelf->m_pVoteOptionLast = 0;
	pSelf->m_NumVoteOptions = 0;
}

void CGameContext::ConVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	// check if there is a vote running
	if (!pSelf->m_VoteCloseTime)
		return;

	if (str_comp_nocase(pResult->GetString(0), "yes") == 0)
		pSelf->m_VoteEnforce = CGameContext::VOTE_ENFORCE_YES;
	else if (str_comp_nocase(pResult->GetString(0), "no") == 0)
		pSelf->m_VoteEnforce = CGameContext::VOTE_ENFORCE_NO;
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "admin forced vote %s", pResult->GetString(0));
	pSelf->SendChatTarget(-1, aBuf);
	str_format(aBuf, sizeof(aBuf), "forcing vote %s", pResult->GetString(0));
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
}

void CGameContext::ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if (pResult->NumArguments())
	{
		CNetMsg_Sv_Motd Msg;
		Msg.m_pMessage = g_Config.m_SvMotd;
		CGameContext *pSelf = (CGameContext *)pUserData;
		for (int i = 0; i < MAX_CLIENTS; ++i)
			if (pSelf->m_apPlayers[i])
				pSelf->Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
	}
}

void CGameContext::ConAbout(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pThis = (CGameContext *)pUserData;

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "%s %s by %s", MOD_NAME, MOD_VERSION, MOD_AUTHORS);
	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_CHAT, "chat", aBuf);

	if (MOD_CREDITS[0])
	{
		str_format(aBuf, sizeof(aBuf), "Credits: %s", MOD_CREDITS);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_CHAT, "chat", aBuf);
	}
	if (MOD_THANKS[0])
	{
		str_format(aBuf, sizeof(aBuf), "Thanks to: %s", MOD_THANKS);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_CHAT, "chat", aBuf);
	}
	if (MOD_SOURCES[0])
	{
		str_format(aBuf, sizeof(aBuf), "Sources: %s", MOD_SOURCES);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_CHAT, "chat", aBuf);
	}
}

void CGameContext::ConLanguage(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	int ClientID = pResult->GetClientID();

	const char *pLanguageCode = (pResult->NumArguments() > 0) ? pResult->GetString(0) : 0x0;
	char aFinalLanguageCode[8];
	aFinalLanguageCode[0] = 0;

	if (pLanguageCode)
	{
		if (str_comp_nocase(pLanguageCode, "ua") == 0)
			str_copy(aFinalLanguageCode, "uk", sizeof(aFinalLanguageCode));
		else
		{
			for (int i = 0; i < pSelf->Server()->Localization()->m_pLanguages.size(); i++)
			{
				if (str_comp_nocase(pLanguageCode, pSelf->Server()->Localization()->m_pLanguages[i]->GetFilename()) == 0)
					str_copy(aFinalLanguageCode, pLanguageCode, sizeof(aFinalLanguageCode));
			}
		}
	}

	if (aFinalLanguageCode[0])
	{
		pSelf->SetClientLanguage(ClientID, aFinalLanguageCode);
		pSelf->SendChatTarget(ClientID, _("语言成功切换为中文"));
	}
	else
	{
		const char *pLanguage = pSelf->m_apPlayers[ClientID]->GetLanguage();
		const char *pTxtUnknownLanguage = pSelf->Server()->Localization()->Localize(pLanguage, _("Unknown language"));
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "language", pTxtUnknownLanguage);

		dynamic_string BufferList;
		int BufferIter = 0;
		for (int i = 0; i < pSelf->Server()->Localization()->m_pLanguages.size(); i++)
		{
			if (i > 0)
				BufferIter = BufferList.append_at(BufferIter, ", ");
			BufferIter = BufferList.append_at(BufferIter, pSelf->Server()->Localization()->m_pLanguages[i]->GetFilename());
		}

		dynamic_string Buffer;
		pSelf->Server()->Localization()->Format_L(Buffer, pLanguage, _("可用语言: {str:ListOfLanguage}"), "ListOfLanguage", BufferList.buffer(), NULL);

		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "language", Buffer.buffer());

		pSelf->SendChatTarget(ClientID, Buffer.buffer());
	}

	return;
}

#ifdef CONF_SQLITE
void CGameContext::ConRegister(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pThis = (CGameContext *)pUserData;

	int ClientID = pResult->GetClientID();
	if (pResult->NumArguments() < 2)
	{
		pThis->SendChatTarget(ClientID, _("使用指令 /register <肉体之名> <肉体之匙> 来创造你在异仙界的血肉体(不用加上 '<' 和 '>' )"));
		pThis->SendChatTarget(ClientID, _("请牢记你在异仙界的肉体名以及此体之匙，否则你的血肉将永远失去魂体"));
		return;
	}

	char Username[256], Password[256];
	str_format(Username, sizeof(Username), pResult->GetString(0));
	str_format(Password, sizeof(Password), pResult->GetString(1));

	if (str_length(Username) > 15 || str_length(Username) < 4 || str_length(Password) > 15 || str_length(Password) < 4)
		return pThis->SendChatTarget(ClientID, _("血肉之名与血肉之匙必须被控制在4 - 15个仙界之字之拼音的范围内"));

	pThis->m_apPlayers[pResult->GetClientID()]->m_pAccount->Register(Username, Password, pResult->GetClientID());
	return;
}

void CGameContext::ConLogin(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pThis = (CGameContext *)pUserData;

	int ClientID = pResult->GetClientID();
	if (pResult->NumArguments() < 2)
	{
		pThis->SendChatTarget(ClientID, _("使用指令 /login <肉体之名> <肉体之匙> 进入异仙界(不用加上 '<' 和 '>' )"));
		pThis->SendChatTarget(ClientID, _("如果忘记了你的血肉之名与匙，可使用仙界之聊天功法QQ联系异仙界主神"));
		pThis->SendChatTarget(ClientID, _("主神的联络魂号：1562151175"));
		return;
	}

	char Username[17], Password[17];
	str_format(Username, sizeof(Username), pResult->GetString(0));
	str_format(Password, sizeof(Password), pResult->GetString(1));

	if (str_length(Username) > 15 || str_length(Username) < 4 || str_length(Password) > 15 || str_length(Password) < 4)
		return pThis->SendChatTarget(ClientID, _("血肉之名与血肉之匙必须被控制在4 - 15个仙界之字之拼音的范围内"));

	pThis->m_apPlayers[pResult->GetClientID()]->m_pAccount->Login(Username, Password, pResult->GetClientID());
	pThis->ClearVotes(ClientID);
	return;
}
#endif

#ifdef CONF_SQL
void CGameContext::ConRegister(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if (pResult->NumArguments() != 2)
	{
		pSelf->SendChatTarget(pResult->GetClientID(), _("Usage: /register <username> <password>"));
		return;
	}

	SAccData Data;
	Data.m_ClientID = pResult->GetClientID();
	str_copy(Data.m_Username, pResult->GetString(0), sizeof(Data.m_Username));
	str_copy(Data.m_Password, pResult->GetString(1), sizeof(Data.m_Password));
	pSelf->Sql()->CreateAccount(Data);
}

void CGameContext::ConLogin(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (pResult->NumArguments() != 2)
	{
		pSelf->SendChatTarget(pResult->GetClientID(), _("usage: /login <username> <password>"));
		return;
	}

	SAccData Data;
	Data.m_ClientID = pResult->GetClientID();
	str_copy(Data.m_Username, pResult->GetString(0), sizeof(Data.m_Username));
	str_copy(Data.m_Password, pResult->GetString(1), sizeof(Data.m_Password));

	pSelf->Sql()->Login(Data);
}
#endif

void CGameContext::ConNewPassword(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pThis = (CGameContext *)pUserData;
	// 不打算写这个，但是放在这假装我写了（
	// 写了这个就少了个赚钱手段捏
}

void CGameContext::ConShowMe(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pThis = (CGameContext *)pUserData;
	int CID = pResult->GetClientID();
	CPlayer *pPlayer = pThis->m_apPlayers[CID];

	if (!pPlayer->m_AccData.m_UserID)
		return;

	pThis->SendChatTarget(CID, _("- - - - - - - - -"));
	pThis->SendChatTarget(CID, _("肉体编号: {int:UID}"), "UID", &pPlayer->m_AccData.m_UserID);
	pThis->SendChatTarget(CID, _("- - - - - - - - -"));
	pThis->SendChatTarget(CID, _("肉体之名: {str:Name}"), "Name", &pPlayer->m_AccData.m_Username);
	pThis->SendChatTarget(CID, _("修为: {int:Xiuwei}"), "Xiuwei", &pPlayer->m_AccData.m_Xiuwei);
	pThis->SendChatTarget(CID, _("魄: {int:Xiuwei}"), "Xiuwei", &pPlayer->m_AccData.m_Po);
	pThis->SendChatTarget(CID, _("- - - - - - - - -"));
}

void CGameContext::SetClientLanguage(int ClientID, const char *pLanguage)
{
	Server()->SetClientLanguage(ClientID, pLanguage);
	if (m_apPlayers[ClientID])
	{
		m_apPlayers[ClientID]->SetLanguage(pLanguage);
		ClearVotes(ClientID);
	}
}

void CGameContext::ConsoleOutputCallback_Chat(const char *pStr, void *pUser)
{
	CGameContext *pThis = (CGameContext *)pUser;
	if (pThis->m_ConsoleOutput_Target >= 0 && pThis->m_ConsoleOutput_Target < MAX_CLIENTS)
		pThis->SendChatTarget(pThis->m_ConsoleOutput_Target, pStr);
}

void CGameContext::OnConsoleInit()
{
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();

	m_ConsoleOutputHandle_ChatPrint = Console()->RegisterPrintCallback(0, ConsoleOutputCallback_Chat, this);
	Console()->SetPrintOutputLevel_Hard(m_ConsoleOutputHandle_ChatPrint, IConsole::OUTPUT_LEVEL_CHAT);

	Console()->Register("tune", "si", CFGFLAG_SERVER, ConTuneParam, this, "Tune variable to value");
	Console()->Register("tune_reset", "", CFGFLAG_SERVER, ConTuneReset, this, "Reset tuning");
	Console()->Register("tune_dump", "", CFGFLAG_SERVER, ConTuneDump, this, "Dump tuning");

	Console()->Register("pause", "", CFGFLAG_SERVER, ConPause, this, "Pause/unpause game");
	Console()->Register("change_map", "?r", CFGFLAG_SERVER | CFGFLAG_STORE, ConChangeMap, this, "Change map");
	Console()->Register("restart", "?i", CFGFLAG_SERVER | CFGFLAG_STORE, ConRestart, this, "Restart in x seconds (0 = abort)");
	Console()->Register("broadcast", "r", CFGFLAG_SERVER, ConBroadcast, this, "Broadcast message");
	Console()->Register("say", "r", CFGFLAG_SERVER, ConSay, this, "Say in chat");
	Console()->Register("set_team", "ii?i", CFGFLAG_SERVER, ConSetTeam, this, "Set team of player to team");
	Console()->Register("set_team_all", "i", CFGFLAG_SERVER, ConSetTeamAll, this, "Set team of all players to team");
	Console()->Register("swap_teams", "", CFGFLAG_SERVER, ConSwapTeams, this, "Swap the current teams");
	Console()->Register("shuffle_teams", "", CFGFLAG_SERVER, ConShuffleTeams, this, "Shuffle the current teams");
	Console()->Register("lock_teams", "", CFGFLAG_SERVER, ConLockTeams, this, "Lock/unlock teams");

	Console()->Register("add_vote", "sr", CFGFLAG_SERVER, ConAddVote, this, "Add a voting option");
	Console()->Register("remove_vote", "s", CFGFLAG_SERVER, ConRemoveVote, this, "remove a voting option");
	Console()->Register("force_vote", "ss?r", CFGFLAG_SERVER, ConForceVote, this, "Force a voting option");
	Console()->Register("clear_votes", "", CFGFLAG_SERVER, ConClearVotes, this, "Clears the voting options");
	Console()->Register("vote", "r", CFGFLAG_SERVER, ConVote, this, "Force a vote to yes/no");

	Console()->Register("about", "", CFGFLAG_CHAT, ConAbout, this, "Show information about the mod");
	Console()->Register("language", "s", CFGFLAG_CHAT, ConLanguage, this, "Show information about the mod");

	Console()->Register("register", "ss", CFGFLAG_CHAT, ConRegister, this, "");
	Console()->Register("login", "ss", CFGFLAG_CHAT, ConLogin, this, "");
	Console()->Register("newpw", "s", CFGFLAG_CHAT, ConNewPassword, this, "");

	Console()->Register("me", "?s", CFGFLAG_CHAT, ConShowMe, this, "");

	Console()->Chain("sv_motd", ConchainSpecialMotdupdate, this);
}

void CGameContext::OnInit(/*class IKernel *pKernel*/)
{
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_World.SetGameServer(this);
	m_Events.SetGameServer(this);

	m_Sql->LoadItem();
	m_Sql->LoadZongMen();

	for (int i = 0; i < NUM_NETOBJTYPES; i++)
		Server()->SnapSetStaticsize(i, m_NetObjHandler.GetObjSize(i));

	m_Layers.Init(Kernel());
	m_Collision.Init(&m_Layers);

	// create game
	m_pController = new CGameController(this);

	// create all entities from the game layer
	CMapItemLayerTilemap *pTileMap = m_Layers.GameLayer();
	CTile *pTiles = (CTile *)Kernel()->RequestInterface<IMap>()->GetData(pTileMap->m_Data);

	for (int y = 0; y < pTileMap->m_Height; y++)
	{
		for (int x = 0; x < pTileMap->m_Width; x++)
		{
			int Index = pTiles[y * pTileMap->m_Width + x].m_Index;

			if (Index >= ENTITY_OFFSET)
			{
				vec2 Pos(x * 32.0f + 16.0f, y * 32.0f + 16.0f);
				m_pController->OnEntity(Index - ENTITY_OFFSET, Pos);
			}
		}
	}
	std::thread(InitBotEngineThread, m_pBotEngine, pTiles, pTileMap->m_Width, pTileMap->m_Height).detach();

	CheckBotNumber();
}

void CGameContext::InitBotEngineThread(CBotEngine *BotEngine, CTile *pTiles, int Width, int Height)
{
	BotEngine->Init(pTiles, Width, Height);
}

void CGameContext::OnShutdown()
{
	delete m_pController;
	m_pController = 0;
	Clear();
}

void CGameContext::OnSnap(int ClientID)
{
	CPlayer *pPlayer = m_apPlayers[ClientID];
	if (!pPlayer)
		return;
	m_World.Snap(ClientID);
	m_pController->Snap(ClientID);
	m_Events.Snap(ClientID);
	for (auto &arpPlayer : m_apPlayers)
	{
		if (arpPlayer)
			arpPlayer->Snap(ClientID);
	}

	// Snap bot debug info
	if (g_Config.m_SvBotEngineDrawGraph)
		m_pBotEngine->Snap(ClientID);
	for (int i = 0; i < MAX_CLIENTS; i++)
		if (m_apPlayers[i] && m_apPlayers[i]->IsBot() && g_Config.m_SvBotDrawTarget)
			m_apPlayers[i]->m_pBot->Snap(ClientID);

	if (ClientID >= MAX_PLAYERS)
		m_apPlayers[ClientID]->FakeSnap();
}
void CGameContext::OnPreSnap() {}
void CGameContext::OnPostSnap()
{
	m_Events.Clear();
}

bool CGameContext::IsClientReady(int ClientID)
{
	return m_apPlayers[ClientID] && m_apPlayers[ClientID]->m_IsReady ? true : false;
}

bool CGameContext::IsClientPlayer(int ClientID)
{
	return m_apPlayers[ClientID] && m_apPlayers[ClientID]->GetTeam() == TEAM_SPECTATORS ? false : true;
}

const char *CGameContext::GameType() { return m_pController && m_pController->m_pGameType ? m_pController->m_pGameType : ""; }
const char *CGameContext::Version() { return GAME_VERSION; }
const char *CGameContext::NetVersion() { return GAME_NETVERSION; }

IGameServer *CreateGameServer() { return new CGameContext; }

#ifdef CONF_SQLITE
void CGameContext::Register(const char *Username, const char *Password, int ClientID)
{
	char *pQueryBuf = sqlite3_mprintf("SELECT * FROM Accounts WHERE Username='%q'", Username);
	CQueryRegister *pQuery = new CQueryRegister();
	pQuery->Username = Username;
	pQuery->Password = Password;
	pQuery->m_ClientID = ClientID;
	pQuery->m_pGameServer = this;
	pQuery->Language = m_apPlayers[ClientID]->GetLanguage();
	pQuery->Query(m_pDatabase, pQueryBuf);
	sqlite3_free(pQueryBuf);
}

void CGameContext::Login(const char *Username, const char *Password, int ClientID)
{
	char *pQueryBuf = sqlite3_mprintf("SELECT * FROM Accounts WHERE Username='%q'", Username);
	CQueryLogin *pQuery = new CQueryLogin();
	pQuery->Username = Username;
	pQuery->Password = Password;
	pQuery->m_ClientID = ClientID;
	pQuery->m_pGameServer = this;
	pQuery->Query(m_pDatabase, pQueryBuf);
	sqlite3_free(pQueryBuf);
	ClearVotes(ClientID);
}

bool CGameContext::Apply(int ClientID, SAccData Data)
{
	char *pQueryBuf = sqlite3_mprintf("SELECT * FROM Accounts WHERE Username='%q'", Data.m_Username);
	CQueryApply *pQuery = new CQueryApply();
	pQuery->m_Data = Data;
	pQuery->m_pGameServer = this;
	pQuery->Query(m_pDatabase, pQueryBuf);
	sqlite3_free(pQueryBuf);

	return true;
}
#endif

#ifdef CONF_SQL
void CGameContext::Apply(int ClientID)
{
	AccDataList DataList;
	SAccData Data = m_apPlayers[ClientID]->m_AccData;
	Apply(ClientID, DataList.GetDataName(AccDataList::ACCDATA_XIUWEI), std::to_string(Data.m_Xiuwei).c_str());
	Apply(ClientID, DataList.GetDataName(AccDataList::ACCDATA_PO), std::to_string(Data.m_Po).c_str());
	Apply(ClientID, DataList.GetDataName(AccDataList::ACCDATA_TIZHI), std::to_string(Data.m_Tizhi).c_str());
	char aYuanSu[256];
	str_format(aYuanSu, sizeof(aYuanSu), "%d|%d|%d|%d|%d", Data.m_YuanSu[0], Data.m_YuanSu[1], Data.m_YuanSu[2], Data.m_YuanSu[3], Data.m_YuanSu[4]);
	Apply(ClientID, DataList.GetDataName(AccDataList::ACCDATA_YUANSU), aYuanSu);
	Apply(ClientID, DataList.GetDataName(AccDataList::ACCDATA_ZONGMEN), std::to_string(Data.m_ZongMen).c_str());
}

void CGameContext::Apply(int ClientID, const char NeedyUpdate[256], const char Value[256])
{
	Sql()->Update(ClientID, m_apPlayers[ClientID]->m_AccData, NeedyUpdate, Value);
	return;
}

#endif

// MMOTee
void CGameContext::AddVote(const char *Desc, const char *Cmd, int ClientID)
{
	while (*Desc && *Desc == ' ')
		Desc++;

	if (ClientID == -2)
		return;

	CVoteOptions Vote;
	str_copy(Vote.m_aDescription, Desc, sizeof(Vote.m_aDescription));
	str_copy(Vote.m_aCommand, Cmd, sizeof(Vote.m_aCommand));
	m_PlayerVotes[ClientID].add(Vote);

	// inform clients about added option
	CNetMsg_Sv_VoteOptionAdd OptionMsg;
	OptionMsg.m_pDescription = Vote.m_aDescription;
	Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::AddVote_VL(int To, const char *aCmd, const char *pText, ...)
{
	int Start = (To < 0 ? 0 : To);
	int End = (To < 0 ? MAX_CLIENTS : To + 1);

	dynamic_string Buffer;

	va_list VarArgs;
	va_start(VarArgs, pText);

	for (int i = Start; i < End; i++)
	{
		if (m_apPlayers[i])
		{
			Buffer.clear();
			Server()->Localization()->Format_VL(Buffer, m_apPlayers[i]->GetLanguage(), pText, VarArgs);
			AddVote(Buffer.buffer(), aCmd, i);
		}
	}

	Buffer.clear();
	va_end(VarArgs);
}

void CGameContext::InitVotes(int ClientID)
{
	if (!m_apPlayers[ClientID])
		return;

	dbg_msg("h", "%s", m_aZongMenData[m_apPlayers[ClientID]->m_AccData.m_ZongMen - 1].m_Name);
	SAccData Data = m_apPlayers[ClientID]->m_AccData;
	AddVote_VL(ClientID, "skip", _("==== ⚠异仙界修士面板⚠ ="));
	AddVote_VL(ClientID, "skip", _("肉体辨识标识: {int:UID}"), "UID", &m_apPlayers[ClientID]->m_AccData.m_UserID);
	AddVote_VL(ClientID, "skip", _("宗门: {str:ZongMen} - {str:info}"), "ZongMen", m_aZongMenData[Data.m_ZongMen - 1].m_Name, "info", m_aZongMenData[Data.m_ZongMen - 1].m_Desc);
	AddVote_VL(ClientID, "skip", _("修为: {int:Xiuwei}"), "Xiuwei", &Data.m_Xiuwei);
	AddVote_VL(ClientID, "skip", _("魄: {int:Po}"), "Po", &Data.m_Po);
	AddVote_VL(ClientID, "skip", _(" "));
	AddVote_VL(ClientID, "skip", _(" "));
	AddVote_VL(ClientID, "skip", _("=-= 灵根属性权重 =-= (置闰五行后归0)"));
	AddVote_VL(ClientID, "skip", _("- 金元素: {int:YuanSu}"), "YuanSu", &Data.m_YuanSu[0]);
	AddVote_VL(ClientID, "skip", _("- 木元素: {int:YuanSu}"), "YuanSu", &Data.m_YuanSu[1]);
	AddVote_VL(ClientID, "skip", _("- 水元素: {int:YuanSu}"), "YuanSu", &Data.m_YuanSu[2]);
	AddVote_VL(ClientID, "skip", _("- 火元素: {int:YuanSu}"), "YuanSu", &Data.m_YuanSu[3]);
	AddVote_VL(ClientID, "skip", _("- 土元素: {int:YuanSu}"), "YuanSu", &Data.m_YuanSu[4]);
	AddVote_VL(ClientID, "skip", _(" "));
	AddVote_VL(ClientID, "skip", _(" "));
	AddVote_VL(ClientID, "skip", _("== = - - 体质 - - = =="));
	for (int i = 1; i <= NUM_TIZHI; i++)
	{
		if (Data.m_Tizhi == 0)
		{
			AddVote_VL(ClientID, "skip", _("=--= - 曾是凡人，如今凡体，即：平凡人"));
			break;
		}
		if (Data.m_Tizhi & i)
			AddVote_VL(ClientID, "skip", _("=-- {str:Tizhi}"), "Tizhi", GetTizhiName(i));
	}
	AddVote_VL(ClientID, "skip", _(" "));
	AddVote_VL(ClientID, "skip", _(" "));
	AddVote_VL(ClientID, "skip", _("-=== - = 物品列表 = - ===-"));
	for (int i = 0; i < NUM_ITEMTYPE; i++)
	{
		SItemData_Function sidf;
		AddVote_VL(ClientID, "skip", _("-- {str:name} --"), "name", sidf.GetItemTypeName(i));
		for (int j = 0; j < NUM_ITEMDATA; j++)
		{
			if (sidf.GetItemType(j) == i)
			{
				if (Data.m_ItemData[j].m_Num > 0)
					AddVote_VL(ClientID, "skip", _("{str:IName} x{int:Num}"), "IName", m_aItemDataList[j].m_Name, "Num", &Data.m_ItemData[j].m_Num);
			}
		}
	}
}

void CGameContext::ClearVotes(int ClientID)
{
	m_PlayerVotes[ClientID].clear();

	// send vote options
	CNetMsg_Sv_VoteClearOptions ClearMsg;
	Server()->SendPackMsg(&ClearMsg, MSGFLAG_VITAL, ClientID);

	InitVotes(ClientID);
}

int CGameContext::PlayerCount()
{
	int c = 0;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (m_apPlayers[i])
			c++;
	}
	return c;
}

void CGameContext::DeleteBot(int i)
{
	Server()->DelBot(i);
	if (m_apPlayers[i] && m_apPlayers[i]->m_IsBot)
	{
		// dbg_msg("context", "Delete bot at slot: %d", i);
		delete m_apPlayers[i];
		m_apPlayers[i] = 0;
	}
}

bool CGameContext::AddBot(int i, bool UseDropPlayer)
{
	if (Server()->NewBot(i) == 1)
		return false;

	int BotNumber = 0;
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (!m_apPlayers[i])
			continue;
		if (m_apPlayers[i]->m_IsBot)
			BotNumber++;
	}

	int StartTeam = 0;

	if (BotNumber >= 32)
		StartTeam = 1; // 坐忘道
	else
		StartTeam = 0; // 清风观

	if (!UseDropPlayer || !m_apPlayers[i])
		m_apPlayers[i] = new (i) CPlayer(this, i, StartTeam);

	m_apPlayers[i]->m_IsBot = true;
	m_apPlayers[i]->m_pBot = new CBot(m_pBotEngine, m_apPlayers[i]);
	Server()->SetClientClan(i, "坐忘道");
	if (StartTeam == 1)
	{
		Server()->SetClientName(i, ZWD_XuLieNames[rand() % 27], true);
		m_apPlayers[i]->m_TeeInfos.m_UseCustomColor = true;
		m_apPlayers[i]->m_TeeInfos.m_ColorBody = 11730688;
		m_apPlayers[i]->m_TeeInfos.m_ColorFeet = 11730688;
	}
	else
	{
		Server()->SetClientName(i, "无名道士", true);
		Server()->SetClientClan(i, "清风观");
		m_apPlayers[i]->m_TeeInfos.m_UseCustomColor = true;
		m_apPlayers[i]->m_TeeInfos.m_ColorBody = 65280;
		m_apPlayers[i]->m_TeeInfos.m_ColorFeet = 65280;
	}

	return true;
}

bool CGameContext::ReplacePlayerByBot(int ClientID)
{
	int BotNumber = 0;
	int PlayerCount = -1;
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (!m_apPlayers[i])
			continue;
		if (m_apPlayers[i]->m_IsBot)
			BotNumber++;
		else
			PlayerCount++;
	}
	if (!PlayerCount || BotNumber >= g_Config.m_SvBotSlots)
		return false;
	return AddBot(ClientID, true);
}

void CGameContext::CheckBotNumber()
{
	if (!m_pBotEngine->m_Inited)
	{
		if(Server()->Tick()%100 == 0)
			SendBroadcast(_("服务器AI正在初始化，请耐心等待..."), -1);
		return;
	}
	int BotNumber = 0;
	int PlayerCount = 0;
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (!m_apPlayers[i])
			continue;
		if (m_apPlayers[i]->m_IsBot)
			BotNumber++;
		else
			PlayerCount++;
	}
	if (!PlayerCount)
		BotNumber += g_Config.m_SvBotSlots;
	// Remove bot excedent
	if (BotNumber - g_Config.m_SvBotSlots > 0)
	{
		int FirstBot = 0;
		for (int i = 0; i < BotNumber - g_Config.m_SvBotSlots; i++)
		{
			for (; FirstBot < MAX_CLIENTS; FirstBot++)
				if (m_apPlayers[FirstBot] && m_apPlayers[FirstBot]->m_IsBot)
					break;
			if (FirstBot < MAX_CLIENTS)
				DeleteBot(FirstBot);
		}
	}
	// Add missing bot if possible
	if (g_Config.m_SvBotSlots - BotNumber > 0)
	{
		int LastFreeSlot = Server()->MaxClients() - 1;
		for (int i = 0; i < g_Config.m_SvBotSlots - BotNumber; i++)
		{
			for (; LastFreeSlot >= 0; LastFreeSlot--)
				if (!m_apPlayers[LastFreeSlot])
					break;
			if (LastFreeSlot >= 0)
				AddBot(LastFreeSlot);
		}
	}
}
