#ifndef AI_GENETICS_H
#define AI_GENETICS_H

class CGenetics
{
	struct CGenome {
		int* m_pGenome;
		int m_Fitness;
	} *m_pGenomes;
	int m_Size;
	int m_NumGenomes;

	int m_CurGenome;

	void InitGenomes();
	void Evolve();

public:
	CGenetics(int Size, int NumGenomes);
	~CGenetics();

	int* GetGenome();
	void SetFitness(int Fitness);
	void NextGenome();

	static int GenomeComp(const void* a, const void* b);
};

#endif //AI_GENETICS_H
