#ifndef GAME_SERVER_AI_DM_AI_H
#define GAME_SERVER_AI_DM_AI_H
#include <game/server/ai.h>
#include <game/server/gamecontext.h>

#include <mutex>

inline std::mutex mute;

enum XIESUI_TYPES
{
	TYPE_L1 = 0,
	TYPE_L2,
	TYPE_L3,
	TYPE_L4,
};

class CAIXieSui : public CAI
{
public:
	CAIXieSui(CGameContext *pGameServer, CPlayer *pPlayer, int Type);

	virtual void DoBehavior();
	void OnCharacterSpawn(class CCharacter *pChr);

	static void ThreadBotAction(void *pUser);

	int m_HookTimer;

private:
	int m_SkipMoveUpdate;
};

#endif