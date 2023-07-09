#ifndef AI_STRATEGY_H
#define AI_STRATEGY_H

#include <base/vmath.h>
#include <game/server/entities/character.h>

#include <game/server/botengine.h>

struct CZone
{
  vec2 m_Center;
  double m_Range;

  //CZone(const vec2& Center, double Range) : m_Center(Center), m_Range(Range) {};
  void Update(const vec2& Center, double Range) { m_Center = Center; m_Range = Range; }
  void SetCenter(const vec2& Center) { m_Center = Center; }
  void SetRange(double Range) { m_Range = Range; }

  vec2 Center() { return m_Center; }
  double Range() { return m_Range; }
};

class CStrategyPosition
{
protected:
  CZone m_Zone;
  CBotEngine* m_pBotEngine;

  CBotEngine* BotEngine() { return m_pBotEngine; }
public:
	CStrategyPosition(CBotEngine* pBotEngine);
  virtual ~CStrategyPosition() {}

  bool AttackPlayer(const CCharacter *pBot, const CCharacter *pEnemy);
  bool FollowPlayer(const CCharacter *pBot, const CCharacter *pEnemy);

  CZone GetDefaultZone();

  virtual void SetTeam(int) {};

  bool IsInsideZone(const vec2& Pos);
  vec2 GetCenter() { return m_Zone.Center(); }
};

#endif //AI_STRATEGY_H
