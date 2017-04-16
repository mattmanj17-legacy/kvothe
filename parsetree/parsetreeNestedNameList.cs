using System;
using System.Collections.Generic;
using System.Linq;

/*
	stat : list EOF | assign EOF
	assign : list '=' list
	list: '[' elements ']'
	elements: element (',' element)*
	element: NAME '=' NAME | NAME | list
*/

// TODO!!!!!!!!!!!!! do tokenizerbase and parser base. trying to have them be the same thing is messy

namespace NestedNameList
{

    public enum TOKK
    {
        EOS,
        NAME,
        RBRACK,
        LBRACK,
        COMMA,
        EQUALS,
    }

    public enum RULEK
    {
        Stat,
        Assign,
        List,
        Elements,
        Element
    }

    public class CTokenizer
    {
        public void SetInput(string strInput)
        {
            m_strInput = strInput;
            m_iChrCur = 0;
        }

        public TOKK TokkNext()
        {
            while (m_iChrCur < m_strInput.Length)
            {
                char chrCur = m_strInput[m_iChrCur];
                if (char.IsWhiteSpace(chrCur)) { ++m_iChrCur; } // ignore white space
                else if (chrCur == ',') { ++m_iChrCur; return TOKK.COMMA; }
                else if (chrCur == '[') { ++m_iChrCur; return TOKK.LBRACK; }
                else if (chrCur == ']') { ++m_iChrCur; return TOKK.RBRACK; }
                else if (chrCur == '=') { ++m_iChrCur; return TOKK.EQUALS; }
                else if (char.IsLetter(chrCur))
                {

                    while (char.IsLetter(chrCur) && m_iChrCur < m_strInput.Length)
                    {
                        ++m_iChrCur;
                        chrCur = m_strInput[m_iChrCur];
                    }

                    return TOKK.NAME;
                }
                else { throw new Exception(); }
            }
            return TOKK.EOS;
        }

        private string m_strInput;
        private int m_iChrCur;
    }

    public class CParser
    {
        CTokenNode TnodeMatch(TOKK tokk)
        {
            CTokenNode tnode = new CTokenNode(tokk);

            if (TokkLookAhead(0) != tokk) { throw new Exception(); }

            if (FIsSpeculating())
            {
                ++m_iBeginCur;
            }
            else
            {
                m_deqTokk.RemoveFront();
            }

            return tnode;
        }

        TOKK TokkLookAhead(int i)
        {
            i = Math.Max(i, 0);

            i += m_iBeginCur;

            while (m_deqTokk.Count < i + 1)
            {
                m_deqTokk.AddBack(m_tokenizer.TokkNext());
            }

            return m_deqTokk[i];
        }

        void BeginSpeculate()
        {
            m_stkIBegin.Push(m_iBeginCur);
        }

        void EndSpeculate()
        {
            m_iBeginCur = m_stkIBegin.Pop();
        }

        public CRuleNode RnodeStat()
        {
            CRuleNode rnode = new CRuleNode(RULEK.Stat);

            if (FSpeculateStatIsAssign())
            {
                rnode.AddChild(RnodeAssign());
                rnode.AddChild(TnodeMatch(TOKK.EOS));
            }
            else if (TokkLookAhead(0) == TOKK.LBRACK)
            {
                rnode.AddChild(RnodeList());
                rnode.AddChild(TnodeMatch(TOKK.EOS));
            }
            else
            {
                throw new Exception();
            }

            return rnode;
        }

        bool FSpeculateStatIsAssign()
        {
            bool fSuccess = true;
            BeginSpeculate();
            try { RnodeAssign(); TnodeMatch(TOKK.EOS); }
            catch { fSuccess = false; }
            EndSpeculate();
            return fSuccess;
        }

        bool FIsSpeculating()
        {
            return m_stkIBegin.Count != 0;
        }

        public CRuleNode RnodeAssign()
        {
            CRuleNode rnode = new CRuleNode(RULEK.Assign);

            bool fFailed = false;
            int iParseStart = m_iBeginCur;

            if (m_dictMemoAssign.ContainsKey(iParseStart))
            {
                if (m_dictMemoAssign[iParseStart].iTokkEnd >= 0)
                {
                    CRuleNode rnodeMemo = m_dictMemoAssign[iParseStart];

                    if(!FIsSpeculating())
                    {
                        // remove memos we no longer need, and adjust remaining memos

                        m_dictMemoAssign =
                            m_dictMemoAssign
                            .Keys
                            .Where((key) => key >= rnodeMemo.iTokkEnd)
                            .ToDictionary(
                                (key) => key - rnodeMemo.iTokkEnd,
                                (key) => m_dictMemoAssign[key + rnodeMemo.iTokkEnd]
                            );
                       
                        m_deqTokk.RemoveRange(0, rnodeMemo.iTokkEnd); // this works because we are not speculating
                    }
                    else
                    {
                        m_iBeginCur = rnodeMemo.iTokkEnd;
                    }

                    return rnodeMemo;
                }
                else
                {
                    throw new Exception();
                }
            }

            try
            {
                rnode.AddChild(RnodeList());
                rnode.AddChild(TnodeMatch(TOKK.EQUALS));
                rnode.AddChild(RnodeList());
            }
            catch (Exception e)
            {
                fFailed = true;
                throw e;
            }
            finally
            {
                if (FIsSpeculating())
                {
                    rnode.iTokkEnd = fFailed ? -1 : m_iBeginCur;
                    m_dictMemoAssign[iParseStart] = rnode;
                }
            }

            return rnode;
        }

        public CRuleNode RnodeList()
        {
            CRuleNode rnode = new CRuleNode(RULEK.List);
            rnode.AddChild(TnodeMatch(TOKK.LBRACK));
            rnode.AddChild(RnodeElements());
            rnode.AddChild(TnodeMatch(TOKK.RBRACK));
            return rnode;
        }

        public CRuleNode RnodeElements()
        {
            CRuleNode rnode = new CRuleNode(RULEK.Elements);

            rnode.AddChild(RnodeElement());

            while (TokkLookAhead(0) == TOKK.COMMA)
            {
                rnode.AddChild(TnodeMatch(TOKK.COMMA));
                rnode.AddChild(RnodeElement());
            }

            return rnode;
        }

        public CRuleNode RnodeElement()
        {
            CRuleNode rnode = new CRuleNode(RULEK.Element);

            if (TokkLookAhead(0) == TOKK.NAME)
            {
                rnode.AddChild(TnodeMatch(TOKK.NAME));

                if (TokkLookAhead(0) == TOKK.EQUALS)
                {
                    rnode.AddChild(TnodeMatch(TOKK.EQUALS));
                    rnode.AddChild(TnodeMatch(TOKK.NAME));
                }
            }
            else if (TokkLookAhead(0) == TOKK.LBRACK)
            {
                rnode.AddChild(RnodeList());
            }
            else
            {
                throw new Exception();
            }

            return rnode;
        }

        public void SetInput(string strInput)
        {
            m_tokenizer.SetInput(strInput);
            m_deqTokk.Clear();
            m_stkIBegin.Clear();
            m_iBeginCur = 0;
        }

        Dictionary<int, CRuleNode> m_dictMemoAssign = new Dictionary<int, CRuleNode>();

        CTokenizer m_tokenizer = new CTokenizer();
        Deque<TOKK> m_deqTokk = new Deque<TOKK>();
        Stack<int> m_stkIBegin = new Stack<int>();
        int m_iBeginCur = 0;
    }
}

