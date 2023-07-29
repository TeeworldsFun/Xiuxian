#ifndef GAME_SERVER_NPC_H
#define GAME_SERVER_NPC_H

#include "gamecontext.h"
// #include <engine/external/nlohmann/json.hpp>
#include <vector>

// 定义了一个NPC的API，要新建NPC只需要继承这个类
class INPC
{
private:
    int m_ClientID;
    CGameContext *m_pGameServer;
    CPlayer *m_pPlayer;

public:
    INPC(CGameContext *pGameServer, CPlayer *pPlayer, int ClientID);

    virtual void NPCAction() = 0; // NPC行动

    // 事件
    virtual void OnTakeDamage(int From, int Dmg, int Weapon) = 0; // 受伤了（哼哼啊啊啊啊啊啊
    virtual void OnDie(int Killer, int Weapon) = 0;               // 似了（悲
    virtual void OnHit(int Who) = 0;                              // 被锤子打了（恼
    virtual void OnHook(int Who) = 0;                             // 被钩住力（喜

    // 行为
    virtual void Motd(int To, const char *pText, ...);          // 使用这个函数和玩家交流
    virtual void Talk(int To, const char *pText, ...); // <-(o_O)\。   这个函数也可以

    int GetCID() { return m_ClientID; }
public:
    CGameContext *GameServer() const { return m_pGameServer; }
    CPlayer *GetPlayer() const { return m_pPlayer; }
};

#endif