#include "pool.h"
#include "regexparse.h"
#include "nfa.h"
#include "dfa.h"
#include "dfamin.h"
#include "regexrand.h"
#include <time.h>
#include <string>
#include <Windows.h>

int main()
{	
	time_t t = time(nullptr);
	srand((u32)t);
	
	for(int i = 0; i < 10000; ++i)
	{
		CRegexRandom regexRand;

		SRegexAstNode regexAst = regexRand.RegexRandom();

		CNfa nfa;
		nfa.Build(&regexAst);

		CDfa dfa;
		dfa.Build(&nfa);

		CDfa dfaMin;
		CDfaMinimizer dfaminimizer;
		dfaminimizer.Minimize(dfa, dfaMin);

		for(int j = 0; j < 10000; ++j)
		{
			string str = regexRand.StrRandFromRegex(regexAst);

			MatchNfa(str, nfa);
			MatchDfa(str, dfa);
			MatchDfa(str, dfaMin);
		}
	}
	
	return 0;
}