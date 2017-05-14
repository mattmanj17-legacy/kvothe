#include <string>
#include <ctype.h>
#include <stdio.h>
#include <vector>
#include <assert.h>
#include <memory>

using std::string;
using std::vector;
using std::shared_ptr;

#include "macros.h"

// parser for gfiles grammar, files that specify a grammar
// for grammar of gfiles (written as a gfile), see gfile.gfile

// token kind

enum TOKK
{
	TOKK_ID,	
	TOKK_STRING,
	TOKK_KW_RULE, 	
	TOKK_KW_TOKEN,
	TOKK_KW_ID,	
	TOKK_KW_STRING, 	
	TOKK_ESC_BSLASH,
	TOKK_ESC_TICK,
	TOKK_TICK,
	TOKK_BTICK,
	TOKK_TILDE,
	TOKK_BANG,
	TOKK_AT,
	TOKK_HASH,
	TOKK_DOLLAR,
	TOKK_PERCENT,
	TOKK_CARROT,
	TOKK_AMP,
	TOKK_STAR,
	TOKK_LPAREN,
	TOKK_RPAREN,
	TOKK_PLUS,
	TOKK_HBAR,
	TOKK_EQUAL,
	TOKK_VBAR,
	TOKK_LBRACK,
	TOKK_RBRACK,
	TOKK_LBRACE,
	TOKK_RBRACE,
	TOKK_SEMICOLON,
	TOKK_COLON,
	TOKK_COMMA,
	TOKK_LT,
	TOKK_GT,
	TOKK_DOT,
	TOKK_FSLASH,
	TOKK_QMARK,

	TOKK_Max,
	TOKK_Nil = -1,
	TOKK_Min = 0
};

// rule kind

enum RULEK
{
	RULEK_grammar,
	RULEK_rule,
	RULEK_ruleBody,
	RULEK_postfixOperator,
	RULEK_atom,
	RULEK_builtinToken,
	RULEK_token,
	RULEK_tokenLiteral,
	RULEK_symbol,
	RULEK_escapedSymbolChar,
	RULEK_unescapedSymbolChar,

	RULEK_Max,
	RULEK_Nil = -1,
	RULEK_Min = 0
};

// parse node kind

enum NODEK
{
	NODEK_Rule,
	NODEK_Token,

	NODEK_Max,
	NODEK_Nil = -1,
	NODEK_Min = 0
};

// parse tree

struct SParseTree // tag = parsetree
{
	struct SParseNode // tag = node
	{
		SParseNode(NODEK nodek)
		: m_nodek(nodek)
		{
		}

		virtual void PrintDebug() = 0;
		virtual TOKK Tokk() { return TOKK_Nil; }
		virtual RULEK Rulek() { return RULEK_Nil; }
		
		NODEK m_nodek;
	};

	struct SRuleNode // tag = rnode
	: public SParseNode 
	{
		SRuleNode(RULEK rulek)
		: SParseNode(NODEK_Rule)
		, m_aryPNodeChild() 
		, m_rulek(rulek)
		{
		}

		void AddChild(SParseNode * pNodeChild)
		{
			m_aryPNodeChild.push_back(pNodeChild);
		}

		SParseNode * PNodeChild(unsigned int nChild)
		{
			assert(nChild >= 0);
			assert(nChild < m_aryPNodeChild.size());

			return m_aryPNodeChild[nChild];
		}

		int CChild()
		{
			return m_aryPNodeChild.size();
		}

		void PrintDebug() override
		{
			printf("(");

			for(SParseNode * pNodeChild : m_aryPNodeChild)
			{
				printf(" ");
				pNodeChild->PrintDebug();
			}

			printf(" )");
		}

		RULEK Rulek() override { return m_rulek; }
	
		vector<SParseNode *> m_aryPNodeChild;

		RULEK m_rulek;
	};

	struct STokenNode // tag = tnode
	: public SParseNode
	{
		STokenNode(TOKK tokk, string str = "")
		: SParseNode(NODEK_Token)
		, m_tokk(tokk)
		, m_str(str)
		{
		}
		
		void PrintDebug() override
		{
			if(m_str.size() > 0)
			{
				printf("'%s'", m_str.c_str());
			}
		}

		TOKK Tokk() override { return m_tokk; }

		TOKK m_tokk;
		string m_str;
	};
	
	SRuleNode* PRNodeCreate(RULEK rulek)
	{
		shared_ptr<SParseNode> pNode(new SRuleNode(rulek));
		m_arypNode.push_back(pNode);
		return (SRuleNode*)pNode.get();
	}

	STokenNode* PTNodeCreate(TOKK tokk, string str = "")
	{
		shared_ptr<SParseNode> pNode(new STokenNode(tokk, str));
		m_arypNode.push_back(pNode);
		return (STokenNode*)pNode.get();
	}

	vector<shared_ptr<SParseNode>> m_arypNode;
};

struct STokenizer // tag = tokenizer
{
	STokenizer()
	: m_parsetree()
	, m_pFile(nullptr)
	{
	}

	SParseTree::STokenNode * PTNodeNext()
	{
		static char s_mpTokkChr [] =
		{
			0,		// TOKK_ID,	
			0,		// TOKK_STRING,
			0,		// TOKK_KW_RULE, 	
			0,		// TOKK_KW_TOKEN,
			0,		// TOKK_KW_ID,	
			0,		// TOKK_KW_STRING, 	
			0,		// TOKK_ESC_BSLASH,
			0,		// TOKK_ESC_TICK,
			'\'',	// TOKK_TICK,
			'`',	// TOKK_BTICK,
			'~',	// TOKK_TILDE,
			'!',	// TOKK_BANG,
			'@',	// TOKK_AT,
			'#',	// TOKK_HASH,
			'$',	// TOKK_DOLLAR,
			'%',	// TOKK_PERCENT,
			'^',	// TOKK_CARROT,
			'&',	// TOKK_AMP,
			'*',	// TOKK_STAR,
			'(',	// TOKK_LPAREN,
			')',	// TOKK_RPAREN,
			'+',	// TOKK_PLUS,
			'-',	// TOKK_HBAR,
			'=',	// TOKK_EQUAL,
			'|',	// TOKK_VBAR,
			'[',	// TOKK_LBRACK,
			']',	// TOKK_RBRACK,
			'{',	// TOKK_LBRACE,
			'}',	// TOKK_RBRACE,
			';',	// TOKK_SEMICOLON,
			':',	// TOKK_COLON,
			',',	// TOKK_COMMA,
			'<',	// TOKK_LT,
			'>',	// TOKK_GT,
			'.',	// TOKK_DOT,
			'/',	// TOKK_FSLASH,
			'?',	// TOKK_QMARK
		};
		CASSERT(DIM(s_mpTokkChr) == TOKK_Max);

		static const char * s_mpTokkPChz [] =
		{
			nullptr,	// TOKK_ID,	
			nullptr,	// TOKK_STRING,
			"Rule",		// TOKK_KW_RULE, 	
			"Token",	// TOKK_KW_TOKEN,
			"ID",		// TOKK_KW_ID,	
			"STRING",	// TOKK_KW_STRING, 	
			nullptr,	// TOKK_ESC_BSLASH,
			nullptr,	// TOKK_ESC_TICK,
			nullptr,	// TOKK_TICK,
			nullptr,	// TOKK_BTICK,
			nullptr,	// TOKK_TILDE,
			nullptr,	// TOKK_BANG,
			nullptr,	// TOKK_AT,
			nullptr,	// TOKK_HASH,
			nullptr,	// TOKK_DOLLAR,
			nullptr,	// TOKK_PERCENT,
			nullptr,	// TOKK_CARROT,
			nullptr,	// TOKK_AMP,
			nullptr,	// TOKK_STAR,
			nullptr,	// TOKK_LPAREN,
			nullptr,	// TOKK_RPAREN,
			nullptr,	// TOKK_PLUS,
			nullptr,	// TOKK_HBAR,
			nullptr,	// TOKK_EQUAL,
			nullptr,	// TOKK_VBAR,
			nullptr,	// TOKK_LBRACK,
			nullptr,	// TOKK_RBRACK,
			nullptr,	// TOKK_LBRACE,
			nullptr,	// TOKK_RBRACE,
			nullptr,	// TOKK_SEMICOLON,
			nullptr,	// TOKK_COLON,
			nullptr,	// TOKK_COMMA,
			nullptr,	// TOKK_LT,
			nullptr,	// TOKK_GT,
			nullptr,	// TOKK_DOT,
			nullptr,	// TOKK_FSLASH,
			nullptr,	// TOKK_QMARK
		};
		CASSERT(DIM(s_mpTokkChr) == TOKK_Max);

		while (ChrCur() != EOF)
		{
			// skip white space

			if(isspace(ChrCur()))
			{
				ConsumeChar();
				continue;
			}
			
			// check for escape sequence
			
			if(ChrCur() == '\\')
			{
				string strTok = "";
				strTok += ConsumeChar();
				char chrNext = ConsumeChar();
				strTok += chrNext;
				
				if(chrNext == '\\')
					return m_parsetree.PTNodeCreate(TOKK_ESC_BSLASH, strTok);
				else if(chrNext == '\'')
					return m_parsetree.PTNodeCreate(TOKK_ESC_TICK, strTok);

				assert(false);
			}
			
			// check for single symbol
			
			for(TOKK tokk = TOKK_Min; tokk < TOKK_Max; tokk = (TOKK)((int)tokk + 1))
			{
				char chrTest = s_mpTokkChr[tokk];
				if(chrTest != 0 && ChrCur() == chrTest)
				{
					return m_parsetree.PTNodeCreate(tokk, string(1, ConsumeChar()));
				}
			}

			// check for string
			
			if(ChrCur() == '"')
			{
				ConsumeChar();
				
				string str = "";

				while(ChrCur() != '"' && ChrCur() != '\n' && ChrCur() != '\r' && ChrCur() != '\v' && ChrCur() != '\f' )
				{
					str += ConsumeChar();
				}

				if(ChrCur() != '"')
				{
					assert(false);
				}

				ConsumeChar();

				return m_parsetree.PTNodeCreate(TOKK_STRING, str);
			}
			
			// check for keyword or ID

			bool fHasNum = false;
			if(isalpha(ChrCur()) || ChrCur() == '_')
			{
				string strTok = "";
				
				while(ChrCur() == '_' || isdigit(ChrCur()) || isalpha(ChrCur()))
				{
					if(isdigit(ChrCur()))
					{
						fHasNum = true;
					}

					strTok+=ConsumeChar();
				}

				if(!fHasNum)
				{
					for(TOKK tokk = TOKK_Min; tokk < TOKK_Max; tokk = (TOKK)((int)tokk + 1))
					{
						const char * pChzTest = s_mpTokkPChz[tokk];
						if(pChzTest && strcmp(pChzTest, strTok.c_str()) == 0)
							return m_parsetree.PTNodeCreate(tokk, string(pChzTest)); // keyword
					}
				}

				return m_parsetree.PTNodeCreate(TOKK_ID, strTok); // ID
			}

			// unrecognized char

			assert(false);
		}

		return nullptr;
	}

	char ChrCur()
	{
		return m_chrCur;
	}

	char ConsumeChar()
	{
		char chrPrev = m_chrCur;
		m_chrCur = (char)fgetc(m_pFile);

		return chrPrev;
	}

	void SetInput(FILE * pFile)
	{
		m_pFile = pFile;
		fseek(m_pFile, 0, SEEK_SET);
		ConsumeChar();
	}

	char m_chrCur;
	FILE *			m_pFile;

	SParseTree		m_parsetree;
};

struct SParser
{
	SParseTree::SRuleNode * PRNodeGrammar()
	{
		SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_grammar);
		
		if(TokkCur() == TOKK_KW_RULE)
		{
			pRNode->AddChild(PRNodeRule());
		}
		else if(TokkCur() == TOKK_KW_TOKEN)
		{
			pRNode->AddChild(PRNodeToken());
		}

		while(TokkCur() == TOKK_KW_RULE || TokkCur() == TOKK_KW_TOKEN)
		{
			if(TokkCur() == TOKK_KW_RULE)
			{
				pRNode->AddChild(PRNodeRule());
			}
			else if(TokkCur() == TOKK_KW_TOKEN)
			{
				pRNode->AddChild(PRNodeToken());
			}
		}

		return pRNode;
	}

	SParseTree::SRuleNode * PRNodeRule()
	{
		SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_rule);
		
		assert(TokkCur() == TOKK_KW_RULE);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		assert(TokkCur() == TOKK_ID);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		assert(TokkCur() == TOKK_COLON);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		pRNode->AddChild(PRNodeRuleBody());

		assert(TokkCur() == TOKK_SEMICOLON);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		return pRNode;
	}

	SParseTree::SRuleNode * PRNodeRuleBody()
	{
		SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_ruleBody);

		pRNode->AddChild(PRNodeAtom());

		if(TokkCur() == TOKK_QMARK || TokkCur() == TOKK_PLUS || TokkCur() == TOKK_STAR)
		{
			pRNode->AddChild(PRNodePostfixOperator());
		}

		while(TokkCur() == TOKK_VBAR || TokkCur() == TOKK_ID || TokkCur() == TOKK_LPAREN || TokkCur() == TOKK_KW_ID || TokkCur() == TOKK_KW_STRING)
		{
			if(TokkCur() == TOKK_VBAR)
			{
				pRNode->AddChild(m_pTNodeCur);
				ReadToken();
			}

			pRNode->AddChild(PRNodeAtom());

			if(TokkCur() == TOKK_QMARK || TokkCur() == TOKK_PLUS || TokkCur() == TOKK_STAR)
			{
				pRNode->AddChild(PRNodePostfixOperator());
			}
		}

		return pRNode;
	}

	SParseTree::SRuleNode * PRNodePostfixOperator()
	{
		if(TokkCur() == TOKK_QMARK || TokkCur() == TOKK_PLUS || TokkCur() == TOKK_STAR)
		{
			SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_postfixOperator);

			pRNode->AddChild(m_pTNodeCur);

			ReadToken();

			return pRNode;
		}

		assert(false);
		return nullptr;
	}

	SParseTree::SRuleNode * PRNodeAtom()
	{
		SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_atom);

		if(TokkCur() == TOKK_ID)
		{
			pRNode->AddChild(m_pTNodeCur);
			ReadToken();
		}
		else if(TokkCur() == TOKK_LPAREN)
		{
			pRNode->AddChild(m_pTNodeCur);
			ReadToken();

			pRNode->AddChild(PRNodeRuleBody());

			assert(TokkCur() == TOKK_RPAREN);
			pRNode->AddChild(m_pTNodeCur);
			ReadToken();
		}
		else
		{
			pRNode->AddChild(PRNodeBuiltInToken());
		}

		return pRNode;
	}

	SParseTree::SRuleNode * PRNodeBuiltInToken()
	{
		if(TokkCur() == TOKK_KW_ID || TokkCur() == TOKK_KW_STRING)
		{
			SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_builtinToken);

			pRNode->AddChild(m_pTNodeCur);

			ReadToken();

			return pRNode;
		}

		assert(false);
		return nullptr;
	}

	SParseTree::SRuleNode * PRNodeToken()
	{
		SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_token);
		
		assert(TokkCur() == TOKK_KW_TOKEN);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		assert(TokkCur() == TOKK_ID);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		assert(TokkCur() == TOKK_COLON);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		pRNode->AddChild(PRNodeTokenLiteral());

		assert(TokkCur() == TOKK_SEMICOLON);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		return pRNode;
	}

	SParseTree::SRuleNode * PRNodeTokenLiteral()
	{
		SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_tokenLiteral);
		
		if(TokkCur() == TOKK_STRING)
		{
			pRNode->AddChild(m_pTNodeCur);
			ReadToken();
			return pRNode;
		}
		
		assert(TokkCur() == TOKK_TICK);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		pRNode->AddChild(PRNodeTokenSymbol());

		assert(TokkCur() == TOKK_TICK);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		return pRNode;
	}

	SParseTree::SRuleNode * PRNodeTokenSymbol()
	{
		SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_symbol);
		
		TOKK aTokk[] =
		{
			TOKK_BTICK,
			TOKK_TILDE,
			TOKK_BANG,
			TOKK_AT,
			TOKK_HASH,
			TOKK_DOLLAR,
			TOKK_PERCENT,
			TOKK_CARROT,
			TOKK_AMP,
			TOKK_STAR,
			TOKK_LPAREN,
			TOKK_RPAREN,
			TOKK_PLUS,
			TOKK_HBAR,
			TOKK_EQUAL,
			TOKK_VBAR,
			TOKK_LBRACK,
			TOKK_RBRACK,
			TOKK_LBRACE,
			TOKK_RBRACE,
			TOKK_SEMICOLON,
			TOKK_COLON,
			TOKK_COMMA,
			TOKK_LT,
			TOKK_GT,
			TOKK_DOT,
			TOKK_FSLASH,
			TOKK_QMARK,
		};
		
		if(TokkCur() == TOKK_ESC_BSLASH || TokkCur() == TOKK_ESC_TICK)
		{
			pRNode->AddChild(PRNodeTokenEscapedSymbolChar());
		}
		else
		{
			pRNode->AddChild(PRNodeTokenUnescapedSymbolChar());
		}

		while(true)
		{
			if(TokkCur() == TOKK_ESC_BSLASH || TokkCur() == TOKK_ESC_TICK)
			{
				pRNode->AddChild(PRNodeTokenEscapedSymbolChar());
				continue;
			}

			bool fIsSymbolChar = false;
			for(TOKK tokk : aTokk)
			{
				if(tokk == TokkCur())
				{
					fIsSymbolChar = true;
					break;
				}
			}

			if(fIsSymbolChar)
			{
				pRNode->AddChild(PRNodeTokenUnescapedSymbolChar());
				continue;
			}

			break;
		}

		return pRNode;
	}

	SParseTree::SRuleNode * PRNodeTokenEscapedSymbolChar()
	{
		if(TokkCur() == TOKK_ESC_BSLASH || TokkCur() == TOKK_ESC_TICK)
		{
			SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_escapedSymbolChar);

			pRNode->AddChild(m_pTNodeCur);

			ReadToken();

			return pRNode;
		}

		assert(false);
		return nullptr;
	}

	SParseTree::SRuleNode * PRNodeTokenUnescapedSymbolChar()
	{
		TOKK aTokk[] =
		{
			TOKK_BTICK,
			TOKK_TILDE,
			TOKK_BANG,
			TOKK_AT,
			TOKK_HASH,
			TOKK_DOLLAR,
			TOKK_PERCENT,
			TOKK_CARROT,
			TOKK_AMP,
			TOKK_STAR,
			TOKK_LPAREN,
			TOKK_RPAREN,
			TOKK_PLUS,
			TOKK_HBAR,
			TOKK_EQUAL,
			TOKK_VBAR,
			TOKK_LBRACK,
			TOKK_RBRACK,
			TOKK_LBRACE,
			TOKK_RBRACE,
			TOKK_SEMICOLON,
			TOKK_COLON,
			TOKK_COMMA,
			TOKK_LT,
			TOKK_GT,
			TOKK_DOT,
			TOKK_FSLASH,
			TOKK_QMARK,
		};

		for(TOKK tokk : aTokk)
		{
			if(tokk == TokkCur())
			{
				SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_unescapedSymbolChar);

				pRNode->AddChild(m_pTNodeCur);

				ReadToken();

				return pRNode;
			}
		}

		assert(false);
		return nullptr;
	}

	void ReadToken()
	{
		m_pTNodeCur = m_tokenizer.PTNodeNext();
	}

	TOKK TokkCur()
	{
		return m_pTNodeCur ? m_pTNodeCur->m_tokk : TOKK_Nil;
	}
	
	SParseTree * PParseTree()
	{
		return &m_tokenizer.m_parsetree;
	}

	void SetInput(FILE * pFile)
	{
		m_tokenizer.SetInput(pFile);
		ReadToken();
	}

	SParseTree::STokenNode * m_pTNodeCur;
	STokenizer m_tokenizer;
};

int main()
{
	const char * pChzFileName = "gfile.gfile";

	FILE * pFile = fopen(pChzFileName, "r");

	SParser parser;

	parser.SetInput(pFile);

	SParseTree::SRuleNode * pRNodeGrammar = parser.PRNodeGrammar();

	pRNodeGrammar->PrintDebug();

	return 0;
}
