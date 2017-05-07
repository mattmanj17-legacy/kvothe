#include <string>
#include <ctype.h>
#include <stdio.h>
#include <stack>
#include <vector>
#include <assert.h>
#include <memory>
#include <stdio.h>
#include <functional>

using std::string;
using std::stack;
using std::vector;
using std::shared_ptr;
using std::function;

// Tiny C
// http://www.iro.umontreal.ca/~felipe/IFT2030-Automne2002/Complements/tinyc.c

// grammar
/*
	program
		: statement +
		;

	statement
		: 'if' paren_expr statement
		| 'if' paren_expr statement 'else' statement
		| 'while' paren_expr statement
		| 'do' statement 'while' paren_expr ';'
		| '{' statement* '}'
		| ID '=' atom ';'
		;

	parenExpr
		: '(' test_expr ')'
		;

	testExpr
		: sum
		| sum '<' sum
		;

	sum
		: atom
		| atom '+' sum
		| atom '-' sum
		;

	atom
		: ID
		| INT
		| parenExpr
		;

	ID : [a-z]
	INT : [0-9]+
*/

// token kind

enum TOKK
{
	TOKK_NIL,
	TOKK_ERR,
	TOKK_EOI,
	TOKK_INT,
	TOKK_ID,
	TOKK_PLUS,
	TOKK_SUB,
	TOKK_LT,
	TOKK_LPAREN,
	TOKK_RPAREN,
	TOKK_EQUALS,
	TOKK_LBRACE,
	TOKK_RBRACE,
	TOKK_DO,
	TOKK_WHILE,
	TOKK_IF,
	TOKK_ELSE,
	TOKK_SEMICOLON,
};

// rule kind

enum RULEK
{
	RULEK_NIL,
	RULEK_ERR,
	RULEK_Atom,
	RULEK_Sum,
	RULEK_TestExpr,
	RULEK_ParenExpr,
	RULEK_Statement,
	RULEK_Program,
	RULEK_Max,
};

// http://stackoverflow.com/a/6256085

#define DIM(arr) ( \
   0 * sizeof(reinterpret_cast<const ::Bad_arg_to_DIM*>(arr)) + \
   0 * sizeof(::Bad_arg_to_DIM::check_type((arr), &(arr))) + \
   sizeof(arr) / sizeof((arr)[0]) )

struct Bad_arg_to_DIM {
   class Is_pointer; // incomplete
   class Is_array {};
   template <typename T>
   static Is_pointer check_type(const T*, const T* const*);
   static Is_array check_type(const void*, const void*);
};

#define CASSERT(predicate) static_assert(predicate, #predicate)

// node in the parse tree

struct SParseNode // tag = node
{
	SParseNode(RULEK rulek)
	: m_rulek(rulek)
	, m_tokk(TOKK_NIL)
	, m_str()
	{
		m_fHasError = m_rulek == RULEK_ERR;
	}

	SParseNode(TOKK tokk)
	: m_rulek(RULEK_NIL)
	, m_tokk(tokk)
	, m_str()
	{
		m_fHasError = m_tokk == TOKK_ERR;
	}

	SParseNode(TOKK tokk, string str)
	: m_rulek(RULEK_NIL)
	, m_tokk(tokk)
	, m_str(str)
	{
		m_fHasError = m_tokk == TOKK_ERR;
	}

	void AddChild(SParseNode * pNodeChild)
	{
		m_aryPNodeChild.push_back(pNodeChild);

		if(pNodeChild->m_fHasError)
			m_fHasError = true;
	}

	SParseNode * PNodeChild(int nChild)
	{
		return m_aryPNodeChild[nChild];
	}

	int CChild()
	{
		return m_aryPNodeChild.size();
	}
	
	vector<SParseNode *>	m_aryPNodeChild;

	RULEK							m_rulek;
	TOKK							m_tokk;
	string							m_str;
	bool							m_fHasError;
};

// parse tree

struct SParseTree // tag = parsetree
{
	SParseNode *	PNodeCreate(RULEK rulek)
	{
		shared_ptr<SParseNode> pNode(new SParseNode(rulek));
		m_aryPNode.push_back(pNode);
		return pNode.get();
	}

	SParseNode *	PNodeCreate(TOKK tokk)
	{
		shared_ptr<SParseNode> pNode(new SParseNode(tokk));
		m_aryPNode.push_back(pNode);
		return pNode.get();
	}

	SParseNode *	PNodeCreate(TOKK tokk, string str)
	{
		shared_ptr<SParseNode> pNode(new SParseNode(tokk, str));
		m_aryPNode.push_back(pNode);
		return pNode.get();
	}

	vector<shared_ptr<SParseNode>> m_aryPNode;
};

// tokenizer

struct STokenizer // tag = tokenizer
{
	STokenizer()
	: m_parsetree()
	, m_iChrCur(0)
	, m_strInput("")
	{
	}

	SParseNode * PNodeTokNext()
	{
		while (m_iChrCur < m_strInput.length())
		{
			char chrCur = m_strInput[m_iChrCur];
			if (isspace(chrCur)) { ++m_iChrCur; } // skip whitespace
			else if (isdigit(chrCur)) // TOKK_INT
			{
				string strInt = "";

				do
				{
					strInt += chrCur;
					++m_iChrCur;
					chrCur = m_strInput[m_iChrCur];
				} while (isdigit(chrCur) && m_iChrCur < m_strInput.length());

				return m_parsetree.PNodeCreate(TOKK_INT, strInt);
			}
			else if (isalpha(chrCur)) // keyword or ID
			{
				string strToken = "";

				do
				{
					strToken += chrCur;
					++m_iChrCur;
					chrCur = m_strInput[m_iChrCur];
				} while (isalpha(chrCur) && m_iChrCur < m_strInput.length());

				TOKK tokkKeyword = TokkKeywordFromString(strToken);
				if (tokkKeyword != TOKK_NIL) // keyword
				{
					return m_parsetree.PNodeCreate(tokkKeyword);
				}
				else if (strToken.length() == 1) // TOKK_Id
				{
					if (isupper(chrCur))
					{
						printf("lex error: expected a lowercase letter, but got uppercase letter '%c'\n", chrCur);
						return  m_parsetree.PNodeCreate(TOKK_ERR);
					}
					else
					{
						return m_parsetree.PNodeCreate(TOKK_ID, strToken);
					}
				}
				else
				{
					printf("lex error: expected keyword or ID, but got '%s'\n", strToken.c_str());
					return  m_parsetree.PNodeCreate(TOKK_ERR);
				}
			}
			else if (chrCur == '+') { ++m_iChrCur; return m_parsetree.PNodeCreate(TOKK_PLUS); }
			else if (chrCur == '-') { ++m_iChrCur; return m_parsetree.PNodeCreate(TOKK_SUB); }
			else if (chrCur == '<') { ++m_iChrCur; return m_parsetree.PNodeCreate(TOKK_LT); }
			else if (chrCur == '(') { ++m_iChrCur; return m_parsetree.PNodeCreate(TOKK_LPAREN); }
			else if (chrCur == ')') { ++m_iChrCur; return m_parsetree.PNodeCreate(TOKK_RPAREN); }
			else if (chrCur == '=') { ++m_iChrCur; return m_parsetree.PNodeCreate(TOKK_EQUALS); }
			else if (chrCur == '{') { ++m_iChrCur; return m_parsetree.PNodeCreate(TOKK_LBRACE); }
			else if (chrCur == '}') { ++m_iChrCur; return m_parsetree.PNodeCreate(TOKK_RBRACE); }
			else if (chrCur == ';') { ++m_iChrCur; return m_parsetree.PNodeCreate(TOKK_SEMICOLON); }
			else
			{
				printf("lex error: unexpected character '%c'", chrCur);
				return  m_parsetree.PNodeCreate(TOKK_ERR);
			}
		}
		return  m_parsetree.PNodeCreate(TOKK_EOI);
	}

	void SetInput(string str)
	{
		m_strInput = str;
		m_iChrCur = 0;
	}

	TOKK TokkKeywordFromString(string str)
	{
		if(str.compare("do") == 0)
		{
			return TOKK_DO;
		}
		else if(str.compare("while") == 0)
		{
			return TOKK_WHILE;
		}
		else if(str.compare("if") == 0)
		{
			return TOKK_IF;
		}
		else if(str.compare("else") == 0)
		{
			return TOKK_ELSE;
		}
		else
		{
			return TOKK_NIL;
		}
	}

	string m_strInput;
	unsigned int m_iChrCur;

	SParseTree m_parsetree;
};

// if we failed to syncronise (we are at the end of input), bail out
#define CheckPNodeSynced(pNode) if (pNode->m_fHasError && TokkCur() == TOKK_EOI) { pNode->m_tokk = TOKK_ERR; pNode->m_rulek = RULEK_ERR; return pNode; } else { pNode->m_fHasError = false; }

// if we are not sycronizing this node, and there was an error, turn this node into an error node so the error can be handled up the call stack
#define CheckPNodeUnsynced(pNode) if (pNode->m_fHasError) { pNode->m_tokk = TOKK_ERR; pNode->m_rulek = RULEK_ERR; return pNode; }

struct SParser
{
	// reresents a given RULEK's first set

	struct SFirstSet // tag = firstset
	{
		const TOKK * m_aTokkFirst;
		const int m_cTokk;

		bool FHasTokk(TOKK tokk) const
		{
			for(int iTokkFirst = 0; iTokkFirst < m_cTokk; ++iTokkFirst)
			{
				if(m_aTokkFirst[iTokkFirst] == tokk)
					return true;
			}

			return false;
		}
	};

	// used to syncronise to a given rule after an error

	void SyncTo(RULEK rulek)
	{
		// first sets

		static const TOKK s_aTokkFirstAtom[] =
		{
			TOKK_ID,
			TOKK_INT,
			TOKK_LPAREN
		};

		static const TOKK s_aTokkFirstSum[] =
		{
			TOKK_ID,
			TOKK_INT,
			TOKK_LPAREN
		};

		static const TOKK s_aTokkFirsTestExpr[] =
		{
			TOKK_ID,
			TOKK_INT,
			TOKK_LPAREN
		};

		static const TOKK s_aTokkFirstParenExpr[] =
		{
			TOKK_LPAREN
		};

		static const TOKK s_aTokkFirstStatement[] =
		{
			TOKK_DO,
			TOKK_WHILE,
			TOKK_IF,
			TOKK_ID,
			TOKK_LBRACE,
		};

		static const TOKK s_aTokkFirstProg[] =
		{
			TOKK_DO,
			TOKK_WHILE,
			TOKK_IF,
			TOKK_ID,
			TOKK_LBRACE,
		};
		
		// map from rulek to first set

		static const SFirstSet s_mpRulekFirstSet[] =
		{
			// RULEK_NIL
			{nullptr, 0},

			//RULEK_ERR
			{nullptr, 0},

			//RULEK_Atom
			{s_aTokkFirstAtom, DIM(s_aTokkFirstAtom)},

			//RULEK_Sum
			{s_aTokkFirstSum, DIM(s_aTokkFirstSum)},

			//RULEK_TestExpr
			{s_aTokkFirsTestExpr, DIM(s_aTokkFirsTestExpr)},

			//RULEK_ParenExpr
			{s_aTokkFirstParenExpr, DIM(s_aTokkFirstParenExpr)},

			//RULEK_Statement
			{s_aTokkFirstStatement, DIM(s_aTokkFirstStatement)},

			//RULEK_Program
			{s_aTokkFirstProg, DIM(s_aTokkFirstProg)},

		};
		CASSERT(DIM(s_mpRulekFirstSet) == RULEK_Max);
		
		const SFirstSet & firstset = s_mpRulekFirstSet[rulek];

		while(!firstset.FHasTokk(TokkCur()) && TokkCur() != TOKK_EOI)
		{
			(void) PNodeTokConsume();
		}
	}

	// used to syncronise to a given token after an error

	void SyncTo(TOKK tokk)
	{
		while(TokkCur() != tokk && TokkCur() != TOKK_EOI)
		{
			(void) PNodeTokConsume();
		}
	}

	typedef SParseNode * (SParser::*FuncPNodeNext)();

	SParseNode * PNodeNext(RULEK rulek, RULEK rulekSync = RULEK_NIL)
	{
		assert(rulek > RULEK_ERR);
		assert(rulekSync != RULEK_ERR);
		
		const FuncPNodeNext s_mpRulekFuncPNodeNext[] =
		{
			//RULEK_NIL,
			nullptr,

			//RULEK_ERR,
			nullptr,

			//RULEK_Atom,
			&SParser::PNodeAtomNext,

			//RULEK_Sum,
			&SParser::PNodeSumNext,

			//RULEK_TestExpr,
			&SParser::PNodeTestExprNext,

			//RULEK_ParenExpr,
			&SParser::PNodeParenExprNext,

			//RULEK_Statement,
			&SParser::PNodeStatNext,

			//RULEK_Program,
			&SParser::PNodeProgNext,
		};
		CASSERT(DIM(s_mpRulekFuncPNodeNext) == RULEK_Max);

		FuncPNodeNext funcPnodeNext = s_mpRulekFuncPNodeNext[rulek];

		SParseNode * pNode = (this->*funcPnodeNext)();
		assert(pNode);

		// if we failed to parse rulek, sync to the sync rule if provided
		
		if(pNode->m_rulek == RULEK_ERR)
		{
			if(rulekSync != RULEK_NIL)
				SyncTo(rulekSync);
		}

		return pNode;
	}

	SParseNode * PNodeNext(RULEK rulek, TOKK tokkSync)
	{
		assert(rulek > RULEK_ERR);
		assert(tokkSync != TOKK_ERR);

		SParseNode * pNode = PNodeNext(rulek);
		assert(pNode);

		// if we failed to parse rulek, sync to the sync token if provided

		if(pNode->m_rulek == RULEK_ERR)
		{
			if(tokkSync != TOKK_NIL)
				SyncTo(tokkSync);
		}

		return pNode;
	}

	SParseNode * PNodeNext(TOKK tokk, RULEK rulekSync = RULEK_NIL)
	{
		assert(tokk > TOKK_ERR);
		assert(rulekSync != RULEK_ERR);

		SParseNode * pNode = PNodeTokConsume();
		assert(pNode);

		// if got an unexpected token, sync to the sync rule if provided

		if(pNode->m_tokk != tokk)
		{
			pNode->m_fHasError = true;
			printf("parse error : unexpected token\n");

			if(rulekSync != RULEK_NIL)
				SyncTo(rulekSync);
		}

		return pNode;
	}

	SParseNode * PNodeNext(TOKK tokk, TOKK tokkSync)
	{
		assert(tokk > TOKK_ERR);
		assert(tokkSync != TOKK_ERR);

		SParseNode * pNode = PNodeTokConsume();
		assert(pNode);

		// if got an unexpected token, sync to the sync token if provided

		if(pNode->m_tokk != tokk)
		{
			pNode->m_fHasError = true;
			printf("parse error : unexpected token\n");
			if(tokkSync != TOKK_NIL)
				SyncTo(tokkSync);
		}

		return pNode;
	}
	
	// parse a program
	
	SParseNode * PNodeProgNext()
	{
		/*
			program
				: statement +
				;
		*/
		
		SParseNode * pNode = PParseTree()->PNodeCreate(RULEK_Program);

		while (TokkCur() != TOKK_EOI)
		{
			pNode->AddChild(PNodeNext(RULEK_Statement, RULEK_Statement)); // statement
			CheckPNodeSynced(pNode);
		}

		return pNode;
	}

	// parse a statement

	SParseNode * PNodeStatNext()
	{
		/*
			statement
				: 'if' paren_expr statement
				| 'if' paren_expr statement 'else' statement
				| 'while' paren_expr statement
				| 'do' statement 'while' paren_expr ';'
				| '{' statement* '}'
				| ID '=' atom ';'
				;
		*/
		
		SParseNode * pNode = PParseTree()->PNodeCreate(RULEK_Statement);

		if (TokkCur() == TOKK_IF)
		{
			// : 'if' paren_expr statement
			// | 'if' paren_expr statement 'else' statement
			
			pNode->AddChild(PNodeNext(TOKK_IF)); // 'if'
			CheckPNodeUnsynced(pNode);

			pNode->AddChild(PNodeNext(RULEK_ParenExpr, RULEK_Statement)); // paren_expr 
			CheckPNodeSynced(pNode);

			pNode->AddChild(PNodeNext(RULEK_Statement)); // statement
			CheckPNodeUnsynced(pNode);

			if (TokkCur() == TOKK_ELSE)
			{
				pNode->AddChild(PNodeNext(TOKK_ELSE, RULEK_Statement)); // 'else'
				CheckPNodeSynced(pNode);

				pNode->AddChild(PNodeNext(RULEK_Statement)); // statement
				CheckPNodeUnsynced(pNode);
			}
		}
		else if (TokkCur() == TOKK_WHILE)
		{
			// 'while' paren_expr statement
			
			pNode->AddChild(PNodeNext(TOKK_WHILE)); // 'while'
			CheckPNodeUnsynced(pNode);

			pNode->AddChild(PNodeNext(RULEK_ParenExpr, RULEK_Statement)); // paren_expr
			CheckPNodeSynced(pNode);

			pNode->AddChild(PNodeNext(RULEK_Statement)); // statement
			CheckPNodeUnsynced(pNode);
		}
		else if (TokkCur() == TOKK_DO)
		{
			// 'do' statement 'while' paren_expr ';'
			
			pNode->AddChild(PNodeNext(TOKK_DO)); // 'do'
			CheckPNodeUnsynced(pNode);

			pNode->AddChild(PNodeNext(RULEK_Statement, TOKK_WHILE)); // statement BB (matthewd) hmmm... syncing on TOKK_WHILE seems dangorous
			CheckPNodeSynced(pNode);

			pNode->AddChild(PNodeNext(TOKK_WHILE, RULEK_ParenExpr)); // 'while'
			CheckPNodeSynced(pNode);

			pNode->AddChild(PNodeNext(RULEK_ParenExpr, TOKK_SEMICOLON)); // paren_expr
			CheckPNodeSynced(pNode);

			pNode->AddChild(PNodeNext(TOKK_SEMICOLON)); // ';'
			CheckPNodeUnsynced(pNode);
		}
		else if (TokkCur() == TOKK_LBRACE)
		{
			// '{' statement* '}'
			
			pNode->AddChild(PNodeNext(TOKK_LBRACE)); // '{'
			CheckPNodeUnsynced(pNode);

			while (TokkCur() != TOKK_RBRACE)
			{
				pNode->AddChild(PNodeNext(RULEK_Statement, RULEK_Statement)); // statement bb hmm...should also probably sync on TOKK_RBRACE
				CheckPNodeSynced(pNode);
			}

			pNode->AddChild(PNodeNext(TOKK_RBRACE)); // '}'
			CheckPNodeUnsynced(pNode);
		}
		else if (TokkCur() == TOKK_ID)
		{
			// ID '=' atom ';'
			
			pNode->AddChild(PNodeNext(TOKK_ID)); // ID

			pNode->AddChild(PNodeNext(TOKK_EQUALS, RULEK_Atom)); // '='
			CheckPNodeSynced(pNode);

			pNode->AddChild(PNodeNext(RULEK_Atom, TOKK_SEMICOLON)); // atom
			CheckPNodeSynced(pNode);

			pNode->AddChild(PNodeNext(TOKK_SEMICOLON)); // ';'
			CheckPNodeUnsynced(pNode);
		}
		else
		{
			pNode->m_fHasError = true;
			pNode->m_tokk = TOKK_ERR;
			pNode->m_rulek = RULEK_ERR;
			printf("parse error : no viable alt");
		}

		return pNode;
	}

	// parse a parenExpr

	SParseNode * PNodeParenExprNext()
	{
		/*
			parenExpr
				: '(' test_expr ')'
				;
		*/
		
		SParseNode * pNode = PParseTree()->PNodeCreate(RULEK_ParenExpr);

		pNode->AddChild(PNodeNext(TOKK_LPAREN, RULEK_TestExpr)); // '('
		CheckPNodeSynced(pNode);

		pNode->AddChild(PNodeNext(RULEK_TestExpr, TOKK_RPAREN)); // test_expr
		CheckPNodeSynced(pNode);

		pNode->AddChild(PNodeNext(TOKK_RPAREN)); // ')'
		CheckPNodeUnsynced(pNode);

		return pNode;
	}

	// parse a testExpr

	SParseNode * PNodeTestExprNext()
	{
		/*
			testExpr
				: sum
				| sum '<' sum
				;
		*/
		
		SParseNode * pNode = PParseTree()->PNodeCreate(RULEK_TestExpr);

		pNode->AddChild(PNodeNext(RULEK_Sum)); // sum
		CheckPNodeUnsynced(pNode);

		if (TokkCur() == TOKK_LT)
		{
			pNode->AddChild(PNodeNext(TOKK_LT)); // '<'
			CheckPNodeUnsynced(pNode);

			pNode->AddChild(PNodeNext(RULEK_Sum)); // sum
			CheckPNodeUnsynced(pNode);
		}
		return pNode;
	}

	// parse a sum

	SParseNode * PNodeSumNext()
	{
		/*
			sum
				: atom
				| atom '+' sum
				| atom '-' sum
				;
		*/
		
		SParseNode * pNode = PParseTree()->PNodeCreate(RULEK_Sum);

		pNode->AddChild(PNodeNext(RULEK_Atom)); // atom
		CheckPNodeUnsynced(pNode);

		if (TokkCur() == TOKK_PLUS)
		{
			pNode->AddChild(PNodeNext(TOKK_PLUS)); // '+'
			CheckPNodeUnsynced(pNode);

			pNode->AddChild(PNodeNext(RULEK_Sum)); //  sum
			CheckPNodeUnsynced(pNode);
		}
		else if (TokkCur() == TOKK_SUB)
		{
			pNode->AddChild(PNodeNext(TOKK_SUB)); // '-'
			CheckPNodeUnsynced(pNode);

			pNode->AddChild(PNodeNext(RULEK_Sum)); // sum
			CheckPNodeUnsynced(pNode);
		}

		return pNode;
	}

	// parse an atom

	SParseNode * PNodeAtomNext()
	{
		/*
			atom
				: ID
				| INT
				| parenExpr
				;
		*/
		
		SParseNode * pNode = PParseTree()->PNodeCreate(RULEK_Atom);

		if (TokkCur() == TOKK_INT) { pNode->AddChild(PNodeNext(TOKK_INT)); }  // ID
		else if (TokkCur() == TOKK_ID) { pNode->AddChild(PNodeNext(TOKK_ID)); } // INT
		else if (TokkCur() == TOKK_LPAREN) 
		{ 
			pNode->AddChild(PNodeNext(RULEK_ParenExpr)); // parenExpr
			CheckPNodeUnsynced(pNode);
		} 
		else
		{
			pNode->m_fHasError = true;
			printf("parse error : no viable alt");
		}

		return pNode;
	}

	void SetInput(string strInput)
	{
		m_tokenizer.SetInput(strInput);
		m_pNodeTokCur = m_tokenizer.PNodeTokNext();
	}

	TOKK TokkCur()
	{
		return m_pNodeTokCur->m_tokk;
	}

	SParseNode * PNodeTokConsume()
	{
		// return current token and get next token

		SParseNode * pNodeTokMatch = m_pNodeTokCur;
		m_pNodeTokCur = m_tokenizer.PNodeTokNext();

		return pNodeTokMatch;
	}

	SParseTree * PParseTree()
	{
		return &m_tokenizer.m_parsetree;
	}

	SParseNode * m_pNodeTokCur;
	STokenizer m_tokenizer;
};

// binary operation kind

enum BINOPK
{
	BINOPK_Nil,
	BINOPK_LT,
	BINOPK_PLUS,
	BINOPK_SUB
};

// expression kind

enum EXPRK
{
	EXPRK_Nil,
	EXPRK_Binop,
	EXPRK_Int,
	EXPRK_Id
};

// an AST expression

struct SExpression 
{
	SExpression(EXPRK exprk)
	: m_exprk(exprk)
	{
	}
	
	EXPRK m_exprk;

	// EXPRK_Binop

	BINOPK m_binopk;
	SExpression * m_pExprLeft = nullptr;
	SExpression * m_pExprRight = nullptr;

	// EXPRK_Int

	unsigned int m_n;

	// EXPRK_Id

	unsigned int m_iGlobal;

	void PrintDebug()
	{
		assert(m_exprk != EXPRK_Nil);
		
		switch (m_exprk)
		{
			case EXPRK_Binop:
				assert(m_binopk != BINOPK_Nil);
				printf("(");
				switch (m_binopk)
				{
					case BINOPK_LT: 
						printf("<");
						break;
					case BINOPK_PLUS: 
						printf("+");
						break;
					case BINOPK_SUB:
						printf("-");
						break;
				}
				printf(" ");
				m_pExprLeft->PrintDebug();
				printf(" ");
				m_pExprRight->PrintDebug();
				printf(")");
				break;
			case EXPRK_Int: 
				printf("%d", m_n);
				break;
			case EXPRK_Id: 
				printf("%c", (char)(m_iGlobal + 'a'));
				break;
			default:
				break;
		}
	}
};

// statement kind

enum STATK
{
	STATK_Nil,
	STATK_Statements,
	STATK_If,
	STATK_While,
	STATK_DoWhile,
	STATK_Assign,
};

// an ast statement

struct SStatement 
{
	SStatement(STATK statk)
	: m_statk(statk)
	{
	}
	
	STATK m_statk;

	// STATK_Statements
		
	vector<SStatement *> m_aryPStat;

	// STATK_If
	
	SStatement * m_pStatElse = nullptr;

	// STATK_If / STATK_While / STATK_DoWhile

	SExpression * m_pExprCondition = nullptr;
	SStatement * m_pStatBody = nullptr;

	// STATK_Assign
		
	SExpression * m_pId = nullptr; // must be EXPRK_Id
	SExpression * m_pExpr = nullptr;

	void PrintDebug()
	{
		assert(m_statk != STATK_Nil);
		
		switch (m_statk)
		{
			case STATK_Statements:
				printf("(");
				for(unsigned int i = 0; i < m_aryPStat.size(); ++i)
				{
					if(i != 0) printf(" ");
					m_aryPStat[i]->PrintDebug();
				}
				printf(")");
				break;
			case STATK_If:
				printf("(ifelse ");
				m_pExprCondition->PrintDebug();
				printf(" ");
				m_pStatBody->PrintDebug();
				if(m_pStatElse)
				{
					printf(" ");
					m_pStatElse->PrintDebug();
				}
				printf(")");
				break;
			case STATK_While:
				printf("(while ");
				m_pExprCondition->PrintDebug();
				printf(" ");
				m_pStatBody->PrintDebug();
				printf(")");
				break;
			case STATK_DoWhile:
				printf("(dowhile ");
				m_pStatBody->PrintDebug();
				printf(" ");
				m_pExprCondition->PrintDebug();
				printf(")");
				break;
			case STATK_Assign:
				printf("(= ");
				m_pId->PrintDebug();
				printf(" ");
				m_pExpr->PrintDebug();
				printf(")");
				break;
		}
	}
};

// the AST

struct SAst
{
	SExpression * PExprCreate(EXPRK exprk)
	{
		shared_ptr<SExpression> pExpr(new SExpression(exprk));
		m_aryPExpr.push_back(pExpr);
		return pExpr.get();
	}

	SExpression * PExprFromPNode(SParseNode * pNode)
	{
		SExpression * pExpr = nullptr;
		
		if (pNode->m_tokk == TOKK_INT)
		{
			// INT
			
			pExpr = PExprCreate(EXPRK_Int);

			assert(pNode->m_str.size() > 0);
			pExpr->m_n = atoi(&pNode->m_str[0]);
		}
		else if (pNode->m_tokk == TOKK_ID)
		{
			// ID
			
			pExpr = PExprCreate(EXPRK_Id);

			assert(pNode->m_str.size() == 1);
			pExpr->m_iGlobal = pNode->m_str[0] - 'a';
		}
		else if (pNode->m_rulek == RULEK_Atom)
		{
			/*
				atom
					: ID
					| INT
					| parenExpr
					;
			*/
			
			assert(pNode->CChild() == 1);
			pExpr = PExprFromPNode(pNode->PNodeChild(0)); // ID | INT | parenExpr
		}
		else if (pNode->m_rulek == RULEK_ParenExpr)
		{
			/*
				parenExpr
					: '(' test_expr ')'
					;
			*/
			
			assert(pNode->CChild() == 3);
			pExpr = PExprFromPNode(pNode->PNodeChild(1)); // test_expr
		}
		else if (pNode->m_rulek == RULEK_Sum || pNode->m_rulek == RULEK_TestExpr)
		{
			/*
				testExpr
					: sum
					| sum '<' sum
					;

				sum
					: atom
					| atom '+' sum
					| atom '-' sum
					;
			*/
			
			if (pNode->CChild() == 3)
			{
				pExpr = PExprCreate(EXPRK_Binop);

				pExpr->m_pExprLeft = PExprFromPNode(pNode->PNodeChild(0)); // sum | atom

				pExpr->m_binopk = BinopkFromTokk(pNode->PNodeChild(1)->m_tokk); // '<' | '+' | '-'
				assert(pExpr->m_binopk != BINOPK_Nil);

				pExpr->m_pExprRight = PExprFromPNode(pNode->PNodeChild(2)); // sum
			}
			else
			{
				assert(pNode->CChild() == 1);

				pExpr = PExprFromPNode(pNode->PNodeChild(0)); // sum | atom
			}
		}
		else
		{
			assert(false);
		}

		return pExpr;
	}

	SStatement * PStatCreate(STATK statk)
	{
		shared_ptr<SStatement> pStat(new SStatement(statk));
		m_aryPStat.push_back(pStat);
		return pStat.get();
	}

	SStatement * PStatFromPNode(SParseNode * pNode)
	{
		SStatement * pStat = nullptr;

		// a program or statement

		if (pNode->m_rulek == RULEK_Program)
		{
			/*
				program
					: statement +
					;
			*/

			pStat = PStatCreate(STATK_Statements);

			for (int iChild = 0; iChild < pNode->CChild(); ++iChild)
			{
				pStat->m_aryPStat.push_back(PStatFromPNode(pNode->PNodeChild(iChild))); // statement
			}
		}
		else if (pNode->m_rulek == RULEK_Statement)
		{
			/*
				statement
					: 'if' paren_expr statement
					| 'if' paren_expr statement 'else' statement
					| 'while' paren_expr statement
					| 'do' statement 'while' paren_expr ';'
					| '{' statement* '}'
					| ID '=' atom ';'
					;
			*/

			assert(pNode->CChild() != 0);

			TOKK tokkChildFirst = pNode->PNodeChild(0)->m_tokk;

			if (tokkChildFirst == TOKK_IF)
			{
				// : 'if' paren_expr statement
				// | 'if' paren_expr statement 'else' statement

				pStat = PStatCreate(STATK_If);

				assert(pNode->CChild() == 3 || pNode->CChild() == 5);
				
				pStat->m_pExprCondition = PExprFromPNode(pNode->PNodeChild(1)); // paren_expr
				pStat->m_pStatBody = PStatFromPNode(pNode->PNodeChild(2)); // statement
				
				if(pNode->CChild() == 5)
				{
					pStat->m_pStatBody = PStatFromPNode(pNode->PNodeChild(4)); // statement
				}
			}
			else if (tokkChildFirst == TOKK_WHILE)
			{
				// | 'while' paren_expr statement
				
				assert(pNode->CChild() == 3);
				
				pStat = PStatCreate(STATK_While);
				pStat->m_pExprCondition = PExprFromPNode(pNode->PNodeChild(1)); // paren_expr
				pStat->m_pStatBody = PStatFromPNode(pNode->PNodeChild(2)); // statement
			}
			else if (tokkChildFirst == TOKK_DO)
			{
				// | 'do' statement 'while' paren_expr ';'
				
				assert(pNode->CChild() == 5);
				
				pStat = PStatCreate(STATK_DoWhile);
				pStat->m_pStatBody = PStatFromPNode(pNode->PNodeChild(1)); // statement
				pStat->m_pExprCondition = PExprFromPNode(pNode->PNodeChild(3)); // paren_expr
			}
			else if (tokkChildFirst == TOKK_LBRACE)
			{
				// | '{' statement* '}'

				pStat = PStatCreate(STATK_Statements);
				
				assert(pNode->CChild() >= 2);
				assert(pNode->PNodeChild(pNode->CChild() - 1)->m_tokk == TOKK_RBRACE);

				for(int iChild = 1; iChild < pNode->CChild() - 1; ++iChild)
				{
					pStat->m_aryPStat.push_back(PStatFromPNode(pNode->PNodeChild(iChild))); // statement
				}
			}
			else if (tokkChildFirst == TOKK_ID)
			{
				// | ID '=' atom ';'
				
				assert(pNode->CChild() == 4);

				pStat = PStatCreate(STATK_Assign);
				pStat->m_pId = PExprFromPNode(pNode->PNodeChild(0)); // ID
				pStat->m_pExpr = PExprFromPNode(pNode->PNodeChild(2)); // atom
			}
			else
			{
				assert(false); // no viable alt
			}
		}
		else
		{
			assert(false); // no viable alt
		}

		return pStat;
	}

	BINOPK BinopkFromTokk(TOKK tokk)
	{
		switch (tokk)
		{
			case TOKK_PLUS: return BINOPK_PLUS;
			case TOKK_SUB:	return BINOPK_SUB;
			case TOKK_LT:	return BINOPK_LT;
			default:		return BINOPK_Nil;
		}
	}

	vector<shared_ptr<SExpression>> m_aryPExpr;
	vector<shared_ptr<SStatement>> m_aryPStat;
};

int main()
{
	string strInput = "{ i=1; while ((i=i+10)<50) ; }";

	SParser parser;

	parser.SetInput(strInput);

	SParseNode * pNode = parser.PNodeProgNext();

	if(pNode->m_fHasError) return 0;

	SAst ast;

	SStatement * pStat = ast.PStatFromPNode(pNode);

	pStat->PrintDebug();

	return 0;
}