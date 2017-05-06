#include <string>
#include <ctype.h>
#include <stdio.h>
#include <stack>
#include <vector>
#include <assert.h>
#include <memory>

using std::string;
using std::stack;
using std::vector;
using std::shared_ptr;

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
	RULEK_Atom,
	RULEK_Sum,
	RULEK_TestExpr,
	RULEK_ParenExpr,
	RULEK_Statement,
	RULEK_Program,
};

// node in the parse tree

struct SParseNode // tag = node
{
	SParseNode(RULEK rulek)
	: m_rulek(rulek)
	, m_tokk(TOKK_NIL)
	, m_str()
	{
	}

	SParseNode(TOKK tokk)
	: m_rulek(RULEK_NIL)
	, m_tokk(tokk)
	, m_str()
	{
	}

	SParseNode(TOKK tokk, string str)
	: m_rulek(RULEK_NIL)
	, m_tokk(tokk)
	, m_str(str)
	{
	}

	void AddChild(SParseNode * pNodeChild)
	{
		m_aryPNodeChild.push_back(pNodeChild);
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
						assert(false); // no viable alt
					}
					else
					{
						return m_parsetree.PNodeCreate(TOKK_ID, strToken);
					}
				}
				else
				{
					assert(false); // no viable alt
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
				assert(false); // no viable alt
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

struct SParser
{
	SParseTree * PParseTree()
	{
		return &m_tokenizer.m_parsetree;
	}
	
	SParseNode * PNodeProgNext()
	{
		/*
			program
				: statement +
				;
		*/
		
		SParseNode * pNode = PParseTree()->PNodeCreate(RULEK_Program);
		while (TokkCur() != TOKK_EOI)
			pNode->AddChild(PNodeStatNext()); // statement
		return pNode;
	}

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
			
			pNode->AddChild(PNodeTokConsume(TOKK_IF)); // 'if'
			pNode->AddChild(PNodeParenExprNext()); // paren_expr
			pNode->AddChild(PNodeStatNext()); // statement
			if (TokkCur() == TOKK_ELSE)
			{
				pNode->AddChild(PNodeTokConsume(TOKK_ELSE)); // 'else'
				pNode->AddChild(PNodeStatNext()); // statement
			}
		}
		else if (TokkCur() == TOKK_WHILE)
		{
			// 'while' paren_expr statement
			
			pNode->AddChild(PNodeTokConsume(TOKK_WHILE)); // 'while'
			pNode->AddChild(PNodeParenExprNext()); // paren_expr
			pNode->AddChild(PNodeStatNext()); // statement
		}
		else if (TokkCur() == TOKK_DO)
		{
			// 'do' statement 'while' paren_expr ';'
			
			pNode->AddChild(PNodeTokConsume(TOKK_DO)); // 'do'
			pNode->AddChild(PNodeStatNext()); // statement
			pNode->AddChild(PNodeTokConsume(TOKK_WHILE)); // 'while'
			pNode->AddChild(PNodeParenExprNext()); // paren_expr
			pNode->AddChild(PNodeTokConsume(TOKK_SEMICOLON)); // ';'
		}
		else if (TokkCur() == TOKK_LBRACE)
		{
			// '{' statement* '}'
			
			pNode->AddChild(PNodeTokConsume(TOKK_LBRACE)); // '{'
			while (TokkCur() != TOKK_RBRACE)
			{
				pNode->AddChild(PNodeStatNext()); // statement
			}
			pNode->AddChild(PNodeTokConsume(TOKK_RBRACE)); // '}'
		}
		else if (TokkCur() == TOKK_ID)
		{
			// ID '=' atom ';'
			
			pNode->AddChild(PNodeTokConsume(TOKK_ID)); // ID
			pNode->AddChild(PNodeTokConsume(TOKK_EQUALS)); // '='
			pNode->AddChild(PNodeAtomNext()); // atom
			pNode->AddChild(PNodeTokConsume(TOKK_SEMICOLON)); // ';'
		}
		else
		{
			assert(false); // no viable alt
		}

		return pNode;
	}

	SParseNode * PNodeParenExprNext()
	{
		/*
			parenExpr
				: '(' test_expr ')'
				;
		*/
		
		SParseNode * pNode = PParseTree()->PNodeCreate(RULEK_ParenExpr);
		pNode->AddChild(PNodeTokConsume(TOKK_LPAREN)); // '('
		pNode->AddChild(PNodeTestExprNext()); // test_expr
		pNode->AddChild(PNodeTokConsume(TOKK_RPAREN)); // ')'
		return pNode;
	}

	SParseNode * PNodeTestExprNext()
	{
		/*
			testExpr
				: sum
				| sum '<' sum
				;
		*/
		
		SParseNode * pNode = PParseTree()->PNodeCreate(RULEK_TestExpr);

		pNode->AddChild(PNodeSumNext()); // sum
		if (TokkCur() == TOKK_LT)
		{
			pNode->AddChild(PNodeTokConsume(TOKK_LT)); // '<'
			pNode->AddChild(PNodeSumNext()); // sum
		}
		return pNode;
	}

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

		pNode->AddChild(PNodeAtomNext()); // atom
		if (TokkCur() == TOKK_PLUS)
		{
			pNode->AddChild(PNodeTokConsume(TOKK_PLUS)); // '+'
			pNode->AddChild(PNodeSumNext()); //  sum
		}
		else if (TokkCur() == TOKK_SUB)
		{
			pNode->AddChild(PNodeTokConsume(TOKK_SUB)); // '-'
			pNode->AddChild(PNodeSumNext()); // sum
		}

		return pNode;
	}

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

		if (TokkCur() == TOKK_INT) { pNode->AddChild(PNodeTokConsume(TOKK_INT)); }  // ID
		else if (TokkCur() == TOKK_ID) { pNode->AddChild(PNodeTokConsume(TOKK_ID)); } // INT
		else if (TokkCur() == TOKK_RPAREN) { pNode->AddChild(PNodeParenExprNext()); } // parenExpr
		else
		{
			assert(false); // no viable alt
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

	SParseNode * PNodeTokConsume(TOKK tokk)
	{
		// current token much match tokk
		// return current token and get next token
		
		assert(TokkCur() == tokk);

		SParseNode * pNodeTokMatch = m_pNodeTokCur;
		m_pNodeTokCur = m_tokenizer.PNodeTokNext();

		return pNodeTokMatch;
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
	SExpression * m_pExprLeft;
	SExpression * m_pExprRight;

	// EXPRK_Int

	int m_n;

	// EXPRK_Id

	int m_iGlobal;
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
	
	SStatement * m_pStatElse;

	// STATK_If / STATK_While / STATK_DoWhile

	SExpression * m_pExprCondition;
	SStatement * m_pStatBody;

	// STATK_Assign
		
	SExpression * m_pId; // must be EXPRK_Id
	SExpression * m_pExpr;
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
	string strInput = "{ i=7; if (i<5) x=1; if (i<10) y=2; }";

	SParser parser;

	parser.SetInput(strInput);

	SParseNode * pNode = parser.PNodeProgNext();

	SAst ast;

	SStatement * pStat = ast.PStatFromPNode(pNode);

	return 0;
}