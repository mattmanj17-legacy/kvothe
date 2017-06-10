#include "dfamin.h"

/* Code below reproduced from "Fast brief practical DFA minimization" by Antti Valmari (2011) */
// https://www.cs.cmu.edu/~cdm/pdf/Valmari12.pdf
#include <iostream>
#include <algorithm> 

// Main program

void CDfaMinimizer::DfaMinMain() 
{
	// Read sizes and reserve most memory

	std::cin >> g_cState >> g_cTran >> g_iStateInitial >> g_cStateFinal;

	g_mpITranIStateFrom = new int[g_cTran]; 
	g_mpITranLabel = new int[g_cTran];
	g_mpITranIStateTo = new int[g_cTran];
	g_partStates.init(g_cState);
	g_aITran = new int[g_cTran]; 
	g_mpIStateITranMic = new int[g_cState + 1];

	// Read transitions

	for (int iTran = 0; iTran < g_cTran; ++iTran) 
	{
		std::cin >> g_mpITranIStateFrom[iTran] >> g_mpITranLabel[iTran] >> g_mpITranIStateTo[iTran];
	}

	// Remove states that cannot be reached from the initial state

	MarkReached(g_iStateInitial); 
	RemoveUnreachable(g_mpITranIStateFrom, g_mpITranIStateTo);

	for (int cStateFinal = 0; cStateFinal < g_cStateFinal; ++cStateFinal) 
	{
		// read in final state
		
		int iStateFinal; 
		std::cin >> iStateFinal; 

		// mark final state as reached (if it is reachable from the start state)

		if (g_partStates.m_mpIElemIElemInPartition[iStateFinal] < g_partStates.m_mpISetIElemMax[0]) 
		{ 
			// note : this also moves this final state to the front of the array
			
			MarkReached(iStateFinal); 
		}
	}
	
	g_cStateFinal = g_cStateReached; 

	// Remove states from which final states cannot be reached 

	RemoveUnreachable(g_mpITranIStateTo, g_mpITranIStateFrom);

	// allocate enough scrate space for marking operation
	// g_cState <= g_cTran + 1, because unreachable states have been removed
	// so g_cTran + 1 is sufficent space

	partition::g_aISetTouched = new int[g_cTran + 1];
	partition::g_mpISetCMarked = new int[g_cTran + 1];
	partition::g_cSetTouched = 0;

	// mark the final states
	// this works because the final states will be at the front of the array
	// after we are done removing irrelevant states

	partition::g_mpISetCMarked[0] = g_cStateFinal;

	// if g_cStateFinal == 0, then this is the empty language and has no states
	
	if (g_cStateFinal > 0) 
	{ 
		partition::g_aISetTouched[partition::g_cSetTouched] = 0; 
		++partition::g_cSetTouched;

		// split into final and non final states

		g_partStates.split(); 
	}

	// Make transition partition

	g_partTrans.init(g_cTran);

	// if g_cTran == 0, then this is the empty language and has no transitions

	if (g_cTran > 0) 
	{
		// we need the transisions with the same labels to be next to each other, so sort them
		// BB (matthewd) hmmm... this could be a counting sort for slightly better run time, because the labels will only be u8's

		std::sort(
			g_partTrans.m_aIElem, 
			g_partTrans.m_aIElem + g_cTran, 
			[this](int iTran0, int iTran1)
			{
				return g_mpITranLabel[iTran0] < g_mpITranLabel[iTran1];
			}
		);

		// partition the transitions into sets containing one label each

		// we are screwing with the sets manually, so reset g_partTrans.m_cSet
		// it will be incremented for each label
		
		g_partTrans.m_cSet = 0;

		// make sure set 0 is init'ed

		partition::g_mpISetCMarked[0] = 0;

		// get the label of the first transition
		
		int iTranFirst = g_partTrans.m_aIElem[0];
		int label = g_mpITranLabel[iTranFirst];

		// for each transition...

		for (int cTran = 0; cTran < g_cTran; ++cTran) 
		{
			int iTran = g_partTrans.m_aIElem[cTran];

			// if this transition has a different label than the current one, start a new set
			
			if (g_mpITranLabel[iTran] != label) 
			{
				// set the label of the new set
				
				label = g_mpITranLabel[iTran]; 

				// create the new set
				// sets its Mic and Max, and inc g_partTrans.m_cSet

				g_partTrans.m_mpISetIElemMax[g_partTrans.m_cSet] = cTran;
				++g_partTrans.m_cSet;
				g_partTrans.m_mpISetIElemMic[g_partTrans.m_cSet] = cTran; 

				// init the marked count for this new set

				partition::g_mpISetCMarked[g_partTrans.m_cSet] = 0;
			}

			// put this transition in the current set

			g_partTrans.m_mpIElemISet[iTran] = g_partTrans.m_cSet; 
			g_partTrans.m_mpIElemIElemInPartition[iTran] = cTran;
		}

		// "close off" the last set. set its max and inc g_partTrans.m_cSet
		
		g_partTrans.m_mpISetIElemMax[g_partTrans.m_cSet] = g_cTran;
		++g_partTrans.m_cSet;
	}

	// Split blocks and cords
	// the meat of the minimization algorithm
	// for explanation/proof, see section 3, "correctness and speed of the splitting stage"

	SetAdjacentTrans(g_mpITranIStateTo);

	// this index has to be outside the inner loop, because we need to retain it's value between iterations
	
	int iSetState = 1; 

	// for each label...

	for(int iSetTrans = 0; iSetTrans < g_partTrans.m_cSet; ++iSetTrans) 
	{
		// for each transition with this label...
		
		int iTranInPartitionMic = g_partTrans.m_mpISetIElemMic[iSetTrans];
		int iTranInPartitionMax = g_partTrans.m_mpISetIElemMax[iSetTrans];
		
		for (int iTranInPartition = iTranInPartitionMic; iTranInPartition < iTranInPartitionMax; ++iTranInPartition) 
		{
			// mark this transition's from state
			
			int iTran = g_partTrans.m_aIElem[iTranInPartition];
			int iState = g_mpITranIStateFrom[iTran];
			g_partStates.mark(iState);
		}

		// split the states
		// this will possibly increase g_partStates.m_cSet,
		// so we will enter the loop below again
		
		g_partStates.split();

		// for every set of states we have not touched yet (possibly created by the previous split)...

		for(; iSetState < g_partStates.m_cSet; ++iSetState) 
		{
			// for every state in this set...
			
			int iStateInPartitionMic = g_partStates.m_mpISetIElemMic[iSetState];
			int iStateInPartitionMax = g_partStates.m_mpISetIElemMax[iSetState];

			for (int iStateInPartition = iStateInPartitionMic; iStateInPartition < iStateInPartitionMax; ++iStateInPartition) 
			{
				// because we did SetAdjacentTrans(g_mpITranIStateTo) above, g_aITran has incoming transitions
				// for every incoming transition...

				int iState = g_partStates.m_aIElem[iStateInPartition];

				int iTranIncomingMic = g_mpIStateITranMic[iState];
				int iTranIncomingMax = g_mpIStateITranMic[iState + 1];

				for (int iTranIncoming = iTranIncomingMic; iTranIncoming < iTranIncomingMax; ++iTranIncoming) 
				{
					// mark this incoming transition
					
					int iTran = g_aITran[iTranIncoming];
					g_partTrans.mark(iTran);
				}
			}

			// split the transitions (possibly making g_partTrans.m_cSet go up)

			g_partTrans.split(); 
		}
	}

	// Count the numbers of transitions in the result
	// for each transition...

	int cTranResult = 0; 
	for (int cTran = 0; cTran < g_cTran; ++cTran) 
	{
		int iStateFrom = g_mpITranIStateFrom[cTran];
		int iSet = g_partStates.m_mpIElemISet[iStateFrom];

		int iStateInPartitionFrom = g_partStates.m_mpIElemIElemInPartition[iStateFrom];
		int iStateInPartitionMic = g_partStates.m_mpISetIElemMic[iSet];

		// if this transision's from state is the first state in a block, inc cTranResult
		// this works because for all states in a block, they all transition to the same block on a given label
		// so we are effectivly combining redundant transitions from on block to another
		
		if (iStateInPartitionFrom == iStateInPartitionMic) 
		{
			++cTranResult;
		}
	}

	// Count the numbers of final states in the result
	// for each set (block of states)

	int cStateFinalResult = 0;
	for (int cSet = 0; cSet < g_partStates.m_cSet; ++cSet) 
	{
		// if the first state in this set is final, then all states in this set are final 
		// so this set if final, so inc cStateFinalResult
		
		bool fIsSetFinal = g_partStates.m_mpISetIElemMic[cSet] < g_cStateFinal;

		if (fIsSetFinal) 
		{ 
			++cStateFinalResult; 
		}
	}

	// Print the result

	// print cState

	std::cout << g_partStates.m_cSet; 
	std::cout << ' '; 

	// print cTran

	std::cout << cTranResult; 
	std::cout << ' '; 

	// print iStateInitial

	std::cout << g_partStates.m_mpIElemISet[g_iStateInitial]; 
	std::cout << ' '; 

	 // print cStateFinal

	std::cout << cStateFinalResult;
	std::cout << '\n';

	// for each transition

	for (int cTran = 0; cTran < g_cTran; ++cTran) 
	{
		int iStateFrom = g_mpITranIStateFrom[cTran];
		int iSetFrom = g_partStates.m_mpIElemISet[iStateFrom];

		int label = g_mpITranLabel[cTran];

		int iStateTo = g_mpITranIStateTo[cTran];
		int iSetTo = g_partStates.m_mpIElemISet[iStateTo];

		int iStateInPartitionFrom = g_partStates.m_mpIElemIElemInPartition[iStateFrom];
		int iStateInPartitionMic = g_partStates.m_mpISetIElemMic[iSetFrom];
		
		// if this transision's from state is the first state in a block, print this transition
		// see note above

		if (iStateInPartitionFrom == iStateInPartitionMic) 
		{
			std::cout << iSetFrom; 
			std::cout << ' '; 
			std::cout << label;
			std::cout << ' '; 
			std::cout << iSetTo;
			std::cout << '\n';
		}
	}

	// for each set of states

	for (int cSet = 0; cSet < g_partStates.m_cSet; ++cSet) 
	{
		// if this set is final, print it
		
		bool fIsSetFinal = g_partStates.m_mpISetIElemMic[cSet] < g_cStateFinal;

		if (fIsSetFinal) 
		{
			std::cout << cSet;
			std::cout << '\n';
		}
	}
}

void CDfaMinimizer::partition::init(int cElem) 
{ 
	// init the data structure as one set containing n elements
	
	// assert n > 0

	m_cSet = 1;

	m_aIElem = new int[cElem];
	m_mpIElemIElemInPartition = new int[cElem]; 
	m_mpIElemISet = new int[cElem];
	m_mpISetIElemMic = new int[cElem]; 
	m_mpISetIElemMax = new int[cElem];

	for (int iElem = 0; iElem < cElem; ++iElem) {
		m_aIElem[iElem] = iElem; 
		m_mpIElemIElemInPartition[iElem] = iElem;
		m_mpIElemISet[iElem] = 0;
	}

	m_mpISetIElemMic[0] = 0; 
	m_mpISetIElemMax[0] = cElem; 
}

void CDfaMinimizer::partition::mark(int iElem) 
{
	// refinement of the partition consists or marking some elements and then calling split

	int iSet = m_mpIElemISet[iElem]; 

	// mark this set as touched if this is the first element in it we are marking
	
	if(g_mpISetCMarked[iSet] == 0)
	{
		g_aISetTouched[g_cSetTouched] = iSet;
		++g_cSetTouched;
	}

	int iElemInPartition = m_mpIElemIElemInPartition[iElem]; 

	// the segment in E the represents S lists its marked elements first.
	// so, the index of first unmarked element is F[s] + M[s]

	int iElemFirstUnmarked = m_mpISetIElemMic[iSet] + g_mpISetCMarked[iSet];

	// mark elem by swapping it with the first unmarked element and incrementing cMarked
	
	m_aIElem[iElemInPartition] = m_aIElem[iElemFirstUnmarked]; 
	m_mpIElemIElemInPartition[m_aIElem[iElemInPartition]] = iElemInPartition;

	m_aIElem[iElemFirstUnmarked] = iElem; 
	m_mpIElemIElemInPartition[iElem] = iElemFirstUnmarked;

	++g_mpISetCMarked[iSet];
}

void CDfaMinimizer::partition::split() 
{
	// for each touched set...

	while (g_cSetTouched) 
	{
		g_cSetTouched--;
		int iSetTouched = g_aISetTouched[g_cSetTouched]; 
		int	iElemFirstUnmarked = m_mpISetIElemMic[iSetTouched] + g_mpISetCMarked[iSetTouched];

		int iElemMax = m_mpISetIElemMax[iSetTouched];

		if (iElemFirstUnmarked == iElemMax) 
		{

			// if all elements are marked, just un mark them and go to the next set
			// because we are only looking at touched sets, we know at least one element is marked

			g_mpISetCMarked[iSetTouched] = 0; 
			continue; 
		}

		// some elements are unmarked
		// because we are only looking at touched sets, we know at least one element is marked
		// so there are both marked and unmarked elements
		// we will split them into two sets

		// create a new set

		// chose the smaller of the marked and unmarked elements in this set, and make it the new set
		// the current set remains, only having the bigger part (either marked or unmarked)
		// always choosing the smaller set makes it possible to avoid a significant complication
		// later i the program

		int cUnmarked = iElemMax - iElemFirstUnmarked;
		
		if (g_mpISetCMarked[iSetTouched] <= cUnmarked) 
		{
			// the new set is the marked elements 
			
			m_mpISetIElemMic[m_cSet] = m_mpISetIElemMic[iSetTouched]; 
			m_mpISetIElemMax[m_cSet] = iElemFirstUnmarked;

			// move the begining of the current set to the first unmarked element

			m_mpISetIElemMic[iSetTouched] = iElemFirstUnmarked;
		}
		else 
		{
			//the new set is the unmarked elements 

			m_mpISetIElemMax[m_cSet] = m_mpISetIElemMax[iSetTouched]; 
			m_mpISetIElemMic[m_cSet] = iElemFirstUnmarked;

			// move the end of the current set to the last marked element

			m_mpISetIElemMax[iSetTouched] = iElemFirstUnmarked;
		}

		for (int i = m_mpISetIElemMic[m_cSet]; i < m_mpISetIElemMax[m_cSet]; ++i) 
		{
			// "move" the elements to the new set

			m_mpIElemISet[m_aIElem[i]] = m_cSet;
		}

		// unmark the elements in the current set

		g_mpISetCMarked[iSetTouched] = 0;

		// the new set begins untouched (all elements unmarked)

		g_mpISetCMarked[m_cSet] = 0;

		++m_cSet;
	}
}

int* CDfaMinimizer::partition::g_mpISetCMarked;
int* CDfaMinimizer::partition::g_aISetTouched;
int CDfaMinimizer::partition::g_cSetTouched;

void CDfaMinimizer::SetAdjacentTrans(int mpITranIState[]) 
{
	// note : this is effectivly a counting sort
	
	// clear g_mpIStateITranOutgoingMic
	
	for (int iState = 0; iState <= g_cState; ++iState) 
	{ 
		g_mpIStateITranMic[iState] = 0; 
	}

	// count the transitions for each state

	for (int iTran = 0; iTran < g_cTran; ++iTran) 
	{ 
		int iState = mpITranIState[iTran];
		++g_mpIStateITranMic[iState]; 
	}

	// "shift" each set of outgoing trans to be after the previous one

	for (int iState = 0; iState < g_cState; ++iState)
	{
		g_mpIStateITranMic[iState + 1] += g_mpIStateITranMic[iState];
	}

	for (int iTran = g_cTran - 1; iTran >= 0; --iTran) 
	{ 
		int iState = mpITranIState[iTran];
		
		// shift down by one (so we can be zero indexed)

		--g_mpIStateITranMic[iState];

		// set g_aITran

		int iTranMic = g_mpIStateITranMic[iState];
		g_aITran[iTranMic] = iTran; 
	}
}

inline void CDfaMinimizer::MarkReached(int iState) 
{ 	
	// reach works like mark, except it does not write into m_mpISetCMarked

	int iStateInPartition = g_partStates.m_mpIElemIElemInPartition[iState];
	if (iStateInPartition >= g_cStateReached) 
	{
		int iStateReached = g_partStates.m_aIElem[g_cStateReached];

		g_partStates.m_aIElem[iStateInPartition] = iStateReached; 
		g_partStates.m_mpIElemIElemInPartition[iStateReached] = iStateInPartition;

		g_partStates.m_aIElem[g_cStateReached] = iState; 
		g_partStates.m_mpIElemIElemInPartition[iState] = g_cStateReached;

		++g_cStateReached;
	}
}

void CDfaMinimizer::RemoveUnreachable(int mpITranIStateFrom[], int mpITranIStateTo[]) 
{
	// breadth first search for reachable nodes.
	
	SetAdjacentTrans(mpITranIStateFrom);

	// for each reached state...

	for (int iStateInPartitionReached = 0; iStateInPartitionReached < g_cStateReached; ++iStateInPartitionReached) 
	{
		// g_aITran has "outgoing" transitons because we did SetAdjacentTrans(mpITranIStateFrom)
		// depending on what was passed for mpITranIStateFrom and mpITranIStateTo,
		// we could be going forward or backward along the transitions
		// for each outgoing transition of this state..
		
		int iStateReached = g_partStates.m_aIElem[iStateInPartitionReached];
		int iTranOutgoingMic = g_mpIStateITranMic[iStateReached];
		int iTranOutgoingMax = g_mpIStateITranMic[iStateReached + 1];

		for (int iTranOutgoing = iTranOutgoingMic; iTranOutgoing < iTranOutgoingMax; ++iTranOutgoing) 
		{
			// mark this tranision's destination as reached
			// this will increase g_cStateReached and continue the breadth first search
			
			int iTran = g_aITran[iTranOutgoing];
			int iStateTo = mpITranIStateTo[iTran];

			MarkReached(iStateTo);
		}
	}

	// count the reachable transitions
	// for each transition...

	int cTranReachable = 0;
	for (int iTran = 0; iTran < g_cTran; ++iTran)
	{
		int iStateFrom = mpITranIStateFrom[iTran];
		int iStateInPartitionFrom = g_partStates.m_mpIElemIElemInPartition[iStateFrom];

		int label = g_mpITranLabel[iTran];
		int iStateTo = mpITranIStateTo[iTran];
		
		bool fIsTranReachable = iStateInPartitionFrom < g_cStateReached;
		
		// if this transitions from state is reachable...
		
		if (fIsTranReachable) 
		{
			// the next reachable transition is this one, so put it next in the list of transitions
			// this may overwrite an unreachable transition, which is intended
			
			mpITranIStateTo[cTranReachable] = iStateTo; 
			g_mpITranLabel[cTranReachable] = label;
			mpITranIStateFrom[cTranReachable] = iStateFrom; 

			++cTranReachable;
		}
	}

	// remove unreacable transitions
	
	g_cTran = cTranReachable; 

	// remove unrechable states

	g_partStates.m_mpISetIElemMax[0] = g_cStateReached; 

	// reset g_cStateReached for the next time we call RemoveUnreachable

	g_cStateReached = 0;
}