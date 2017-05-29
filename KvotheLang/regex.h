#pragma once

#include <assert.h>

struct SRegex // tag = regex
{
	virtual void PrintDebug() = 0;

	virtual ~SRegex(){};

	virtual SNfaFragment NfafragCreate(SNfaBuilder * pBuilder) = 0;
};

struct SRegexList : public SRegex // tag = regexlist
{
	vector<SRegex *> m_arypRegex;
};

struct SUnion : public SRegexList // tag = union
{
	void PrintDebug() override
	{
		printf("(UNION");

		for(SRegex * pRegex : m_arypRegex)
		{
			printf(" ");
			pRegex->PrintDebug();
		}

		printf(")");
	}

	SNfaFragment NfafragCreate(SNfaBuilder * pBuilder) override
	{
		assert(m_arypRegex.size() > 0);

		SNfaState * pNfasOr = pBuilder->PNfasCreate();

		vector<SNfaState *> aryUnpatched;

		for(SRegex * pRegex : m_arypRegex)
		{
			SNfaFragment nfaAlt = pRegex->NfafragCreate(pBuilder);
			aryUnpatched.insert(aryUnpatched.end(), nfaAlt.m_aryUnpatched.begin(), nfaAlt.m_aryUnpatched.end());
			pNfasOr->AddEpsilon(nfaAlt.m_pStateBegin);
		}

		return SNfaFragment(pNfasOr, aryUnpatched);
	}
};

struct SConcatination : public SRegexList // tag = concat
{
	void PrintDebug() override
	{
		printf("(CONCAT");

		for(SRegex * pRegex : m_arypRegex)
		{
			printf(" ");
			pRegex->PrintDebug();
		}

		printf(")");
	}

	SNfaFragment NfafragCreate(SNfaBuilder * pBuilder) override
	{
		assert(m_arypRegex.size() > 0);

		SNfaFragment nfa = m_arypRegex[0]->NfafragCreate(pBuilder);

		for(size_t i = 1; i < m_arypRegex.size(); ++i)
		{
			nfa.Patch(m_arypRegex[i]->NfafragCreate(pBuilder));
		}

		return nfa;
	}
};

struct SQuantifier : public SRegex // tag = quant
{
	int m_cMic;
	int m_cMac;
	SRegex * m_pRegex;

	void PrintDebug() override
	{
		printf("({%d, %d} ", m_cMic, m_cMac);

		m_pRegex->PrintDebug();

		printf(")");
	}

	SNfaFragment NfaCreateCount(SNfaBuilder * pBuilder, int c)
	{
		SConcatination concat;

		for(int iC = 0; iC < c; ++ iC)
		{
			concat.m_arypRegex.push_back(m_pRegex);
		}

		return concat.NfafragCreate(pBuilder);
	}

	SNfaFragment NfaCreateStar(SNfaBuilder * pBuilder)
	{
		// create a ?
		
		SNfaState * pNfasOr = pBuilder->PNfasCreate();
		SNfaFragment nfa = m_pRegex->NfafragCreate(pBuilder);
		pNfasOr->AddEpsilon(nfa.m_pStateBegin);
		pNfasOr->AddEpsilon(nullptr);

		// the final nfa will only have one unpatched node, the starting node, 
		// because the nfa created from m_pregex is patched back to pOrState

		SNfaFragment nfaStar(pNfasOr, { pNfasOr });

		// loop back the nfa created from m_pregex to pOrState
		// turning a ? into a *

		nfa.Patch(nfaStar);

		return nfaStar;
	}

	SNfaFragment NfaCreateQMark(SNfaBuilder * pBuilder)
	{
		// create a ?
		
		SNfaState * pNfasOr = pBuilder->PNfasCreate();
		SNfaFragment nfa = m_pRegex->NfafragCreate(pBuilder);
		pNfasOr->AddEpsilon(nfa.m_pStateBegin);
		pNfasOr->AddEpsilon(nullptr);

		// the unpatched states are pOrState and all the unpatched states of
		// the nfa created from m_pRegex

		vector<SNfaState *> aryUnpatched;
		aryUnpatched.push_back(pNfasOr);
		aryUnpatched.insert(aryUnpatched.end(), nfa.m_aryUnpatched.begin(), nfa.m_aryUnpatched.end());

		return SNfaFragment(pNfasOr, aryUnpatched);
	}

	SNfaFragment NfaCreateCountOptional(SNfaBuilder * pBuilder, int c)
	{
		assert(c > 0);

		// create nested ?'s, starting on the right and working
		// back to the left to the starting node
		
		SNfaFragment nfaLeftMost;

		for(int iC = c; iC > 0; --iC)
		{
			SNfaFragment nfaQMark = NfaCreateQMark(pBuilder);

			if(nfaLeftMost.m_pStateBegin)
			{
				nfaQMark.Patch(nfaLeftMost);
			}

			nfaLeftMost = nfaQMark;
		}

		return nfaLeftMost;
	}

	SNfaFragment NfafragCreate(SNfaBuilder * pBuilder) override
	{
		SNfaFragment nfa;

		if(m_cMic > 0)
		{
			nfa = NfaCreateCount(pBuilder, m_cMic);
		}

		if(m_cMac == -1)
		{
			if(nfa.m_pStateBegin)
			{
				nfa.Patch(NfaCreateStar(pBuilder));
			}
			else
			{
				nfa = NfaCreateStar(pBuilder);
			}

			return nfa;
		}

		if(m_cMac <= m_cMic)
		{
			assert(nfa.m_pStateBegin);
			return nfa;
		}

		if(nfa.m_pStateBegin)
		{
			nfa.Patch(NfaCreateCountOptional(pBuilder, m_cMac - m_cMic));
		}
		else
		{
			nfa = NfaCreateCountOptional(pBuilder, m_cMac - m_cMic);
		}

		return nfa;
	}
};

struct SRange : SRegex // tag = range
{
	u8 m_chrMic;
	u8 m_chrMac;

	void PrintDebug() override
	{
		printf("(%d '%c' - %d '%c')", m_chrMic, m_chrMic, m_chrMac, m_chrMac);
	}

	SNfaFragment NfafragCreate(SNfaBuilder * pBuilder) override
	{
		SNfaState * pNfasOr = pBuilder->PNfasCreate();

		for(int iChr = m_chrMic; iChr <= m_chrMac; ++iChr)
		{
			SNfaState * pNfasChr = pBuilder->PNfasCreate();
			pNfasChr->AddTransition(iChr, nfasEmpty);

			pNfasOr->AddEpsilon(pNfasChr);
		}

		return SNfaFragment(pNfasOr, pNfasOr->m_aryEpsilon);
	}
};

struct SRegexChar : SRegex // tag = regexchr
{
	u8 m_chr;

	void PrintDebug() override
	{
		printf("%d '%c'", m_chr, m_chr);
	}

	SNfaFragment NfafragCreate(SNfaBuilder * pBuilder) override
	{
		SNfaState * pNfasChr = pBuilder->PNfasCreate();
		pNfasChr->AddTransition(m_chr, nfasEmpty);

		return SNfaFragment(pNfasChr, { pNfasChr });
	}
};

Pool<SRegex> g_poolRegex;