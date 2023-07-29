#ifndef GAME_SERVER_NPCS_TEST_H
#define GAME_SERVER_NPCS_TEST_H

#include <game/server/npc.h>

class CTestNPC : public INPC
{
public:
    CTestNPC(CGameContext *pGameServer, CPlayer *pPlayer, int ClientID);

    virtual void NPCAction(); // NPC行动

    // 事件
    virtual void OnTakeDamage(int From, int Dmg, int Weapon); // output: fku
    virtual void OnDie(int Killer, int Weapon);               // 输出：die
    virtual void OnHit(int Who);                              // 输出：hit
    virtual void OnHook(int Who);                             // 输出：hook
};

#endif