#pragma once

#include <queue>
using std::queue;

#include "bitary.h"
#include "macros.h"
#include "regexparse.h"

struct SNfaState;

SNfaState * nfasEmpty;

struct SNfaState
{	
	SNfaState()
	: m_aryEpsilon()
	, m_transitions()
	, m_fBaked(false)
	, m_eclosure()
	, m_nId(-1)
	{
		memset(m_transitions, 0, DIM(m_transitions) * sizeof(SNfaState *));
	}

	void PrintDebug()
	{
		printf("\n");
		printf("state %d:\n", m_nId);
	
		printf("transitions:");

		for(size_t iChr = 0; iChr < DIM(m_transitions); ++iChr)
		{
			if(m_transitions[iChr])
			{
				printf(" ");
				printf("%d '%c' -> %d", iChr, iChr, m_transitions[iChr]->m_nId);
			}
		}
		printf("\n");

		if(m_aryEpsilon.size() > 0)
		{
			printf("epsilon:");
			for(SNfaState * nfas : m_aryEpsilon)
			{
				assert(nfas);
				printf(" %d", nfas->m_nId);
			}
			printf("\n");
		}
	}

	void AddTransition(u8 chr, SNfaState * pNfas)
	{
		m_transitions[chr] = pNfas;
	}

	void AddEpsilon(SNfaState * pNfas)
	{
		m_aryEpsilon.push_back(pNfas);
	}

	void Patch(SNfaState * pNfas)
	{
		for(size_t iEpsilon = 0; iEpsilon < m_aryEpsilon.size(); ++iEpsilon)
		{
			if(!m_aryEpsilon[iEpsilon])
			{
				m_aryEpsilon[iEpsilon] = pNfas;
			}
		}

		for(size_t iChr = 0; iChr < DIM(m_transitions); ++iChr)
		{
			if(m_transitions[iChr] == nfasEmpty)
			{
				m_transitions[iChr] = pNfas;
			}
		}
	}

	bool FUnpatched()
	{
		// BB (matthewd) cache this value instead?
		
		for(size_t iEpsilon = 0; iEpsilon < m_aryEpsilon.size(); ++iEpsilon)
		{
			if(!m_aryEpsilon[iEpsilon])
				return true;
		}

		for(size_t iChr = 0; iChr < DIM(m_transitions); ++iChr)
		{
			if(m_transitions[iChr] == nfasEmpty)
				return true;
		}

		return false;
	}

	void Bake(int cState)
	{
		assert(!m_fBaked);
		
		m_fBaked = true;

		m_eclosure.SetSize(cState);

		// breadth first search for nodes we can reach by 0 or more epsilon transitions
		
		queue<SNfaState*> qpNfas;
		qpNfas.push(this);

		while(qpNfas.size() > 0)
		{
			SNfaState* pNfas = qpNfas.front();
			qpNfas.pop();

			if(!m_eclosure.At(pNfas->m_nId))
			{
				m_eclosure.Set(pNfas->m_nId);
			
				for(SNfaState* pStateEpsilon : pNfas->m_aryEpsilon)
				{
					qpNfas.push(pStateEpsilon);
				}
			}
		}
	}

	CDynBitAry * EClosure()
	{
		assert(m_fBaked);
		
		return &m_eclosure;
	}

	vector<SNfaState*> m_aryEpsilon;
	SNfaState* m_transitions [256];
	bool m_fBaked;
	CDynBitAry m_eclosure;
	int m_nId;
};

struct SNfaFragment
{
	SNfaState * m_pStateBegin;
	vector<SNfaState *> m_aryUnpatched;

	void Patch(SNfaFragment nfa)
	{
		for(SNfaState * Nfas : m_aryUnpatched)
		{
			Nfas->Patch(nfa.m_pStateBegin);
		}

		m_aryUnpatched.clear();

		// we cant just set m_arypStateUnpatched = nfa.m_arypStateUnpatched,
		// because m_arypStateUnpatched and nfa.m_arypStateUnpatched are not garenteed to be disjoint
		// so we may have already patched some of the nodes in nfa.m_arypStateUnpatched
		// BB (matthewd) hmm.... i dont like this
		
		for(SNfaState * Nfas : nfa.m_aryUnpatched)
		{
			if(Nfas->FUnpatched())
			{
				m_aryUnpatched.push_back(Nfas);
			}
		}
	}

	SNfaFragment()
	: m_pStateBegin(nullptr)
	, m_aryUnpatched()
	{
	}

	SNfaFragment(SNfaState * pNfas, vector<SNfaState *> aryUnpatched)
	: m_pStateBegin(pNfas)
	, m_aryUnpatched(aryUnpatched)
	{
	}
};

struct SNfaBuilder
{	
	SNfaFragment NfafragFromRegex(SRegexAstNode regex)
	{
		switch (regex.m_regexk)
		{
			case REGEXK_Union:
				return NfafragFromUnion(regex.m_pUnionData);
				break;

			case REGEXK_Concat:
				return NfafragFromConcat(regex.m_pConcatData);
				break;

			case REGEXK_Quant:
				return NfafragFromQuant(regex.m_pQuantData);
				break;

			case REGEXK_Range:
				return NfafragFromRange(regex.m_pRangeData);
				break;

			case REGEXK_Chr:
				return NfafragFromRegexchr(regex.m_pChrData);
				break;

			default:
				assert(false);
				return SNfaFragment();
				break;
		}
	}

	SNfaFragment NfafragFromUnion(SUnionRegexData * pUnion)
	{
		assert(pUnion->m_aryRegex.size() > 0);
	
		SNfaState * pNfasOr = PNfasCreate();
	
		vector<SNfaState *> aryUnpatched;
	
		for(SRegexAstNode regex : pUnion->m_aryRegex)
		{
			SNfaFragment nfaAlt = NfafragFromRegex(regex);
			aryUnpatched.insert(aryUnpatched.end(), nfaAlt.m_aryUnpatched.begin(), nfaAlt.m_aryUnpatched.end());
			pNfasOr->AddEpsilon(nfaAlt.m_pStateBegin);
		}
	
		return SNfaFragment(pNfasOr, aryUnpatched);
	}

	SNfaFragment NfafragFromConcat(SConcatinationRegexData * pConcat)
	{
		assert(pConcat->m_aryRegex.size() > 0);
	
		SNfaFragment nfa = NfafragFromRegex(pConcat->m_aryRegex[0]);
	
		for(size_t i = 1; i < pConcat->m_aryRegex.size(); ++i)
		{
			nfa.Patch(NfafragFromRegex(pConcat->m_aryRegex[i]));
		}
	
		return nfa;
	}

	SNfaFragment NfafragFromQuant(SQuantifierRegexData * pQuant)
	{
		SNfaFragment nfa;
	
		if(pQuant->m_cMic > 0)
		{
			nfa = NfaCreateCount(pQuant, pQuant->m_cMic);
		}
	
		if(pQuant->m_cMac == -1)
		{
			if(nfa.m_pStateBegin)
			{
				nfa.Patch(NfaCreateStar(pQuant));
			}
			else
			{
				nfa = NfaCreateStar(pQuant);
			}
	
			return nfa;
		}
	
		if(pQuant->m_cMac <= pQuant->m_cMic)
		{
			assert(nfa.m_pStateBegin);
			return nfa;
		}
	
		if(nfa.m_pStateBegin)
		{
			nfa.Patch(NfaCreateCountOptional(pQuant, pQuant->m_cMac - pQuant->m_cMic));
		}
		else
		{
			nfa = NfaCreateCountOptional(pQuant, pQuant->m_cMac - pQuant->m_cMic);
		}
	
		return nfa;
	}

	SNfaFragment NfafragFromRange(SRangeRegexData * pRange)
	{
		SNfaState * pNfasOr = PNfasCreate();
	
		for(int iChr = pRange->m_chrMic; iChr <= pRange->m_chrMac; ++iChr)
		{
			SNfaState * pNfasChr = PNfasCreate();
			pNfasChr->AddTransition(iChr, nfasEmpty);
	
			pNfasOr->AddEpsilon(pNfasChr);
		}
	
		return SNfaFragment(pNfasOr, pNfasOr->m_aryEpsilon);
	}

	SNfaFragment NfafragFromRegexchr(SChrRegexData * pRegexchr)
	{
		SNfaState * pNfasChr = PNfasCreate();
		pNfasChr->AddTransition(pRegexchr->m_chr, nfasEmpty);
	
		return SNfaFragment(pNfasChr, { pNfasChr });
	}

	SNfaFragment NfaCreateCount(SQuantifierRegexData * pQuant, int c)
	{
		SConcatinationRegexData concat;
	
		for(int iC = 0; iC < c; ++ iC)
		{
			concat.m_aryRegex.push_back(pQuant->m_regex);
		}
	
		return NfafragFromConcat(&concat);
	}
	
	SNfaFragment NfaCreateStar(SQuantifierRegexData * pQuant)
	{
		// create a ?
		
		SNfaState * pNfasOr = PNfasCreate();
		SNfaFragment nfa = NfafragFromRegex(pQuant->m_regex);
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
	
	SNfaFragment NfaCreateQMark(SQuantifierRegexData * pQuant)
	{
		// create a ?
		
		SNfaState * pNfasOr = PNfasCreate();
		SNfaFragment nfa = NfafragFromRegex(pQuant->m_regex);
		pNfasOr->AddEpsilon(nfa.m_pStateBegin);
		pNfasOr->AddEpsilon(nullptr);
	
		// the unpatched states are pOrState and all the unpatched states of
		// the nfa created from m_pRegex
	
		vector<SNfaState *> aryUnpatched;
		aryUnpatched.push_back(pNfasOr);
		aryUnpatched.insert(aryUnpatched.end(), nfa.m_aryUnpatched.begin(), nfa.m_aryUnpatched.end());
	
		return SNfaFragment(pNfasOr, aryUnpatched);
	}
	
	SNfaFragment NfaCreateCountOptional(SQuantifierRegexData * pQuant, int c)
	{
		assert(c > 0);
	
		// create nested ?'s, starting on the right and working
		// back to the left to the starting node
		
		SNfaFragment nfaLeftMost;
	
		for(int iC = c; iC > 0; --iC)
		{
			SNfaFragment nfaQMark = NfaCreateQMark(pQuant);
	
			if(nfaLeftMost.m_pStateBegin)
			{
				nfaQMark.Patch(nfaLeftMost);
			}
	
			nfaLeftMost = nfaQMark;
		}
	
		return nfaLeftMost;
	}
	
	SNfaState * PNfasCreate()
	{
		SNfaState * pNfas = m_poolNfas.PTNew<SNfaState>();

		pNfas->m_nId = m_poolNfas.m_arypT.size() - 1;

		return pNfas;
	}

	void Bake()
	{
		for(SNfaState * pNfas : m_poolNfas.m_arypT)
		{
			pNfas->Bake(m_poolNfas.m_arypT.size());
		}
	}

	SNfaState * PNfasFromId(int nId)
	{
		return m_poolNfas.m_arypT[nId];
	}

	CDynBitAry * PBAryCreate()
	{
		CDynBitAry * pBary = m_poolBary.PTNew<CDynBitAry>();
		pBary->SetSize(m_poolNfas.m_arypT.size());
		return pBary;
	}

	CDynBitAry * PBAryCreateUnpooled()
	{
		CDynBitAry * pBary = new CDynBitAry();
		pBary->SetSize(m_poolNfas.m_arypT.size());
		return pBary;
	}

	void AddToPool(CDynBitAry * pBary)
	{
		m_poolBary.m_arypT.push_back(pBary);
	}

	Pool<CDynBitAry> m_poolBary;
	Pool<SNfaState> m_poolNfas;
};
