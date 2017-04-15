using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ParseBase
{
    public interface INext<T>
    {
        T Next();
        bool FAtEnd();
    }

    public interface IParseNode<TRULEK, TToken>
    {
        int ITokkEnd();
        void SetITokkEnd(int iTokEnd);

        bool FIsToken();

        TRULEK Rulek();
        TToken Tok();

        void SetRulek(TRULEK rulek);
        void SetTok(TToken tok);

        void AddChild(TToken tok);
        void AddChild(IParseNode<TRULEK, TToken> tok);

        IEnumerable<IParseNode<TRULEK, TToken>> Children();
    }

    public class ParseException : Exception {}

    public abstract class CParserBase<TRuleNode, TRULEK, TToken> : INext<TRuleNode>
    where TRuleNode : IParseNode<TRULEK, TToken>, new()
    where TRULEK : struct // enum used to tag
    {
        public void SetInput(INext<TToken> nextTok)
        {
            m_nextTok = nextTok;
            m_deqTokk.Clear();
            m_stkIBegin.Clear();
            m_iBeginCur = 0;
        }

        // only need to impliment this if being fed to another parser, i.e., we are a tokenizer

        public virtual TRuleNode Next()
        {
            throw new NotImplementedException();
        }

        public bool FAtEnd()
        {
            return m_nextTok.FAtEnd();
        }

        public TRuleNode RnodeMatch(TRULEK rulek)
        {
            TRuleNode rnode = new TRuleNode();
            rnode.SetRulek(rulek);

            bool fFailed = false;
            int iParseStart = m_iBeginCur;

            if (m_mpRulekMemo[rulek].ContainsKey(iParseStart))
            {
                if (m_mpRulekMemo[rulek][iParseStart].ITokkEnd() >= 0)
                {
                    TRuleNode rnodeMemo = m_mpRulekMemo[rulek][iParseStart];

                    if (!FIsSpeculating())
                    {
                        // remove memos we no longer need, and adjust remaining memos

                        m_mpRulekMemo[rulek] =
                            m_mpRulekMemo[rulek]
                            .Keys
                            .Where((key) => key >= rnodeMemo.ITokkEnd())
                            .ToDictionary(
                                (key) => key - rnodeMemo.ITokkEnd(),
                                (key) => m_mpRulekMemo[rulek][key + rnodeMemo.ITokkEnd()]
                            );

                        m_deqTokk.RemoveRange(0, rnodeMemo.ITokkEnd()); // this works because we are not speculating
                    }
                    else
                    {
                        m_iBeginCur = rnodeMemo.ITokkEnd();
                    }

                    return rnodeMemo;
                }
                else
                {
                    throw new ParseException();
                }
            }

            try
            {
                m_mpRulekAct[rulek](rnode);
            }
            catch (ParseException e)
            {
                fFailed = true;
                throw e;
            }
            finally
            {
                if (FIsSpeculating())
                {
                    rnode.SetITokkEnd(fFailed ? -1 : m_iBeginCur);
                    m_mpRulekMemo[rulek][iParseStart] = rnode;
                }
            }

            return rnode;
        }

        protected CParserBase()
        {
            foreach (TRULEK rulek in Enum.GetValues(typeof(TRULEK)))
            {
                m_mpRulekMemo[rulek] = new Dictionary<int, TRuleNode>();
            }

            InitRules();
        }

        protected abstract void InitRules(); // set up m_mpRulekAct

        protected bool FTokEquals(TToken tokA, TToken tokB)
        {
            return EqualityComparer<TToken>.Default.Equals(tokA, tokB);
        }

        protected TToken TokMatch(TToken tokk)
        {
            if (!FTokEquals(TokLookAhead(0), tokk))
                throw new ParseException();

            TToken tokMatch = TokLookAhead(0);

            SkipToken();

            return tokMatch;
        }

        protected void SkipToken()
        {
            if (FIsSpeculating())
            {
                ++m_iBeginCur;
            }
            else
            {
                m_deqTokk.RemoveFront();
            }
        }

        protected TToken TokLookAhead(int i)
        {
            i = Math.Max(i, 0);

            i += m_iBeginCur;

            while (m_deqTokk.Count < i + 1)
            {
                m_deqTokk.AddBack(m_nextTok.Next());
            }

            return m_deqTokk[i];
        }

        protected bool FSpeculate(Action actAlt)
        {
            bool fSuccess = true;
            BeginSpeculate();
            try { actAlt(); }
            catch { fSuccess = false; }
            EndSpeculate();
            return fSuccess;
        }

        protected INext<TToken> m_nextTok;
        protected Dictionary<TRULEK, Action<TRuleNode>> m_mpRulekAct = new Dictionary<TRULEK, Action<TRuleNode>>();

        private void BeginSpeculate()
        {
            m_stkIBegin.Push(m_iBeginCur);
        }

        private void EndSpeculate()
        {
            m_iBeginCur = m_stkIBegin.Pop();
        }

        private bool FIsSpeculating()
        {
            return m_stkIBegin.Count != 0;
        }

        private Dictionary<TRULEK, Dictionary<int, TRuleNode>> m_mpRulekMemo = new Dictionary<TRULEK, Dictionary<int, TRuleNode>>();
        private Deque<TToken> m_deqTokk = new Deque<TToken>();
        private Stack<int> m_stkIBegin = new Stack<int>();
        private int m_iBeginCur = 0;
    }
}
