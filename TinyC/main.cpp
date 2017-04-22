// tiny c compiler and virtual machine

#include <string>
#include <ctype.h>
#include <stdio.h>
#include <stack>

#include "assert.h"
#include "ary.h"

using std::string;
using std::stack;

// parse tree

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

class CParseNode // tag = node
{
	friend class CParseTree;

public:

	void			AddChild(int iNode);

	RULEK			Rulek() const;
	TOKK			Tokk() const;

	CParseNode		NodeChild(int iChild) const;
	int				CChild() const;

	string			Str() const;

private:
					CParseNode();

	RULEK			m_rulek;
	TOKK			m_tokk;
	string			m_str;

	CDynAry<int>	m_daryINodeChild;
};

class CParseTree // tag = parsetree
{
public:
	int	INodeCreate(RULEK rulek);
	int	INodeCreate(TOKK tokk);
	int	INodeCreate(TOKK tokk, string str);

	CParseNode * PNodeFromINode(int iNode);

private:
	CDynAry<CParseNode> m_daryNode;
};

// tokenizer

class CTokenizer // tag = tokenizer
{
public:
		CTokenizer(CParseTree * pParsetree);

	int INodeNext()
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

				return m_pParsetree->INodeCreate(TOKK_INT, strInt);
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

				int iNodeKeyword = INodeKeywordFromString(strToken);
				CParseNode * PNodeKeyword = m_pParsetree->PNodeFromINode(iNodeKeyword);
				if (PNodeKeyword->Tokk() != TOKK_NIL) // keyword
				{
					return iNodeKeyword;
				}
				else if (strToken.length() == 1) // id
				{
					if (isupper(chrCur))
					{
						ASSERT(false);
					}
					else
					{
						return m_pParsetree->INodeCreate(TOKK_ID, strToken);
					}
				}
				else
				{
					ASSERT(false);
				}
			}
			else if (chrCur == '+') { ++m_iChrCur; return m_pParsetree->INodeCreate(TOKK_PLUS); }
			else if (chrCur == '-') { ++m_iChrCur; return m_pParsetree->INodeCreate(TOKK_SUB); }
			else if (chrCur == '<') { ++m_iChrCur; return m_pParsetree->INodeCreate(TOKK_LT); }
			else if (chrCur == '(') { ++m_iChrCur; return m_pParsetree->INodeCreate(TOKK_LPAREN); }
			else if (chrCur == ')') { ++m_iChrCur; return m_pParsetree->INodeCreate(TOKK_RPAREN); }
			else if (chrCur == '=') { ++m_iChrCur; return m_pParsetree->INodeCreate(TOKK_EQUALS); }
			else if (chrCur == '{') { ++m_iChrCur; return m_pParsetree->INodeCreate(TOKK_LBRACE); }
			else if (chrCur == '}') { ++m_iChrCur; return m_pParsetree->INodeCreate(TOKK_RBRACE); }
			else if (chrCur == ';') { ++m_iChrCur; return m_pParsetree->INodeCreate(TOKK_SEMICOLON); }
			else
			{
				ASSERT(false);
			}
		}
		return {TOKK_EOI};
	}

	void SetInput(string str)
	{
		m_strInput = str;
		m_iChrCur = 0;
	}

private:
	int INodeKeywordFromString(string strToken);

	string m_strInput;
	int m_iChrCur;

	CParseTree * m_pParsetree;
};

// parser

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
*/

class CParser
{
public:
	CParseNode RnodeProgram()
	{
		CParseNode rnode(RULEK_Program);
		while (TokkCur() != TOKK_EOI)
			rnode.AddChild(RnodeStatment());
		return rnode;
	}

	CParseNode RnodeStatment()
	{
		CParseNode rnode(RULEK_Statement);
		if (TokkCur() == TOKK_IF)
		{
			rnode.AddChild(TokMatch(TOKK_IF));
			rnode.AddChild(RnodeParenExpr());
			rnode.AddChild(RnodeStatment());
			if (TokkCur() == TOKK_ELSE)
			{
				rnode.AddChild(TokMatch(TOKK_ELSE));
				rnode.AddChild(RnodeStatment());
			}
		}
		else if (TokkCur() == TOKK_WHILE)
		{
			rnode.AddChild(TokMatch(TOKK_WHILE));
			rnode.AddChild(RnodeParenExpr());
			rnode.AddChild(RnodeStatment());
		}
		else if (TokkCur() == TOKK_DO)
		{
			rnode.AddChild(TokMatch(TOKK_DO));
			rnode.AddChild(RnodeStatment());
			rnode.AddChild(TokMatch(TOKK_WHILE));
			rnode.AddChild(RnodeParenExpr());
			rnode.AddChild(TokMatch(TOKK_SEMICOLON));
		}
		else if (TokkCur() == TOKK_LBRACE)
		{
			rnode.AddChild(TokMatch(TOKK_LBRACE));
			while (TokkCur() != TOKK_RBRACE)
			{
				rnode.AddChild(RnodeStatment());
			}
			rnode.AddChild(TokMatch(TOKK_RBRACE));
		}
		else if (TokkCur() == TOKK_ID)
		{
			rnode.AddChild(TokMatch(TOKK_ID));
			rnode.AddChild(TokMatch(TOKK_EQUALS));
			rnode.AddChild(RnodeTestExpr());
			rnode.AddChild(TokMatch(TOKK_SEMICOLON));
		}
		else
		{
			ASSERT(false);
		}
	}

	CParseNode RnodeParenExpr()
	{
		CParseNode rnode(RULEK_ParenExpr);
		rnode.AddChild(TokMatch(TOKK_LPAREN));
		rnode.AddChild(RnodeTestExpr());
		rnode.AddChild(TokMatch(TOKK_RPAREN));
		return rnode;
	}

	CParseNode RnodeTestExpr()
	{
		CParseNode rnode(RULEK_TestExpr);
		rnode.AddChild(RnodeSum());
		if (TokkCur() == TOKK_LT)
		{
			rnode.AddChild(TokMatch(TOKK_LT));
			rnode.AddChild(RnodeSum());
		}
		return rnode;
	}

	CParseNode RnodeSum()
	{
		CParseNode rnode(RULEK_Sum);
		rnode.AddChild(RnodeAtom());
		if (TokkCur() == TOKK_PLUS)
		{
			rnode.AddChild(TokMatch(TOKK_PLUS));
			rnode.AddChild(RnodeSum());
		}
		else if (TokkCur() == TOKK_SUB)
		{
			rnode.AddChild(TokMatch(TOKK_SUB));
			rnode.AddChild(RnodeSum());
		}
	}

	CParseNode RnodeAtom()
	{
		CParseNode rnode(RULEK_Atom);
		if (TokkCur() == TOKK_INT) { rnode.AddChild(TokMatch(TOKK_INT)); }
		else if (TokkCur() == TOKK_ID) { rnode.AddChild(TokMatch(TOKK_ID)); }
		else if (TokkCur() == TOKK_RPAREN) { rnode.AddChild(RnodeParenExpr()); }
		else
		{
			ASSERT(false);
		}
	}

	void SetInput(string strInput)
	{
		m_tokenizer.SetInput(strInput);
		m_tokCur = m_tokenizer.TokNext();
	}
private:

	TOKK TokkCur()
	{
		return m_tokCur.Tokk();
	}

	CParseNode TokMatch(TOKK tokk)
	{
		ASSERT(TokkCur() == tokk);

		CParseNode tokMatch = m_tokCur;
		m_tokCur = m_tokenizer.TokNext();

		return tokMatch;
	}

	CParseNode m_tokCur;
	CTokenizer m_tokenizer;
};

// AST

enum BINOPK
{
	BINOPK_NIL,
	BINOPK_LT,
	BINOPK_PLUS,
	BINOPK_SUB
};

enum EXPRK
{
	EXPRK_Nil,
	EXPRK_Binop,
	EXPRK_Int,
	EXPRK_Id
};

struct SExpression 
{
	EXPRK m_exprk;

	union
	{
		// EXPRK_Binop

		struct
		{
			BINOPK m_binopk;
			int m_iExprLeft;
			int m_iExprRight;
		};

		// EXPRK_Int

		int m_n;

		// EXPRK_Id

		int m_iGlobal;
	};
};

enum STATK
{
	STATK_Nil,
	STATK_Statements,
	STATK_If,
	STATK_While,
	STATK_DoWhile,
	STATK_Assign,
};

struct SStatement 
{
	STATK m_statk;

	union
	{
		// STATK_Statements
		
		CDynAry<SStatement *> m_aryPStat;

		// STATK_If
		
		struct 
		{
			int m_iExprCondition;
			int m_iStatBody;
			bool m_fHasElse;
			SStatement * m_pStatElse;
		};

		// STATK_While / STATK_DoWhile

		struct
		{
			SExpression * m_pExprCondition;
			SStatement * m_pStatBody;
		};

		// STATK_Assign

		struct
		{
			SExpression * m_pId; // must be EXPRK_Id
			SExpression * m_pExpr;
		};
	};
};

class CAstBuilder
{
public:

	void InitStatement(CParseNode pnode, SAstStatement * pAststat)
	{
		if (pnode.Rulek() == RULEK_Program)
		{
			SAstStatements astStatments;

			for (int iChild = 0; iChild < pnode.CChild(); ++iChild)
			{
				astStatments.m_aryPStat.push_back(Statement(pnode.PnodeChild(iChild)));
			}

			return astStatments;
		}
		else
		{
			ASSERT(pnode.Rulek() == RULEK_Statement);
			ASSERT(pnode.CChild() != 0);

			if (pnode.PnodeChild(0).Tokk() == TOKK_IF)
			{
				//TODO
			}
			else if (pnode.PnodeChild(0).Tokk() == TOKK_WHILE)
			{
				ASSERT(pnode.CChild() == 3);
				
				SAstWhile astWhile;
				astWhile.m_pExprCondition = Expression(pnode.PnodeChild(1));
				astWhile.m_pStatBody = Statement(pnode.PnodeChild(2));

				return astWhile;
			}
			else if (pnode.PnodeChild(0).Tokk() == TOKK_DO)
			{
				ASSERT(pnode.CChild() == 3);
				
				SAstDoWhile astDoWhile;
				astDoWhile.m_pStatBody = Statement(pnode.PnodeChild(1));
				astDoWhile.m_pExprCondition = Expression(pnode.PnodeChild(3));
				return astDoWhile;
			}
			else if (pnode.PnodeChild(0).Tokk() == TOKK_LBRACE)
			{
				CParseNode pnodeProg(RULEK_Program);
				for (int iStat = 1; iStat < pnode.CChild() - 1; ++iStat)
				{
					pnodeProg.AddChild(pnode.PnodeChild(iStat));
				}
				return Statement(pnodeProg);
			}
			else if (pnode.PnodeChild(0).Tokk() == TOKK_ID)
			{
				ASSERT(pnode.CChild() == 3);

				SAstAssign astAssign;
				astAssign.m_pId = Id(pnode.PnodeChild(0));
				astAssign.m_pExpr = Expression(pnode.PnodeChild(2));
				return astAssign;
			}
			else
			{
				ASSERT(false);
				return nullptr;
			}
		}
	}

private:

	SAstExpression * Expression(CParseNode pnode)
	{
		if (pnode.Tokk() == TOKK_INT)
		{
			SAstInt astInt;
			astInt.m_n = atoi(&pnode.Str()[0]);
			return astInt;
		}
		else if (pnode.Tokk() == TOKK_ID)
		{
			return Id(pnode);
		}
		else if (pnode.Tokk() == RULEK_Atom)
		{
			ASSERT(pnode.CChild() == 1);
			return Expression(pnode.PnodeChild(0));
		}
		else if (pnode.Rulek() == RULEK_ParenExpr)
		{
			ASSERT(pnode.CChild() == 3);
			return Expression(pnode.PnodeChild(1));
		}
		else if (pnode.Rulek() == RULEK_Sum || pnode.Rulek() == RULEK_TestExpr)
		{
			if (pnode.CChild() == 3)
			{
				SAstBinOp astBinop;
				astBinop.m_aryPExpr[0] = Expression(pnode.PnodeChild(0));
				astBinop.m_binopk = BinopkFromTokk(pnode.PnodeChild(1).Tokk());
				astBinop.m_aryPExpr[1] = Expression(pnode.PnodeChild(2));
				return astBinop;
			}
			else
			{
				ASSERT(pnode.CChild() == 1);
				return Expression(pnode.PnodeChild(0));
			}
		}
		else
		{
			ASSERT(false);
			return nullptr;
		}
	}

	SAstId * Id(CParseNode pnode)
	{
		ASSERT(pnode.Tokk() == TOKK_ID);
		SAstId astId;
		astId.m_iGlobal = pnode.Str()[0] - 'a';
		return astId;
	}

	BINOPK BinopkFromTokk(TOKK tokk)
	{
		switch (tokk)
		{
			case TOKK_PLUS: return BINOPK_PLUS;
			case TOKK_SUB: return BINOPK_SUB;
			case TOKK_LT: return BINOPK_LT;
			default:
			{
				ASSERT(false);
				return BINOPK_NIL;
			}
		}
	}

	CDynAry<SAstExpression> m_daryPExpr;
	CDynAry<SAstStatement> m_daryPStat;
};

// code gen

enum OPCODE : int
{
	OPCODE_IFETCH,
	OPCODE_ISTORE,
	OPCODE_IPUSH,
	OPCODE_IPOP,
	OPCODE_IADD,
	OPCODE_ISUB,
	OPCODE_ILT,
	OPCODE_JZ,
	OPCODE_JNZ,
	OPCODE_JMP,
	OPCODE_HALT
};

enum EXPRK
{
	EXPRK_Nil,
	EXPRK_Binop,
	EXPRK_Int,
	EXPRK_Id
};

class CCodeGen
{
public:
	void GenStatement(SAstStatement * pStat, CDynAry<int> * pDary)
	{
		CDynAry<int> aryRet;
		
		switch (pStat->m_statk)
		{
			case STATK_Statements:
			break;

			case STATK_If:
			break;

			case STATK_While:
			break;

			case STATK_DoWhile:
			break;

			case STATK_Assign:
			{
				SAstAssign * pAstAssign = (SAstAssign *)pStat;
				GenExpression(pAstAssign->m_pExpr, pDary);
				GenExpression(pAstAssign->m_pId, pDary);
				pDary->Append(OPCODE_ISTORE);
			}
			break;

			case STATK_Nil:
			break;
		}
	}

private:
	void GenExpression(SAstExpression * pExpr, CDynAry<int> * pDary)
	{
		switch (pExpr->m_exprk)
		{
			case EXPRK_Binop:
			break;

			case EXPRK_Int:
			break;

			case EXPRK_Id:
			break;

			case EXPRK_Nil:
			break;
		}
	}
};

// Virtual Machine

class CVM
{
public:

	CTinyCVM(CDynAry<int> aryNProg)
	{
		for (int & nGlobal : m_aryNGlobal)
		{
			nGlobal = 0;
		}

		m_aryNProg = aryNProg;
	}

	void Run()
	{
		while ((OPCODE) m_aryNGlobal[m_pc] != OPCODE_HALT)
		{
			Step();
		}

		for (int nGlobal : m_aryNGlobal)
		{
			if (nGlobal != 0)
			{
				printf("%d\n", nGlobal);
			}
		}
	}

	void Step()
	{
		OPCODE opcode = (OPCODE)m_aryNGlobal[m_pc];

		switch (opcode)
		{
			case OPCODE_IFETCH:
			{
				int iGlobal = m_stkInt.top();
				m_stkInt.pop();
				m_stkInt.push(m_aryNGlobal[iGlobal]);
			}
			break;

			case OPCODE_ISTORE:
			{
				int iGlobal = m_stkInt.top();
				m_stkInt.pop();
				int nValue = m_stkInt.top();
				m_stkInt.pop();

				m_aryNGlobal[iGlobal] = nValue;
			}
			break;

			case OPCODE_IPUSH:
			{
				++m_pc;
				int nLit = m_aryNGlobal[m_pc];
				m_stkInt.push(nLit);
			}
			break;

			case OPCODE_IPOP:
			{
				m_stkInt.pop();
			}
			break;

			case OPCODE_IADD:
			{
				int nLeft = m_stkInt.top();
				m_stkInt.pop();
				int nRight = m_stkInt.top();
				m_stkInt.pop();

				m_stkInt.push(nLeft + nRight);
			}
			break;

			case OPCODE_ISUB:
			{
				int nLeft = m_stkInt.top();
				m_stkInt.pop();
				int nRight = m_stkInt.top();
				m_stkInt.pop();

				m_stkInt.push(nLeft - nRight);
			}
			break;

			case OPCODE_ILT:
			{
				int nLeft = m_stkInt.top();
				m_stkInt.pop();
				int nRight = m_stkInt.top();
				m_stkInt.pop();

				m_stkInt.push(nLeft < nRight);
			}
			break;

			case OPCODE_JZ:
			{
				int iJmp = m_stkInt.top();
				m_stkInt.pop();
				int nValue = m_stkInt.top();
				m_stkInt.pop();

				if (nValue == 0)
				{
					m_pc = iJmp - 1;
				}
			}
			break;

			case OPCODE_JNZ:
			{
				int iJmp = m_stkInt.top();
				m_stkInt.pop();
				int nValue = m_stkInt.top();
				m_stkInt.pop();

				if (nValue != 0)
				{
					m_pc = iJmp - 1;
				}
			}
			break;

			case OPCODE_JMP:
			{
				int iJmp = m_stkInt.top();
				m_stkInt.pop();

				m_pc = iJmp - 1;
			}
			break;

			case OPCODE_HALT: return;
			break;

			default:
			break;
		}

		++m_pc;
	}

private:
	int m_aryNGlobal[26];
	int m_pc = 0;
	CDynAry<int> m_aryNProg;
	stack<int> m_stkInt;
};

int main()
{

}