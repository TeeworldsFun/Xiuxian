/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include <math.h>
#include <engine/map.h>
#include <engine/kernel.h>

#include <game/mapitems.h>
#include <game/layers.h>
#include <game/collision.h>

CCollision::CCollision()
{
	m_pTiles = 0;
	m_Width = 0;
	m_Height = 0;
	m_pLayers = 0;
}

void CCollision::Init(class CLayers *pLayers)
{
	m_pLayers = pLayers;
	m_Width = m_pLayers->GameLayer()->m_Width;
	m_Height = m_pLayers->GameLayer()->m_Height;
	m_pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer()->m_Data));

	for(int i = 0; i < m_Width*m_Height; i++)
	{
		int Index = m_pTiles[i].m_Index;

		if(Index > 128)
			continue;

		switch(Index)
		{
		case TILE_DEATH:
			m_pTiles[i].m_Index = COLFLAG_DEATH;
			break;
		case TILE_SOLID:
			m_pTiles[i].m_Index = COLFLAG_SOLID;
			break;
		case TILE_NOHOOK:
			m_pTiles[i].m_Index = COLFLAG_SOLID|COLFLAG_NOHOOK;
			break;
		default:
			m_pTiles[i].m_Index = 0;
		}
	}
}

int CCollision::GetTile(int x, int y)
{
	int Nx = clamp(x/32, 0, m_Width-1);
	int Ny = clamp(y/32, 0, m_Height-1);

	return m_pTiles[Ny*m_Width+Nx].m_Index > 128 ? 0 : m_pTiles[Ny*m_Width+Nx].m_Index;
}

bool CCollision::IsTileSolid(int x, int y)
{
	return GetTile(x, y)&COLFLAG_SOLID;
}

// TODO: rewrite this smarter!
int CCollision::IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision)
{
	float Distance = distance(Pos0, Pos1);
	int End(Distance+1);
	vec2 Last = Pos0;

	for(int i = 0; i < End; i++)
	{
		float a = i/Distance;
		vec2 Pos = mix(Pos0, Pos1, a);
		if(CheckPoint(Pos.x, Pos.y))
		{
			if(pOutCollision)
				*pOutCollision = Pos;
			if(pOutBeforeCollision)
				*pOutBeforeCollision = Last;
			return GetCollisionAt(Pos.x, Pos.y);
		}
		Last = Pos;
	}
	if(pOutCollision)
		*pOutCollision = Pos1;
	if(pOutBeforeCollision)
		*pOutBeforeCollision = Pos1;
	return 0;
}

// TODO: OPT: rewrite this smarter!
void CCollision::MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces)
{
	if(pBounces)
		*pBounces = 0;

	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;
	if(CheckPoint(Pos + Vel))
	{
		int Affected = 0;
		if(CheckPoint(Pos.x + Vel.x, Pos.y))
		{
			pInoutVel->x *= -Elasticity;
			if(pBounces)
				(*pBounces)++;
			Affected++;
		}

		if(CheckPoint(Pos.x, Pos.y + Vel.y))
		{
			pInoutVel->y *= -Elasticity;
			if(pBounces)
				(*pBounces)++;
			Affected++;
		}

		if(Affected == 0)
		{
			pInoutVel->x *= -Elasticity;
			pInoutVel->y *= -Elasticity;
		}
	}
	else
	{
		*pInoutPos = Pos + Vel;
	}
}

bool CCollision::TestBox(vec2 Pos, vec2 Size)
{
	Size *= 0.5f;
	if(CheckPoint(Pos.x-Size.x, Pos.y-Size.y))
		return true;
	if(CheckPoint(Pos.x+Size.x, Pos.y-Size.y))
		return true;
	if(CheckPoint(Pos.x-Size.x, Pos.y+Size.y))
		return true;
	if(CheckPoint(Pos.x+Size.x, Pos.y+Size.y))
		return true;
	return false;
}

void CCollision::MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity)
{
	// do the move
	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;

	float Distance = length(Vel);
	int Max = (int)Distance;

	if(Distance > 0.00001f)
	{
		//vec2 old_pos = pos;
		float Fraction = 1.0f/(float)(Max+1);
		for(int i = 0; i <= Max; i++)
		{
			//float amount = i/(float)max;
			//if(max == 0)
				//amount = 0;

			vec2 NewPos = Pos + Vel*Fraction; // TODO: this row is not nice

			if(TestBox(vec2(NewPos.x, NewPos.y), Size))
			{
				int Hits = 0;

				if(TestBox(vec2(Pos.x, NewPos.y), Size))
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					Hits++;
				}

				if(TestBox(vec2(NewPos.x, Pos.y), Size))
				{
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
					Hits++;
				}

				// neither of the tests got a collision.
				// this is a real _corner case_!
				if(Hits == 0)
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
				}
			}

			Pos = NewPos;
		}
	}

	*pInoutPos = Pos;
	*pInoutVel = Vel;
}

int CCollision::FastIntersectLine(vec2 Pos0, vec2 Pos1)
{
	//float Distance = distance(Pos0, Pos1) / 4.0f;
	float Distance = distance(Pos0, Pos1);
	int End(Distance+1);

	for(int i = 0; i < End; i++)
	{
		float a = i/Distance;
		vec2 Pos = mix(Pos0, Pos1, a);
		if(CheckPoint(Pos.x, Pos.y))
			return GetCollisionAt(Pos.x, Pos.y);
	}
	return 0;
}

void CCollision::GenerateWaypoints()
{
	ClearWaypoints();
	for(int x = 2; x < m_Width-2; x++)
	{
		for(int y = 2; y < m_Height-2; y++)
		{
			if (m_pTiles[y*m_Width+x].m_Index && m_pTiles[y*m_Width+x].m_Index < 128)
				continue;

			
			// find all outer corners
			if ((IsTileSolid((x-1)*32, (y-1)*32) && !IsTileSolid((x-1)*32, (y-0)*32) && !IsTileSolid((x-0)*32, (y-1)*32)) ||
				(IsTileSolid((x-1)*32, (y+1)*32) && !IsTileSolid((x-1)*32, (y-0)*32) && !IsTileSolid((x-0)*32, (y+1)*32)) ||
				(IsTileSolid((x+1)*32, (y+1)*32) && !IsTileSolid((x+1)*32, (y-0)*32) && !IsTileSolid((x-0)*32, (y+1)*32)) ||
				(IsTileSolid((x+1)*32, (y-1)*32) && !IsTileSolid((x+1)*32, (y-0)*32) && !IsTileSolid((x-0)*32, (y-1)*32)))
			{
				// outer corner found -> create a waypoint
				AddWaypoint(vec2(x, y));
			}
			else
			// find all inner corners
			if ((IsTileSolid((x+1)*32, y*32) || IsTileSolid((x-1)*32, y*32)) && (IsTileSolid(x*32, (y+1)*32) || IsTileSolid(x*32, (y+1)*32)))
			{
				// inner corner found -> create a waypoint
				//AddWaypoint(vec2(x, y), true);
				AddWaypoint(vec2(x, y));
			}
		}
	}
		
	bool KeepGoing = true;
	int i = 0;
	
	while (KeepGoing && i++ < 10)
	{
		ConnectWaypoints();
		KeepGoing = GenerateSomeMoreWaypoints();
	}
	ConnectWaypoints();
}


// create a new waypoints between connected, far apart ones
bool CCollision::GenerateSomeMoreWaypoints()
{
	bool Result = false;
	
	for (int i = 0; i < m_WaypointCount; i++)
	{
		for (int j = 0; j < m_WaypointCount; j++)
		{
			if (m_apWaypoint[i] && m_apWaypoint[j] && m_apWaypoint[i]->Connected(m_apWaypoint[j]))
			{
				if (abs(m_apWaypoint[i]->m_X - m_apWaypoint[j]->m_X) > 20 && m_apWaypoint[i]->m_Y == m_apWaypoint[j]->m_Y)
				{
					int x = (m_apWaypoint[i]->m_X + m_apWaypoint[j]->m_X) / 2;
					
					if (IsTileSolid(x*32, (m_apWaypoint[i]->m_Y+1)*32) || IsTileSolid(x*32, (m_apWaypoint[i]->m_Y-1)*32))
					{
						AddWaypoint(vec2(x, m_apWaypoint[i]->m_Y));
						Result = true;
					}
				}
				
				if (abs(m_apWaypoint[i]->m_Y - m_apWaypoint[j]->m_Y) > 30 && m_apWaypoint[i]->m_X == m_apWaypoint[j]->m_X)
				{
					int y = (m_apWaypoint[i]->m_Y + m_apWaypoint[j]->m_Y) / 2;
					
					if (IsTileSolid((m_apWaypoint[i]->m_X+1)*32, y*32) || IsTileSolid((m_apWaypoint[i]->m_X-1)*32, y*32))
					{
						AddWaypoint(vec2(m_apWaypoint[i]->m_X, y));
						Result = true;
					}
				}
				
				
				m_apWaypoint[i]->Unconnect(m_apWaypoint[j]);
			}
		}
	}
	
	return Result;
}



CWaypoint *CCollision::GetWaypointAt(int x, int y)
{
	for (int i = 0; i < m_WaypointCount; i++)
	{
		if (m_apWaypoint[i])
		{
			if (m_apWaypoint[i]->m_X == x && m_apWaypoint[i]->m_Y == y)
				return m_apWaypoint[i];
		}
	}
	return NULL;
}


void CCollision::ConnectWaypoints()
{
	m_ConnectionCount = 0;
	
	// clear existing connections
	for (int i = 0; i < m_WaypointCount; i++)
	{
		if (!m_apWaypoint[i])
			continue;
		
		m_apWaypoint[i]->ClearConnections();
	}
		
	
		
	for (int i = 0; i < m_WaypointCount; i++)
	{
		if (!m_apWaypoint[i])
			continue;
		
		int x, y;
		
		x = m_apWaypoint[i]->m_X - 1;
		y = m_apWaypoint[i]->m_Y;
		
		// find waypoints at left
		while (!m_pTiles[y*m_Width+x].m_Index || m_pTiles[y*m_Width+x].m_Index >= 128)
		{
			CWaypoint *W = GetWaypointAt(x, y);
			
			if (W)
			{
				if (m_apWaypoint[i]->Connect(W))
					m_ConnectionCount++;
				break;
			}
			
			//if (!IsTileSolid(x*32, (y-1)*32) && !IsTileSolid(x*32, (y+1)*32))
			if (!IsTileSolid(x*32, (y+1)*32))
				break;
			
			x--;
		}
		
		x = m_apWaypoint[i]->m_X;
		y = m_apWaypoint[i]->m_Y - 1;
		
		int n = 0;
		
		// find waypoints at up
		//bool SolidFound = false;
		while ((!m_pTiles[y*m_Width+x].m_Index || m_pTiles[y*m_Width+x].m_Index >= 128) && n++ < 10)
		{
			CWaypoint *W = GetWaypointAt(x, y);
			
			//if (IsTileSolid((x+1)*32, y*32) || IsTileSolid((x+1)*32, y*32))
			//	SolidFound = true;
			
			//if (W && SolidFound)
			if (W)
			{
				if (m_apWaypoint[i]->Connect(W))
					m_ConnectionCount++;
				break;
			}
			
			y--;
		}
	}
	
	
	// connect to near, visible waypoints
	for (int i = 0; i < m_WaypointCount; i++)
	{
		if (m_apWaypoint[i] && m_apWaypoint[i]->m_InnerCorner)
			continue;
		
		for (int j = 0; j < m_WaypointCount; j++)
		{
			if (m_apWaypoint[j] && m_apWaypoint[j]->m_InnerCorner)
				continue;
			
			if (m_apWaypoint[i] && m_apWaypoint[j] && !m_apWaypoint[i]->Connected(m_apWaypoint[j]))
			{
				float Dist = distance(m_apWaypoint[i]->m_Pos, m_apWaypoint[j]->m_Pos);
				
				if (Dist < 600 && !IntersectLine(m_apWaypoint[i]->m_Pos, m_apWaypoint[j]->m_Pos, NULL, NULL))
				{
					if (m_apWaypoint[i]->Connect(m_apWaypoint[j]))
						m_ConnectionCount++;
				}
			}
		}
	}
}


CWaypoint *CCollision::GetClosestWaypoint(vec2 Pos)
{
	CWaypoint *W = NULL;
	float Dist = 9000;
	
	for (int i = 0; i < m_WaypointCount; i++)
	{
		if (m_apWaypoint[i])
		{
			//int d = abs(m_apWaypoint[i]->m_Pos.x-Pos.x)+abs(m_apWaypoint[i]->m_Pos.y-Pos.y);
			int d = distance(m_apWaypoint[i]->m_Pos, Pos);
			
			if (d < Dist && d < 800)
			{
				if (!FastIntersectLine(m_apWaypoint[i]->m_Pos, Pos))
				{
					W = m_apWaypoint[i];
					Dist = d;
				}
			}
		}
	}
	
	return W;
}


void CCollision::SetWaypointCenter(vec2 Position)
{
	m_pCenterWaypoint = GetClosestWaypoint(Position);
	
	// clear path weights
	for (int i = 0; i < m_WaypointCount; i++)
	{
		if (m_apWaypoint[i])
			m_apWaypoint[i]->m_PathDistance = 0;
	}
	
	if (m_pCenterWaypoint)
		m_pCenterWaypoint->SetCenter();
	
}


void CCollision::AddWeight(vec2 Pos, int Weight)
{
	CWaypoint *Wp = GetClosestWaypoint(Pos);
	
	if (Wp)
		Wp->AddWeight(Weight);
}



bool CCollision::FindWaypointPath(vec2 TargetPos)
{

	
	CWaypoint *Target = GetClosestWaypoint(TargetPos);
	
	
	if (Target && m_pCenterWaypoint)
	{
		if (m_pPath)
			delete m_pPath;
		
		m_pPath = Target->FindPathToCenter();
		
		// for displaying the chosen waypoints
		for (int w = 0; w < 99; w++)
			m_aPath[w] = vec2(0, 0);
		
		if (m_pPath)
		{
			CWaypointPath *Wp = m_pPath;
			
			int p = 0;
			for (int w = 0; w < 10; w++)
			{
				m_aPath[p++] = vec2(Wp->m_Pos.x, Wp->m_Pos.y);
				
				if (Wp->m_pNext)
					Wp = Wp->m_pNext;
				else
					break;
			}
			
			return true;
		}
	}
	
	return false;
}

void CCollision::ClearWaypoints()
{
	m_WaypointCount = 0;
	
	for (int i = 0; i < MAX_WAYPOINTS; i++)
	{
		if (m_apWaypoint[i])
			delete m_apWaypoint[i];
		
		m_apWaypoint[i] = NULL;
	}
	
	m_pCenterWaypoint = NULL;
}


void CCollision::AddWaypoint(vec2 Position, bool InnerCorner)
{
	if (m_WaypointCount >= MAX_WAYPOINTS)
		return;
	
	m_apWaypoint[m_WaypointCount] = new CWaypoint(Position, InnerCorner);
	m_WaypointCount++;
}