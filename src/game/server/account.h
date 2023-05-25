/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#ifndef GAME_SERVER_ACCOUNT_H
#define GAME_SERVER_ACCOUNT_H
#include <engine/server.h>
#include <game/server/player.h>

class CAccount
{
public:
	CAccount(CPlayer *pPlayer, CGameContext *pGameServer);

	void Login(char *Username, char *Password, int ClientID);
	void Register(char *Username, char *Password, int ClientID);
	bool Apply();
	void Reset();
	void NewPassword(char *NewPassword);
	bool Exists(const char * Username);
	
private:
	class CPlayer *m_pPlayer;
	class CGameContext *m_pGameServer;
	CGameContext *GameServer() const { return m_pGameServer; }
};

#endif
