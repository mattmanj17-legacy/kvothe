#pragma once

//#include "dfa.h"

class CDfaMinimizer
{
public:

	void DfaMinMain();

protected:

	// Refinable partition 
	// an element of the partition is an int, meant to be an index.
	// in this case, an index either to a state or a tran

	struct partition
	{
		void	init(int cElem);
		void	mark(int iElem);
		void	split();
		
		int		m_cSet;							// number of sets in the partition 
		int*	m_aIElem;						// elements of the sets. the elements of s are E[f], E[f+1], ..., E[p-1], where f = F[s] and p= P[s]
		int*	m_mpIElemIElemInPartition;		// location of element. i.e., E[L[e]] = e, E[L[i]] = i 
		int*	m_mpIElemISet;					// the set that e belongs to. i.e., F[S[e]] <= L[e] < P[S[e]]
		int*	m_mpISetIElemMic;				// begining of set S
		int*	m_mpISetIElemMax;				// end of set S

		static int*	g_mpISetCMarked;			// the number of marked elements in a set. numMarkedInS = M[s]

		static int*	g_aISetTouched;				// touched sets and count of touched sets
		static int	g_cSetTouched;				// ... 
	};

	partition g_partStates;			// partition of states (blocks)
	partition g_partTrans;			// partition of transitions (chords)

	int		g_cState;				// number of states
	int		g_cTran;				// number of transitions
	int		g_cStateFinal;			// number of final states
	int		g_iStateInitial;		// initial state
	int*	g_mpITranIStateFrom;	// tails of transitions
	int*	g_mpITranLabel;			// labels of transitions
	int*	g_mpITranIStateTo;		// heads of transitions

	// Outgoing/incoming transitions from states
	// we are either storing the outgoing or incoming trans for each state
	// the outgoing trans of state s are A[F[s]], A[F[s] + 1], ..., A[F[s + 1] - 1]

	int* g_aITran; 
	int* g_mpIStateITranMic;

	void SetAdjacentTrans(int mpITranIState[]);

	int g_cStateReached = 0; // number of reached states

	void MarkReached(int iState);

	void RemoveUnreachable(int mpITranIStateFrom[], int mpITranIStateTo[]);
};
