#pragma once

#include "bitary.h"
#include "pool.h"
#include "nfa.h"

struct SDfaState // tag = dfas
{
				SDfaState();

	void		PrintDebug();
	
	SDfaState*	m_transitions [256];
	int			m_nId;
	bool		m_fIsFinal;
};

class CDfa
{
	friend class CDfaMinimizer;

public:
						CDfa();

	void				PrintDebug() const;

	void				Build(const CNfa * pNfa);
	
	const SDfaState *	PStateStart() const
							{ return m_pStateStart; }
private:

	Pool<SDfaState>		m_poolState;	// pool for allocating states
	SDfaState *			m_pStateStart;	// the start state created in Build
};

void MatchDfa(string str, const CDfa & dfa);

