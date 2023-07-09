#include "genetics.h"
#include <stdlib.h>
#include <base/system.h>
#include <base/math.h>

void CGenetics::InitGenomes()
{
	for(int i = 0 ; i < m_NumGenomes ; i++)
		for(int j = 0 ; j < m_Size ; j++)
			m_pGenomes[i].m_pGenome[j] = rand()%100;
}

int CGenetics::GenomeComp(const void* a, const void* b)
{
	CGenome *g0 = (CGenome *)a;
	CGenome *g1 = (CGenome *)b;
	if(g0->m_Fitness > g1->m_Fitness)
		return 1;
	if(g0->m_Fitness < g1->m_Fitness)
		return -1;
	return 0;
}

void CGenetics::Evolve()
{
	qsort(m_pGenomes,m_NumGenomes,sizeof(CGenome),GenomeComp);
	for(int i = 0 ; i < m_NumGenomes ; i++)
	{
		CGenome Genome = m_pGenomes[i];
		dbg_msg("Cgentics", "fitness=%d (%d %d %d %d %d %d %d %d)", Genome.m_Fitness, Genome.m_pGenome[0], Genome.m_pGenome[1], Genome.m_pGenome[2], Genome.m_pGenome[3], Genome.m_pGenome[4], Genome.m_pGenome[5], Genome.m_pGenome[6], Genome.m_pGenome[7]);
	}
	// Merge
	for(int i = 0 ; i < (m_NumGenomes >> 1) ; i++)
	{
		CGenome Genome = m_pGenomes[i];
		for(int j = 0 ; j < m_Size ; j++)
			Genome.m_pGenome[j] = m_pGenomes[rand()%(m_NumGenomes >> 1)+(m_NumGenomes >> 1)].m_pGenome[j];
	}
	// Mutate
	for(int i = 0 ; i < m_NumGenomes ; i++)
	{
		CGenome Genome = m_pGenomes[i];
		Genome.m_Fitness = 0;
		for(int j = 0 ; j < m_Size ; j++)
			if(rand()%100 == 1)
				Genome.m_pGenome[j] = rand()%100;
	}
}

CGenetics::CGenetics(int Size, int NumGenomes)
{
	m_pGenomes = (CGenome*) mem_alloc(NumGenomes*sizeof(CGenome),1);
	for(int i = 0 ; i < NumGenomes ; i++)
	{
		m_pGenomes[i].m_pGenome = (int*) mem_alloc(Size*sizeof(int),1);
		m_pGenomes[i].m_Fitness = 0;
	}
	m_Size = Size;
	m_NumGenomes = NumGenomes;
	m_CurGenome = 0;
	InitGenomes();
}
CGenetics::~CGenetics()
{
	for(int i = 0 ; i < m_NumGenomes ; i++)
		mem_free(m_pGenomes[i].m_pGenome);
	mem_free(m_pGenomes);
}

int* CGenetics::GetGenome()
{
	return (m_pGenomes + m_CurGenome)->m_pGenome;
}

void CGenetics::SetFitness(int Fitness)
{
	(m_pGenomes + m_CurGenome)->m_Fitness = Fitness;
}

void CGenetics::NextGenome()
{
	m_CurGenome++;
	if(m_CurGenome == m_NumGenomes)
	{
		Evolve();
		m_CurGenome = 0;
	}
}
