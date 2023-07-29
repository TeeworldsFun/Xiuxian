#include "test-npc.h"

CTestNPC::CTestNPC(CGameContext *pGameServer, CPlayer *pPlayer, int ClientID) : INPC(pGameServer, pPlayer, ClientID)
{
}

void CTestNPC::NPCAction()
{
    return;
}

void CTestNPC::OnTakeDamage(int From, int Dmg, int Weapon)
{
    if (GameServer()->m_apPlayers[From]->GetNPC())
        return;
    
    if(GameServer()->m_apPlayers[From]->m_LastHitNPC < 100)
        return;

    GameServer()->m_apPlayers[From]->LastHitNPC();

    if (Dmg > 4)
        Talk(-1, _("(尖叫)滚！！！！！！11"));
    else if (GetPlayer()->GetCharacter()->GetHealth() < 4)
        Talk(-1, _("(大声)滚。"));
    else
        Talk(From, _("滚一边去"));
}

void CTestNPC::OnDie(int Killer, int Weapon)
{
    if (GameServer()->m_apPlayers[Killer]->GetNPC())
        return;

    Talk(-1, _("靠！{str:pname}! 你把我杀了！！！"), "pname", GameServer()->Server()->ClientName(Killer));
}

void CTestNPC::OnHit(int Who)
{
    if (GameServer()->m_apPlayers[Who]->GetNPC())
        return;

    GameServer()->m_apPlayers[Who]->LastHitNPC();

    dbg_msg("sad","FUCK ONHIT");
    Motd(Who, _("{str:name}对你说:\n"
                "\"你好！这是修仙模式的测试服！\n"
                "  我们正在测试NPC对话机制！所以你可以和我交流！\"\n"
                "\n"
                "\n"
                "\n"
                "\n"
                "\n"
                "\n"
                "\nTAB to Close"),
         "name", GameServer()->Server()->ClientName(GetCID()));

    Talk(Who, _("你好！这是修仙模式的测试服！"));
    Talk(Who, _("我们正在测试NPC对话机制！所以你可以和我交流！"));
    Talk(Who, _("在这个测试版里我们只对话不战斗"));
}

void CTestNPC::OnHook(int Who)
{
}