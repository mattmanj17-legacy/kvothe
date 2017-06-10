#include "dfamin.h"

/* Code below reproduced from "Fast brief practical DFA minimization" by Antti Valmari (2011) */
// https://www.cs.cmu.edu/~cdm/pdf/Valmari12.pdf

#include <iostream>
#include <algorithm> 
#include <assert.h>

// Main program

void CDfaMinimizer::DfaMinMain() 
{
	// Read sizes and reserve most memory
	
	// std::cin >> nn >> mm >> q0 >> ff;
	std::cin >> m_cState >> m_cTran >> m_iStateInitial >> m_cStateFinal;

	m_mpITranIStateFrom = new int[m_cTran]; // T = new int[mm];
	m_mpITranLabel = new int[m_cTran]; // L = new int[mm];
	m_mpITranIStateTo = new int[m_cTran]; // H = new int[mm];
	m_pPartStates = new SPartition(m_cState); // B.init(nn);
	m_aITran = new int[m_cTran]; // A = new int[mm];
	m_mpIStateITranMic = new int[m_cState + 1]; // F = new int[nn + 1];

	// Read transitions

	for (int iTran = 0; iTran < m_cTran; ++iTran) // for (int t = 0; t < mm; ++t)
	{
		// std::cin >> T[t] >> L[t] >> H[t];
		std::cin >> m_mpITranIStateFrom[iTran] >> m_mpITranLabel[iTran] >> m_mpITranIStateTo[iTran];
	}

	// Remove states that cannot be reached from the initial state

	MarkReached(m_iStateInitial); // reach(q0);
	RemoveUnreachable(m_mpITranIStateFrom, m_mpITranIStateTo); // rem_unreachable(T, H);

	for (int cStateFinal = 0; cStateFinal < m_cStateFinal; ++cStateFinal) // for (int i = 0; i < ff; ++i)
	{
		// read in final state
		
		// int q; 
		int iStateFinal;

		// std::cin >> q;
		std::cin >> iStateFinal; 

		// mark final state as reached (if it is reachable from the start state)

		// if (B.L[q] < B.P[0])
		if (m_pPartStates->m_mpIElemIElemInPartition[iStateFinal] < m_pPartStates->m_mpISetIElemMax[0]) 
		{ 
			// note : this also moves this final state to the front of the array
			
			// reach(q);
			MarkReached(iStateFinal); 
		}
	}
	
	// ff = rr;
	m_cStateFinal = m_cStateReached; 

	// Remove states from which final states cannot be reached 

	// rem_unreachable(H, T);
	RemoveUnreachable(m_mpITranIStateTo, m_mpITranIStateFrom);

	// mark the final states
	// this works because the final states will be at the front of the array
	// after we are done removing irrelevant states

	// M[0] = ff;
	m_pPartStates->m_mpISetCMarked[0] = m_cStateFinal;

	// if g_cStateFinal == 0, then this is the empty language and has no states
	
	if (m_cStateFinal > 0) // if (ff)
	{ 
		// W[w++] = 0;
		m_pPartStates->m_aISetTouched[m_pPartStates->m_cSetTouched] = 0; 
		++m_pPartStates->m_cSetTouched;

		// split into final and non final states

		// B.split();
		m_pPartStates->Split(); 
	}

	// Make transition partition

	// C.init(mm);
	m_pPartTrans = new SPartition(m_cTran);

	// if g_cTran == 0, then this is the empty language and has no transitions

	// if (mm)
	if (m_cTran > 0) 
	{
		// we need the transisions with the same labels to be next to each other, so sort them
		// BB (matthewd) hmmm... this could be a counting sort for slightly better run time, because the labels will only be u8's

		// td::sort(C.E, C.E + mm, cmp);
		std::sort(
			m_pPartTrans->m_aIElem, 
			m_pPartTrans->m_aIElem + m_cTran, 
			[this](int iTran0, int iTran1)
			{
				return m_mpITranLabel[iTran0] < m_mpITranLabel[iTran1];
			}
		);

		// partition the transitions into sets containing one label each

		// we are screwing with the sets manually, so reset g_partTrans.m_cSet
		// it will be incremented for each label
		
		// C.z = 0;
		m_pPartTrans->m_cSet = 0;

		// make sure set 0 is init'ed

		// M[0] = 0;
		m_pPartTrans->m_mpISetCMarked[0] = 0;

		// get the label of the first transition
		
		int iTranFirst = m_pPartTrans->m_aIElem[0];

		// int a = L[C.E[0]];
		int label = m_mpITranLabel[iTranFirst];

		// for each transition...

		for (int cTran = 0; cTran < m_cTran; ++cTran) // for (int i = 0; i < mm; ++i)
		{
			// int t = C.E[i];
			int iTran = m_pPartTrans->m_aIElem[cTran];

			// if this transition has a different label than the current one, start a new set
			
			if (m_mpITranLabel[iTran] != label) // if (L[t] != a)
			{
				// set the label of the new set
				
				// a = L[t];
				label = m_mpITranLabel[iTran]; 

				// create the new set
				// sets its Mic and Max, and inc g_partTrans.m_cSet

				// C.P[C.z++] = i;
				m_pPartTrans->m_mpISetIElemMax[m_pPartTrans->m_cSet] = cTran;
				++m_pPartTrans->m_cSet;

				// C.F[C.z] = i;
				m_pPartTrans->m_mpISetIElemMic[m_pPartTrans->m_cSet] = cTran; 

				// init the marked count for this new set

				// M[C.z] = 0;
				m_pPartTrans->m_mpISetCMarked[m_pPartTrans->m_cSet] = 0;
			}

			// put this transition in the current set

			m_pPartTrans->m_mpIElemISet[iTran] = m_pPartTrans->m_cSet; 
			m_pPartTrans->m_mpIElemIElemInPartition[iTran] = cTran;
		}

		// "close off" the last set. set its max and inc g_partTrans.m_cSet
		
		// C.P[C.z++] = mm;
		m_pPartTrans->m_mpISetIElemMax[m_pPartTrans->m_cSet] = m_cTran;
		++m_pPartTrans->m_cSet;
	}

	// Split blocks and cords
	// the meat of the minimization algorithm
	// for explanation/proof, see section 3, "correctness and speed of the splitting stage"

	// make_adjacent(H);
	SetAdjacentTrans(m_mpITranIStateTo);

	// this index has to be outside the inner loop, because we need to retain it's value between iterations
	
	// int b = 1
	int iSetState = 1; 

	// for each label...

	for(int iSetTrans = 0; iSetTrans < m_pPartTrans->m_cSet; ++iSetTrans) // while (c < C.z) ... ++c; ...
	{
		// for each transition with this label...
		
		int iTranInPartitionMic = m_pPartTrans->m_mpISetIElemMic[iSetTrans];
		int iTranInPartitionMax = m_pPartTrans->m_mpISetIElemMax[iSetTrans];
		
		// for (i = C.F[c]; i < C.P[c]; ++i)
		for (int iTranInPartition = iTranInPartitionMic; iTranInPartition < iTranInPartitionMax; ++iTranInPartition) 
		{
			// mark this transition's from state
			
			// B.mark(T[C.E[i]]);
			int iTran = m_pPartTrans->m_aIElem[iTranInPartition];
			int iState = m_mpITranIStateFrom[iTran];
			m_pPartStates->MarkElem(iState);
		}

		// split the states
		// this will possibly increase g_partStates.m_cSet,
		// so we will enter the loop below again
		
		// B.split();
		m_pPartStates->Split();

		// ++c; is done by the for loop

		// for every set of states we have not touched yet (possibly created by the previous split)...

		for(; iSetState < m_pPartStates->m_cSet; ++iSetState) // while (b < B.z) ... ++b;
		{
			// for every state in this set...
			
			int iStateInPartitionMic = m_pPartStates->m_mpISetIElemMic[iSetState];
			int iStateInPartitionMax = m_pPartStates->m_mpISetIElemMax[iSetState];

			// for (i = B.F[b]; i < B.P[b]; ++i)
			for (int iStateInPartition = iStateInPartitionMic; iStateInPartition < iStateInPartitionMax; ++iStateInPartition) 
			{
				// because we did SetAdjacentTrans(g_mpITranIStateTo) above, g_aITran has incoming transitions
				// for every incoming transition...

				int iState = m_pPartStates->m_aIElem[iStateInPartition];

				int iTranIncomingMic = m_mpIStateITranMic[iState];
				int iTranIncomingMax = m_mpIStateITranMic[iState + 1];

				/*
					for (
						j = F[B.E[i]];
						j < F[B.E[i] + 1]; 
						++j
					)
				*/
				for (int iTranIncoming = iTranIncomingMic; iTranIncoming < iTranIncomingMax; ++iTranIncoming) 
				{
					// mark this incoming transition
					
					// C.mark(A[j]);
					int iTran = m_aITran[iTranIncoming];
					m_pPartTrans->MarkElem(iTran);
				}
			}

			// split the transitions (possibly making g_partTrans.m_cSet go up)


			// C.split();
			m_pPartTrans->Split(); 
		}
	}

	// Count the numbers of transitions in the result
	// for each transition...

	// int mo = 0
	int cTranResult = 0; 
	for (int cTran = 0; cTran < m_cTran; ++cTran) // for (int t = 0; t < mm; ++t) 
	{
		int iStateFrom = m_mpITranIStateFrom[cTran];
		int iSet = m_pPartStates->m_mpIElemISet[iStateFrom];

		int iStateInPartitionFrom = m_pPartStates->m_mpIElemIElemInPartition[iStateFrom];
		int iStateInPartitionMic = m_pPartStates->m_mpISetIElemMic[iSet];

		// if this transision's from state is the first state in a block, inc cTranResult
		// this works because for all states in a block, they all transition to the same block on a given label
		// so we are effectivly combining redundant transitions from on block to another
		
		// if (B.L[T[t]] == B.F[B.S[T[t]]])
		if (iStateInPartitionFrom == iStateInPartitionMic) 
		{
			// ++mo;
			++cTranResult;
		}
	}

	// Count the numbers of final states in the result
	// for each set (block of states)

	// int fo = 0;
	int cStateFinalResult = 0;
	for (int cSet = 0; cSet < m_pPartStates->m_cSet; ++cSet) // for (int b = 0; b < B.z; ++b)
	{
		// if the first state in this set is final, then all states in this set are final 
		// so this set if final, so inc cStateFinalResult
		
		bool fIsSetFinal = m_pPartStates->m_mpISetIElemMic[cSet] < m_cStateFinal;

		if (fIsSetFinal) // if (B.F[b] < ff) 
		{ 
			// ++fo;
			++cStateFinalResult; 
		}
	}

	// Print the result

	// std::cout << B.z << ' ' << mo << ' ' << B.S[q0] << ' ' << fo << '\n';

	// print cState

	std::cout << m_pPartStates->m_cSet; 
	std::cout << ' '; 

	// print cTran

	std::cout << cTranResult; 
	std::cout << ' '; 

	// print iStateInitial

	std::cout << m_pPartStates->m_mpIElemISet[m_iStateInitial]; 
	std::cout << ' '; 

	 // print cStateFinal

	std::cout << cStateFinalResult;
	std::cout << '\n';

	// for each transition

	for (int cTran = 0; cTran < m_cTran; ++cTran) // for (int t = 0; t < mm; ++t)
	{
		int iStateFrom = m_mpITranIStateFrom[cTran];
		int iSetFrom = m_pPartStates->m_mpIElemISet[iStateFrom];

		int label = m_mpITranLabel[cTran];

		int iStateTo = m_mpITranIStateTo[cTran];
		int iSetTo = m_pPartStates->m_mpIElemISet[iStateTo];

		int iStateInPartitionFrom = m_pPartStates->m_mpIElemIElemInPartition[iStateFrom];
		int iStateInPartitionMic = m_pPartStates->m_mpISetIElemMic[iSetFrom];
		
		// if this transision's from state is the first state in a block, print this transition
		// see note above

		// if (B.L[T[t]] == B.F[B.S[T[t]]])
		if (iStateInPartitionFrom == iStateInPartitionMic) 
		{
			// std::cout << B.S[T[t]] << ' ' << L[t] << ' ' << B.S[H[t]] << '\n';
			
			std::cout << iSetFrom; 
			std::cout << ' '; 
			std::cout << label;
			std::cout << ' '; 
			std::cout << iSetTo;
			std::cout << '\n';
		}
	}

	// for each set of states

	// for (int b = 0; b < B.z; ++b)
	for (int cSet = 0; cSet < m_pPartStates->m_cSet; ++cSet) 
	{
		// if this set is final, print it
		
		bool fIsSetFinal = m_pPartStates->m_mpISetIElemMic[cSet] < m_cStateFinal;

		// if (B.F[b] < ff)
		if (fIsSetFinal) 
		{
			// std::cout << b << '\n';
			std::cout << cSet;
			std::cout << '\n';
		}
	}

	delete [] m_mpITranIStateFrom;
	m_mpITranIStateFrom = nullptr;

	delete [] m_mpITranLabel;
	m_mpITranLabel = nullptr;

	delete [] m_mpITranIStateTo;
	m_mpITranIStateTo = nullptr;

	delete [] m_aITran;
	m_aITran = nullptr;

	delete [] m_mpIStateITranMic;
	m_mpIStateITranMic = nullptr;

	delete m_pPartStates;
	m_pPartStates = nullptr;

	delete m_pPartTrans;
	m_pPartTrans = nullptr;
}

CDfaMinimizer::SPartition::SPartition(int cElem) 
{ 
	// init the data structure as one set containing n elements
	
	// z = bool(n);
	assert(cElem > 0);
	m_cSet = 1;

	m_aIElem = new int[cElem]; // E = new int[n];
	m_mpIElemIElemInPartition = new int[cElem]; // L = new int[n];
	m_mpIElemISet = new int[cElem]; // S = new int[n];
	m_mpISetIElemMic = new int[cElem]; // F = new int[n];
	m_mpISetIElemMax = new int[cElem]; // P = new int[n];

	// for (int i = 0; i < n; ++i)
	for (int iElem = 0; iElem < cElem; ++iElem) 
	{
		m_aIElem[iElem] = iElem; // E[i] = i;
		m_mpIElemIElemInPartition[iElem] = iElem; // L[i] = i;
		m_mpIElemISet[iElem] = 0; // S[i] = 0;
	}

	// if (z), will alway be true because we assert(cElem > 0);

	m_mpISetIElemMic[0] = 0; // F[0] = 0;
	m_mpISetIElemMax[0] = cElem; // P[0] = n;

	m_mpISetCMarked = new int[cElem];
	m_aISetTouched = new int[cElem];
	m_cSetTouched = 0;
}

CDfaMinimizer::SPartition::~SPartition()
{
	delete m_aIElem;
	delete m_mpIElemIElemInPartition;
	delete m_mpIElemISet;
	delete m_mpISetIElemMic;
	delete m_mpISetIElemMax;
	delete m_mpISetCMarked;
	delete m_aISetTouched;
}

void CDfaMinimizer::SPartition::MarkElem(int iElem) 
{
	// refinement of the partition consists or marking some elements and then calling split

	// int s = S[e],
	int iSet = m_mpIElemISet[iElem]; 

	// mark this set as touched if this is the first element in it we are marking
	
	// if (!M[s]) . we do M[s]++ below
	if(m_mpISetCMarked[iSet] == 0)
	{
		// W[w++] = s;
		m_aISetTouched[m_cSetTouched] = iSet;
		++m_cSetTouched;
	}

	// i = L[e]
	int iElemInPartition = m_mpIElemIElemInPartition[iElem]; 

	// the segment in E the represents S lists its marked elements first.
	// so, the index of first unmarked element is F[s] + M[s]

	// j = F[s] + M[s];
	int iElemFirstUnmarked = m_mpISetIElemMic[iSet] + m_mpISetCMarked[iSet];

	// mark elem by swapping it with the first unmarked element and incrementing cMarked
	
	m_aIElem[iElemInPartition] = m_aIElem[iElemFirstUnmarked]; // E[i] = E[j];
	m_mpIElemIElemInPartition[m_aIElem[iElemInPartition]] = iElemInPartition; // L[E[i]] = i;

	m_aIElem[iElemFirstUnmarked] = iElem; // E[j] = e;
	m_mpIElemIElemInPartition[iElem] = iElemFirstUnmarked; // L[e] = j;

	// M[s]++
	++m_mpISetCMarked[iSet];
}

void CDfaMinimizer::SPartition::Split() 
{
	// for each touched set...

	// while (w)
	while (m_cSetTouched) 
	{
		--m_cSetTouched;

		// int s = W[--w],
		int iSetTouched = m_aISetTouched[m_cSetTouched];

		// j = F[s] + M[s];
		int	iElemFirstUnmarked = m_mpISetIElemMic[iSetTouched] + m_mpISetCMarked[iSetTouched];

		// P[s]
		int iElemMax = m_mpISetIElemMax[iSetTouched];

		// if (j == P[s])
		if (iElemFirstUnmarked == iElemMax) 
		{

			// if all elements are marked, just un mark them and go to the next set
			// because we are only looking at touched sets, we know at least one element is marked

			m_mpISetCMarked[iSetTouched] = 0; // M[s] = 0;
			continue; 
		}

		// some elements are unmarked
		// because we are only looking at touched sets, we know at least one element is marked
		// so there are both marked and unmarked elements
		// we will split them into two sets

		// chose the smaller of the marked and unmarked elements in this set, and make it the new set
		// the current set remains, only having the bigger part (either marked or unmarked)
		// always choosing the smaller set makes it possible to avoid a significant complication
		// later in the program

		int cUnmarked = iElemMax - iElemFirstUnmarked;
		
		// if (M[s] <= P[s] - j)
		if (m_mpISetCMarked[iSetTouched] <= cUnmarked) 
		{
			// the new set is the marked elements 
			
			m_mpISetIElemMic[m_cSet] = m_mpISetIElemMic[iSetTouched]; // F[z] = F[s];
			m_mpISetIElemMax[m_cSet] = iElemFirstUnmarked; // P[z] = j;

			// move the begining of the current set to the first unmarked element

			m_mpISetIElemMic[iSetTouched] = iElemFirstUnmarked; // F[s] = j;
		}
		else 
		{
			// the new set is the unmarked elements 

			m_mpISetIElemMax[m_cSet] = m_mpISetIElemMax[iSetTouched]; // P[z] = P[s];
			m_mpISetIElemMic[m_cSet] = iElemFirstUnmarked; // F[z] = j;

			// move the end of the current set to the last marked element

			m_mpISetIElemMax[iSetTouched] = iElemFirstUnmarked; // P[s] = j;
		}

		// for (int i = F[z]; i < P[z]; ++i) 
		for (int iiElem = m_mpISetIElemMic[m_cSet]; iiElem < m_mpISetIElemMax[m_cSet]; ++iiElem) 
		{
			// "move" the elements to the new set

			// S[E[i]] = z;
			m_mpIElemISet[m_aIElem[iiElem]] = m_cSet;
		}

		// unmark the elements in the current set

		// M[s] = 0;
		m_mpISetCMarked[iSetTouched] = 0;

		// the new set begins untouched (all elements unmarked)

		// M[z++] = 0;
		m_mpISetCMarked[m_cSet] = 0;
		++m_cSet;
	}
}

void CDfaMinimizer::SetAdjacentTrans(int mpITranIState[]) 
{
	// note : this is effectivly a counting sort
	
	// clear g_mpIStateITranOutgoingMic
	
	// for (q = 0; q <= nn; ++q)
	for (int iState = 0; iState <= m_cState; ++iState) 
	{ 
		m_mpIStateITranMic[iState] = 0; // F[q] = 0;
	}

	// count the transitions for each state

	// or (t = 0; t < mm; ++t)
	for (int iTran = 0; iTran < m_cTran; ++iTran) 
	{ 
		// ++F[K[t]];
		int iState = mpITranIState[iTran];
		++m_mpIStateITranMic[iState]; 
	}

	// "shift" each set of outgoing trans to be after the previous one

	// for (q = 0; q < nn; ++q)
	for (int iState = 0; iState < m_cState; ++iState)
	{
		// F[q + 1] += F[q];
		m_mpIStateITranMic[iState + 1] += m_mpIStateITranMic[iState];
	}

	// for (t = mm; t--; )
	for (int iTran = m_cTran - 1; iTran >= 0; --iTran) 
	{ 
		int iState = mpITranIState[iTran];
		
		// shift down by one (so we can be zero indexed)

		--m_mpIStateITranMic[iState];

		// set g_aITran

		int iTranMic = m_mpIStateITranMic[iState];
		
		// A[--F[K[t]]] = t;
		m_aITran[iTranMic] = iTran; 
	}
}

inline void CDfaMinimizer::MarkReached(int iState) 
{ 	
	// reach works like mark, except it does not write into m_mpISetCMarked

	int iStateInPartition = m_pPartStates->m_mpIElemIElemInPartition[iState]; // int i = B.L[q];
	if (iStateInPartition >= m_cStateReached) // if (i >= rr)
	{
		int iStateReached = m_pPartStates->m_aIElem[m_cStateReached]; // B.E[rr]

		m_pPartStates->m_aIElem[iStateInPartition] = iStateReached; // B.E[i] = B.E[rr];

		// B.E[i] = B.E[rr] = iStateReached, so instead of B.L[B.E[i]] = i, we can do B.L[iStateReached] = i

		m_pPartStates->m_mpIElemIElemInPartition[iStateReached] = iStateInPartition; // B.L[iStateReached] = i

		m_pPartStates->m_aIElem[m_cStateReached] = iState; // B.E[rr] = q
		m_pPartStates->m_mpIElemIElemInPartition[iState] = m_cStateReached; // B.L[q] = rr

		// rr++;
		++m_cStateReached;
	}
}

void CDfaMinimizer::RemoveUnreachable(int mpITranIStateFrom[], int mpITranIStateTo[]) 
{
	// breadth first search for reachable nodes.
	
	// make_adjacent(T);
	SetAdjacentTrans(mpITranIStateFrom);

	// for each reached state...

	// for (i = 0; i < rr; ++i)
	for (int iStateInPartitionReached = 0; iStateInPartitionReached < m_cStateReached; ++iStateInPartitionReached) 
	{
		// g_aITran has "outgoing" transitons because we did SetAdjacentTrans(mpITranIStateFrom)
		// depending on what was passed for mpITranIStateFrom and mpITranIStateTo,
		// we could be going forward or backward along the transitions
		// for each outgoing transition of this state..
		
		int iStateReached = m_pPartStates->m_aIElem[iStateInPartitionReached]; // B.E[i]
		int iTranOutgoingMic = m_mpIStateITranMic[iStateReached]; // F[B.E[i]]
		int iTranOutgoingMax = m_mpIStateITranMic[iStateReached + 1]; // F[B.E[i] + 1]

		// for (j = F[B.E[i]]; j < F[B.E[i] + 1]; ++j)
		for (int iTranOutgoing = iTranOutgoingMic; iTranOutgoing < iTranOutgoingMax; ++iTranOutgoing) 
		{
			// mark this tranision's destination as reached
			// this will increase g_cStateReached and continue the breadth first search
			
			int iTran = m_aITran[iTranOutgoing]; // A[j]
			int iStateTo = mpITranIStateTo[iTran]; // H[A[j]]

			MarkReached(iStateTo); // reach(H[A[j]]);
		}
	}

	// count the reachable transitions
	// for each transition...

	int cTranReachable = 0; // j = 0;
	for (int iTran = 0; iTran < m_cTran; ++iTran) // for (int t = 0; t < mm; ++t)
	{
		int iStateFrom = mpITranIStateFrom[iTran]; // T[t]
		int iStateInPartitionFrom = m_pPartStates->m_mpIElemIElemInPartition[iStateFrom]; // B.L[T[t]]

		int label = m_mpITranLabel[iTran]; // L[t]
		int iStateTo = mpITranIStateTo[iTran]; // H[t];
		
		bool fIsTranReachable = iStateInPartitionFrom < m_cStateReached; // B.L[T[t]] < rr
		
		// if this transitions from state is reachable...
		
		// if (B.L[T[t]] < rr)
		if (fIsTranReachable) 
		{
			// the next reachable transition is this one, so put it next in the list of transitions
			// this may overwrite an unreachable transition, which is intended
			
			mpITranIStateTo[cTranReachable] = iStateTo; // H[j] = H[t];
			m_mpITranLabel[cTranReachable] = label; // L[j] = L[t];
			mpITranIStateFrom[cTranReachable] = iStateFrom; // T[j] = T[t];

			++cTranReachable; // ++j;
		}
	}

	// remove unreacable transitions
	
	m_cTran = cTranReachable; // mm = j;

	// remove unrechable states

	m_pPartStates->m_mpISetIElemMax[0] = m_cStateReached; // B.P[0] = rr;

	// reset g_cStateReached for the next time we call RemoveUnreachable

	m_cStateReached = 0; // rr = 0;
}