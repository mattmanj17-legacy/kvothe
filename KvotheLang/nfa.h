#pragma once

#include "bitary.h"
#include "regexparse.h"

struct CNfaState
{
	friend class CNfa;

public:

						CNfaState();

	void				PrintDebug() const;
	
	const CDynBitAry *	EpsilonClosure() const
							{ assert(m_fWasEpsilonClosureComputed); return &m_baryEpsilonClosure; }
	const CNfaState*	PStateTransition(u8 chr) const 
							{ return m_mpChrPStateTransition[chr]; }

	int					NId() const 
							{ return m_nId; }

private:

	void				AddChrTransition(u8 chr, const CNfaState * pNfas) 
							{ m_mpChrPStateTransition[chr] = pNfas; }
	void				AddEpsilonTransition(CNfaState * pNfas)
							{ m_arypStateTransitionEpsilon.push_back(pNfas); }

	void				Patch(CNfaState * pNfas);
	bool				FIsUnpatched() const;

	void				ComputeEpsilonClosure(int cState);

	vector<CNfaState*>	m_arypStateTransitionEpsilon;
	const CNfaState*	m_mpChrPStateTransition [256];

	bool				m_fWasEpsilonClosureComputed;
	CDynBitAry			m_baryEpsilonClosure;

	int					m_nId;
};

class CNfa
{	
public:

	void						PrintDebug();
	
	void						Build(const SRegexAstNode * pRegex);

	const CNfaState *			PStateStart() const 
									{ return m_pStateStart; }
	const CNfaState *			PStateAccept() const 
									{ return m_pStateStart; }

	const CNfaState *			PStateFromId(int nId) const
									{ return m_poolState.m_arypT[nId]; }

	int							CState() const
									{ return m_poolState.m_arypT.size(); }

	static const CNfaState *	PStateEmpty()
									{ return &s_stateEmpty; }
private:

	struct SNfaFragment
	{
								SNfaFragment();
								SNfaFragment(CNfaState * pNfas, vector<CNfaState *> aryUnpatched);

		void					Patch(SNfaFragment nfa);

		CNfaState *				m_pStateBegin;
		vector<CNfaState *>		m_arypStateUnpatched;
	};

	SNfaFragment				FragFromRegex(const SRegexAstNode * pRegex);
	SNfaFragment				FragFromUnion(const SUnionRegexData * pUnion);
	SNfaFragment				FragFromConcat(const SConcatinationRegexData * pConcat);
	SNfaFragment				FragFromQuant(const SQuantifierRegexData * pQuant);
	SNfaFragment				FragFromRange(const SRangeRegexData * pRange);
	SNfaFragment				FragFromChr(const SChrRegexData * pRegexchr);

	SNfaFragment				FragCreateCount(const SRegexAstNode * pRegex, size_t c);
	SNfaFragment				FragCreateStar(const SRegexAstNode * pRegex);
	SNfaFragment				FragCreateQMark(const SRegexAstNode * pRegex);
	SNfaFragment				FragCreateCountOptional(const SRegexAstNode * pRegex, size_t c);
	
	CNfaState *					PStateCreate();

	Pool<CNfaState>				m_poolState;

	CNfaState *					m_pStateStart;
	CNfaState *					m_pStateEnd;

	static CNfaState			s_stateEmpty;
};
