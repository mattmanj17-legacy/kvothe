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
		printf("%s", regexAst.StrPretty().c_str());
		printf("\n\n");
		string str = regexRand.StrRandFromRegex(regexAst);
		printf("%s",str.c_str());
		system("cls");
	}
	
	return 0;
}