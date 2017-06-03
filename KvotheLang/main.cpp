#include "pool.h"
#include "regexparse.h"
#include "nfa.h"
#include "dfa.h"

int main()
{	
	const char * pChzFileName = "example.regex";
	FILE * pFile = fopen(pChzFileName, "r");
			
	CRegexParser parser;
	parser.ParseFile(pFile);
	parser.PRegexAstParsed()->PrintDebug();

	printf("\n\n");

	fclose(pFile);

	CNfa nfa;
	nfa.Build(parser.PRegexAstParsed());
	nfa.PrintDebug();

	printf("\n\n");

	CDfa dfa;
	dfa.Build(&nfa);
	dfa.PrintDebug();

	return 0;
}