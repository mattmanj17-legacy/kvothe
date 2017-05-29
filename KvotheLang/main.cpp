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
			
			SParser parser;

			parser.SetInput(pFile);
			
			SRegex * pRegex = parser.RegexFileParse();

			fclose(pFile);

			nfa =  nfaBuilder.NfafragFromRegex(pRegex);
		}

		SNfaState * pStateAccept = nfaBuilder.PNfasCreate();

		nfa.Patch(SNfaFragment(pStateAccept, {}));

		nfaBuilder.Bake();

		pDfas = DfaFromNfa(&nfaBuilder, nfa.m_pStateBegin, pStateAccept);
	}

	g_poolDfas.Clear();

	return 0;
}