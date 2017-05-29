#include "pool.h"
#include "regexparse.h"
#include "nfa.h"
#include "dfa.h"

int main()
{
	CNfaState s;
	
	SDfaState * pDfas = nullptr;

	{
		CNfa nfaBuilder;
	
		{
			const char * pChzFileName = "example.regex";

			FILE * pFile = fopen(pChzFileName, "r");
			
			CRegexParser parser;
			
			parser.ParseFile(pFile);

			fclose(pFile);

			nfaBuilder.Build(parser.PRegexAstParsed());
		}

		pDfas = DfaFromNfa(&nfaBuilder);
	}

	g_poolDfas.Clear();

	return 0;
}