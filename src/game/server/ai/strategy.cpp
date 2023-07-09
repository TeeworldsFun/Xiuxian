#include "strategy.h"

CStrategyPosition::CStrategyPosition(CBotEngine* pBotEngine) : m_pBotEngine(pBotEngine)
{
	m_Zone = GetDefaultZone();
}

bool CStrategyPosition::AttackPlayer(const CCharacter *pBot, const CCharacter *pEnemy)
{
  return distance(pBot->GetPos(), m_Zone.Center()) < m_Zone.Range();
}

bool CStrategyPosition::FollowPlayer(const CCharacter *pBot, const CCharacter *pEnemy)
{
  return distance(pEnemy->GetPos(), m_Zone.Center()) < m_Zone.Range();;
}

bool CStrategyPosition::IsInsideZone(const vec2& Pos)
{
  return distance(Pos, m_Zone.Center()) < m_Zone.Range();
}

CZone CStrategyPosition::GetDefaultZone()
{
	CZone Zone;
	Zone.m_Center = vec2(BotEngine()->GetWidth(), BotEngine()->GetHeight())*16;
	Zone.m_Range = max(BotEngine()->GetWidth(), BotEngine()->GetHeight())*16;
	return Zone;
}
