#pragma once

#include "bitary.h"
#include "regexparse.h"

struct CNfaState
{
	friend class CNfa;

public:

	CNfaState();
	
	const CDynBitAry * EpsilonClosure() const;

	void PrintDebug() const;

	const CNfaState* PStateTransition(u8 chr) const 
	{ return m_transitions[chr]; }

	int NId() const 
	{ return m_nId; }

private:

	void AddTransition(u8 chr, const CNfaState * pNfas);

	void AddEpsilon(CNfaState * pNfas);

	void Patch(CNfaState * pNfas);

	bool FUnpatched() const;

	void Bake(int cState);

	vector<CNfaState*> m_aryEpsilon;
	const CNfaState* m_transitions [256];
	bool m_fBaked;
	CDynBitAry m_eclosure;
	int m_nId;
};

class CNfa
{	
public:

	void Build(const SRegexAstNode * pRegex);

	const CNfaState * PStateStart() const 
	{ return m_pStateStart; }

	const CNfaState * PStateAccept() const 
	{ return m_pStateStart; }

	static const CNfaState * PStateEmpty()
	{ return &s_stateEmpty; }

	const CNfaState * PStateFromId(int nId) const;

	int CState() const
	{
		return m_poolNfas.m_arypT.size();
	}

private:

	struct SNfaFragment
	{
		void Patch(SNfaFragment nfa);

		SNfaFragment();

		SNfaFragment(CNfaState * pNfas, vector<CNfaState *> aryUnpatched);

		CNfaState * m_pStateBegin;
		vector<CNfaState *> m_aryUnpatched;
	};

	SNfaFragment FragFromRegex(const SRegexAstNode * pRegex);

	SNfaFragment FragFromUnion(const SUnionRegexData * pUnion);

	SNfaFragment FragFromConcat(const SConcatinationRegexData * pConcat);

	SNfaFragment FragFromQuant(const SQuantifierRegexData * pQuant);

	SNfaFragment FragFromRange(const SRangeRegexData * pRange);

	SNfaFragment FragFromRegexchr(const SChrRegexData * pRegexchr);

	SNfaFragment FragCreateCount(const SQuantifierRegexData * pQuant, int c);
	
	SNfaFragment FragCreateStar(const SQuantifierRegexData * pQuant);
	
	SNfaFragment FragCreateQMark(const SQuantifierRegexData * pQuant);
	
	SNfaFragment FragCreateCountOptional(const SQuantifierRegexData * pQuant, int c);
	
	CNfaState * PStateCreate();

	void Bake();

	CDynBitAry * PBAryCreate();

	Pool<CDynBitAry> m_poolBary;
	Pool<CNfaState> m_poolNfas;

	CNfaState * m_pStateStart;
	CNfaState * m_pStateEnd;

	static CNfaState s_stateEmpty;
};
