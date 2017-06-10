#pragma once

#include "dfa.h"
#include "types.h"

class CDfaMinimizer
{
public:
				CDfaMinimizer()
				: m_pPartStates(nullptr)
				, m_pPartTrans(nullptr)
				, m_cState(0)
				, m_cTran(0)
				, m_cStateFinal(0)
				, m_iStateInitial(0)
				, m_mpITranIStateFrom(nullptr)
				, m_mpITranLabel(nullptr)
				, m_mpITranIStateTo(nullptr)
				, m_aITran(nullptr)
				, m_mpIStateITranMic(nullptr)
				, m_cStateReached(0)
				{ ; }

	void		Minimize(const CDfa & dfaIn, CDfa & dfaOut);

protected:

	void		SetAdjacentTrans(int mpITranIState[]);								// make_adjacent
	void		MarkReached(int iState);											// reach
	void		RemoveUnreachable(int mpITranIStateFrom[], int mpITranIStateTo[]);	// rem_unreachable

	// Refinable partition 
	// an element of the partition is an int, meant to be an index.
	// in this case, an index either to a state or a tran

	struct SPartition // partition 
	{
					SPartition(int cElem);
					~SPartition();

		void		MarkElem(int iElem);		// mark
		void		Split();					// split
		
		int			m_cSet;						// z. number of sets in the partition 
		int*		m_aIElem;					// E. elements of the sets. the elements of s are E[f], E[f+1], ..., E[p-1], where f = F[s] and p= P[s]
		int*		m_mpIElemIElemInPartition;	// L. location of element. i.e., E[L[e]] = e, E[L[i]] = i 
		int*		m_mpIElemISet;				// S. the set that e belongs to. i.e., F[S[e]] <= L[e] < P[S[e]]
		int*		m_mpISetIElemMic;			// F. begining of set S
		int*		m_mpISetIElemMax;			// P. end of set S

		int*		m_mpISetCMarked;			// M. the number of marked elements in a set. numMarkedInS = M[s]
		int*		m_aISetTouched;				// W. touched sets and count of touched sets
		int			m_cSetTouched;				// w. ... 
	};

	SPartition*	m_pPartStates;			// B. partition of states (blocks)
	SPartition*	m_pPartTrans;			// C. partition of transitions (chords)

	int			m_cState;				// nn. number of states
	int			m_cTran;				// mm. number of transitions
	int			m_cStateFinal;			// ff. number of final states
	int			m_iStateInitial;		// q0. initial state
	int*		m_mpITranIStateFrom;	// T. tails of transitions
	u8*			m_mpITranLabel;			// L. labels of transitions
	int*		m_mpITranIStateTo;		// H. heads of transitions

	// Outgoing/incoming transitions from states
	// we are either storing the outgoing or incoming trans for each state
	// the adjacent trans of state s are A[F[s]], A[F[s] + 1], ..., A[F[s + 1] - 1]

	int*		m_aITran;				// A.
	int*		m_mpIStateITranMic;		// F.

	int			m_cStateReached = 0;	// rr. number of reached states. used for removing irrelevant states
};
