/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <string.h>
#include <fstream>
#include <engine/config.h>
#include <engine/shared/config.h>
#include "account.h"

#include <teeuniverses/components/localization.h>

CAccount::CAccount(CPlayer *pPlayer, CGameContext *pGameServer)
{
	m_pPlayer = pPlayer;
	m_pGameServer = pGameServer;
}

void CAccount::Login(char *Username, char *Password, int ClientID)
{
	char aBuf[125];
	if (m_pPlayer->m_AccData.m_UserID)
		return GameServer()->SendChatTarget(m_pPlayer->GetCID(), _("⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠"));

	/*char aHash[64]; //Result
	mem_zero(aHash, sizeof(aHash));
	Crypt(Password, (const unsigned char*) "d9", 1, 14, aHash);*/

	GameServer()->Login(Username, Password, m_pPlayer->GetCID());
}

void CAccount::Register(char *Username, char *Password, int ClientID)
{
	char aBuf[125];
	if(m_pPlayer->m_AccData.m_UserID)
		return GameServer()->SendChatTarget(m_pPlayer->GetCID(), _("⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠"));

	GameServer()->Register(Username, Password, m_pPlayer->GetCID());
}

bool CAccount::Exists(const char *Username)
{
}

bool CAccount::Apply()
{
	return GameServer()->Apply(m_pPlayer->GetCID(), m_pPlayer->m_AccData);
}

void CAccount::Reset()
{
	m_pPlayer->m_AccData.m_UserID = 0;
	str_copy(m_pPlayer->m_AccData.m_Username, "", 32);
	str_copy(m_pPlayer->m_AccData.m_Password, "", 32);
	
	m_pPlayer->m_AccData.m_Xiuwei = m_pPlayer->m_AccData.m_Xianqi = 0;
}

void CAccount::NewPassword(char *NewPassword)
{	
	str_copy(m_pPlayer->m_AccData.m_Password, NewPassword, 32);
	Apply();
	
	dbg_msg("account", "Password changed - ('%s')", m_pPlayer->m_AccData.m_Username);
	GameServer()->SendChatTarget(m_pPlayer->GetCID(), _("Password successfully changed!"));
}
