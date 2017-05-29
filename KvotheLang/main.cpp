#include "pool.h"
#include "nfa.h"
#include "dfa.h"
#include "regexparse.h"

int main()
{
	SNfaState s;
	nfasEmpty = &s;
	
	SDfaState * pDfas = nullptr;
	
	SNfaBuilder nfaBuilder;

	{
		SNfaFragment nfa;
	
		{
			const char * pChzFileName = "example.regex";

			FILE * pFile = fopen(pChzFileName, "r");
			
			CRegexParser parser;
			
			parser.ParseFile(pFile);

			fclose(pFile);

			nfa =  nfaBuilder.NfafragFromRegex(parser.PRegexAstParsed());
		}

		SNfaState * pStateAccept = nfaBuilder.PNfasCreate();

		nfa.Patch(SNfaFragment(pStateAccept, {}));

		nfaBuilder.Bake();

		pDfas = DfaFromNfa(&nfaBuilder, nfa.m_pStateBegin, pStateAccept);
	}

	g_poolDfas.Clear();

	return 0;
}