#ifndef AI_DEFENCE_H
#define AI_DEFENCE_H

#include "strategy.h"

class CDefence : public CStrategyPosition
{
	CZone m_aZones[2];
public:
	CDefence(CBotEngine* pBotEngine);

	void SetTeam(int Team);
};

#endif //AI_GENETICS_H
