#include "defence.h"

CDefence::CDefence(CBotEngine* pBotEngine) : CStrategyPosition(pBotEngine)
{
	CGraph *pGraph = BotEngine()->GetGraph();
	int Flag0 = BotEngine()->GetClosestVertex(BotEngine()->GetFlagStandPos(0));
	int Flag1 = BotEngine()->GetClosestVertex(BotEngine()->GetFlagStandPos(1));
	vec2 *pVertices = (vec2*) mem_alloc(pGraph->m_Diameter*sizeof(vec2),1);
	int Size = pGraph->GetPath(Flag0,Flag1,pVertices);
	if(Size > 3)
	{
		int a0 = 0, a1 = Size/4, a3 = 3*Size / 4, a4 = Size-1;
		m_aZones[0].m_Center = pVertices[a0];
		m_aZones[0].m_Range = distance(pVertices[a0],pVertices[a1]);
		m_aZones[1].m_Center = pVertices[a4];
		m_aZones[1].m_Range = distance(pVertices[a4],pVertices[a3]);
	}
	else
		m_aZones[0] = m_aZones[1] = m_Zone;
	mem_free(pVertices);
}

void CDefence::SetTeam(int Team)
{
	m_Zone = m_aZones[Team&1];
}