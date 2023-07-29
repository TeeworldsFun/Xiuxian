#include "npc.h"
#include <string>

INPC::INPC(CGameContext *pGameServer, CPlayer *pPlayer, int ClientID)
{
    m_pGameServer = pGameServer;
    m_pPlayer = pPlayer;

    m_ClientID = ClientID;
}

void INPC::Motd(int To, const char *pText, ...)
{
    int Start = (To < 0 ? 0 : To);
	int End = (To < 0 ? MAX_CLIENTS : To + 1);

	CNetMsg_Sv_Motd Msg;

	dynamic_string Buffer;

	va_list VarArgs;
	va_start(VarArgs, pText);

	for (int i = Start; i < End; i++)
	{
		if (GameServer()->m_apPlayers[i])
		{
			Buffer.clear();
			GameServer()->Server()->Localization()->Format_VL(Buffer, GameServer()->m_apPlayers[i]->GetLanguage(), pText, VarArgs);

			Msg.m_pMessage = Buffer.buffer();
			GameServer()->Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
		}
	}

	va_end(VarArgs);
}

void INPC::Talk(int To, const char *pText, ...)
{
    int Start = (To < 0 ? 0 : To);
	int End = (To < 0 ? MAX_CLIENTS : To + 1);

	CNetMsg_Sv_Chat Msg;
	Msg.m_Team = 0;
	Msg.m_ClientID = GetCID();

	dynamic_string Buffer;

	va_list VarArgs;
	va_start(VarArgs, pText);

	for (int i = Start; i < End; i++)
	{
		if (GameServer()->m_apPlayers[i])
		{
			Buffer.clear();
			GameServer()->Server()->Localization()->Format_VL(Buffer, GameServer()->m_apPlayers[i]->GetLanguage(), pText, VarArgs);

			Msg.m_pMessage = Buffer.buffer();
            GameServer()->SendChat(GetCID(), i, Buffer.buffer());
            //GameServer()->Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
		}
	}

	va_end(VarArgs);
}

/**/