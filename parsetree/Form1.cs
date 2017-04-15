using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using Satsuma;
using Satsuma.Drawing;
using NestedNameList;

namespace parsetree
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            this.Paint += Form1_Paint;

            string input = "[a,b] = [c,d]";
            CParser parser = new CParser();
            parser.SetInput(input);

            CRuleNode rnode = parser.RnodeAssign();
            graphFromPtree(g, rnode);
        }

        Node graphFromPtree(CustomGraph g, CParseTree ptree)
        {
            Node n = g.AddNode();

            nodeCaptions[n] = "null!";

            if (ptree is CRuleNode)
            {
                nodeCaptions[n] = (ptree as CRuleNode).m_rulek.ToString();
            }
            else if(ptree is CTokenNode)
            {
                nodeCaptions[n] = (ptree as CTokenNode).m_tokk.ToString();
            }

            foreach(CParseTree ptreeChild in ptree.children)
            {
                g.AddArc(n, graphFromPtree(g, ptreeChild), Directedness.Directed);
            }

            return n;
        }

        private void Form1_Paint(object sender, PaintEventArgs e)
        {
            Random r = new Random();
            // compute a nice layout of the graph
            var layout = new ForceDirectedLayout(g);
            layout.Initialize((node) => new PointD(0.5 + r.NextDouble() * 0.01 , 0.5 + r.NextDouble() * 0.01));
            layout.Run();
            // draw the graph using the computed layout
            var nodeShape = new NodeShape(NodeShapeKind.Rectangle, new PointF(40, 40));
            var nodeStyle = new NodeStyle { Brush = Brushes.Yellow, Shape = nodeShape };
            var drawer = new GraphDrawer()
            {
            	Graph = g,
            	NodePosition = (node => (PointF)layout.NodePositions[node]),
            	NodeCaption = (node => nodeCaptions[node]),
            	NodeStyle = (node => nodeStyle)
            };
            drawer.Draw(e.Graphics, e.ClipRectangle);
        }

        private CustomGraph g = new CustomGraph();
        private Dictionary<Node, string> nodeCaptions = new Dictionary<Node, string>();
    }
}
