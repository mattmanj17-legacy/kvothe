#pragma once

#include "bitary.h"
#include "regexparse.h"
#include <string>
using std::string;

class CNfaState
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

	void				AddChrTransition(u8 chr, const CNfaState * pState) 
							{ m_mpChrPStateTransition[chr] = pState; }
	void				AddEpsilonTransition(CNfaState * pState)
							{ m_arypStateTransitionEpsilon.push_back(pState); }

	void				Patch(CNfaState * pState); // set dangling transitions to state
	bool				FIsUnpatched() const;

	void				ComputeEpsilonClosure(int cState); // cState is the number of states in our parent nfa

	vector<CNfaState*>	m_arypStateTransitionEpsilon;
	const CNfaState*	m_mpChrPStateTransition [256];

	bool				m_fWasEpsilonClosureComputed;
	CDynBitAry			m_baryEpsilonClosure;

	int					m_nId;
};

class CNfa
{	
public:

	void						PrintDebug() const;
	
	// convert a regex to an nfa

	void						Build(const SRegexAstNode * pRegex);

	const CNfaState *			PStateStart() const 
									{ return m_pStateStart; }
	const CNfaState *			PStateAccept() const 
									{ return m_pStateEnd; }

	const CNfaState *			PStateFromId(int nId) const
									{ return m_poolState.m_arypT[nId]; }

	int							CState() const
									{ return m_poolState.m_arypT.size(); }

	// BB(matthewd) this represents a dangling transition in CNfaState::m_mpChrPStateTransition
	// we can't use nullptr, as that signifies no transition

	static const CNfaState *	PStateEmpty()									
									{ return &s_stateEmpty; }
private:

	// a partially built nfa. a start state and a list of nodes with dangling transitions.
	
	struct SNfaFragment
	{
								SNfaFragment();
								SNfaFragment(CNfaState * pStateBegin, vector<CNfaState *> aryUnpatched);

		void					Patch(SNfaFragment frag); // set all dangling transitions the start state of another frag 

		CNfaState *				m_pStateBegin;
		vector<CNfaState *>		m_arypStateUnpatched;
	};

	// convert regexs to fragmens
	
	SNfaFragment				FragFromRegex(const SRegexAstNode * pRegex);
	SNfaFragment				FragFromUnion(const SUnionRegexData * pUnion);
	SNfaFragment				FragFromConcat(const SConcatinationRegexData * pConcat);
	SNfaFragment				FragFromQuant(const SQuantifierRegexData * pQuant);
	SNfaFragment				FragFromRange(const SRangeRegexData * pRange);
	SNfaFragment				FragFromChr(const SChrRegexData * pRegexchr);

	// helper functions used by FragFromQuant

	SNfaFragment				FragCreateCount(const SRegexAstNode * pRegex, size_t c);			// concatinate regex to itself c times
	SNfaFragment				FragCreateStar(const SRegexAstNode * pRegex);						// regex 0 or more times
	SNfaFragment				FragCreateQMark(const SRegexAstNode * pRegex);						// regex 0 or 1 time
	SNfaFragment				FragCreateCountOptional(const SRegexAstNode * pRegex, size_t c);	// regex between 0 and c times
	
	CNfaState *					PStateCreate();	// create a new state in the state pool

	Pool<CNfaState>				m_poolState;	// pool for allocating states

	CNfaState *					m_pStateStart;	// the start and end states created in Build
	CNfaState *					m_pStateEnd;	// ...

	static CNfaState			s_stateEmpty;	// see comment on PStateEmpty
};

void MatchNfa(string str, const CNfa & nfa);
