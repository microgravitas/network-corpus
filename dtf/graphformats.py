import sys
import datetime
from dtf.graph import Graph as Graph
from xml.sax.saxutils import quoteattr

def read_pajek(filename):
    graph = Graph()

    def read_vertex(line, graph):
        u, *rest = line.split()
        u = int(u)
        graph.add_node(u)

    def read_edge(line, graph):
        u, v, *rest = line.split()
        u, v = int(u), int(v)
        graph.add_edge(u,v)     

    def read_edgelist(line, graph):
        u, *N = line.split()
        u = int(u)
        for v in N:
            graph.add_edge(u,int(v))        

    mode = None
    with open(filename) as f:
        for line in f:
            line = line.strip()
            if len(line) == 0 or line[0] == '%':
                continue   
            if line[0] == '*':
                # Change mode
                modestr = (line.split()[0])[1:].lower()
                if modestr == 'vertices':
                    mode = read_vertex
                elif modestr == 'edges' or modestr == 'arcs':
                    mode = read_edge
                elif modestr == 'edgeslist' or modestr == 'arcslist':
                    mode = read_edgelist
                else:
                    raise Exception('Unknown mode string {} in line {}'.format(modestr, line))
                continue
            mode(line, graph)
    return graph

def read_edgelist(filename):
    graph = Graph()
    for line in open(filename).readlines():
        line = line.strip()
        if len(line) == 0 or line[0] in ['%','#']:
            continue
        s,t,*rest = line.split()
        try:
            s, t = int(s), int(t)
        except:
            pass 
        graph.add_edge(s,t) 

    return graph

def write_edgelist(graph, ostream, **kwargs):
    sep = '\t'
    if 'separator' in kwargs:
        sep = kwargs['separator']
    offset = 0
    if 'base' in kwargs:
        offset = int(kwargs['base'])

    formstr = "{0}"+sep+"{1}"
    if 'weighted' in kwargs and kwargs['weighted'] == True:
        formstr += sep+"1"
    formstr += "\n"
    for u,v in graph.edges():
        ostream.write(formstr.format( u+offset, v+offset ) )


def read_leda(filename):
    graph = Graph()

    numVertices = 10**10
    lines = open(filename)
    
    # Skip preable       
    skip_lines(lines, 4)
    numVertices = int(next(lines))

    # We do not need vertex labels
    skip_lines(lines, numVertices)

    numEdges = int(next(lines))

    for line in lines:
        line = line.strip()
        if len(line) == 0 or line[0] in ['#']:
            continue
        s,t,r,l = line.split(' ')
        graph.add_edge(int(s)-1, int(t)-1) # LEDA is 1-based.
 
    return graph


def write_leda(graph, ostream, **kwargs):
    ostream.write('LEDA.GRAPH\nstring\nstring\n-1\n')

    n = len(graph)
    m = sum(1 for _,_ in graph.edges())

    ostream.write(str(n)+'\n')
    indices = {}
    for i,v in enumerate(graph):
        indices[v] = i
        ostream.write('|{}|\n')

    ostream.write(str(m)+'\n')
    for s,t in graph.edges():
        ostream.write(str(s+1)+' '+str(t+1)+' 0 |{}|\n')

def skip_lines(fileit, num):
    skipped = 0
    while skipped < num:
        line = next(fileit).strip()
        if line != '' and line[0] != '#':
            skipped += 1

def read_gexf(filename):
    from BeautifulSoup import BeautifulSoup as Soup
    soup = Soup(open(filename).read())
    graph = Graph()

    for edge in soup.findAll("edge"):
        source = int(edge['source'])
        target = int(edge['target'])
        graph.add_edge( source, target )
    return graph

def read_graphml(filename):
    from bs4 import BeautifulSoup as BeautifulSoup
    soup = BeautifulSoup(open(filename).read(), "lxml")
    graph = Graph()

    for edge in soup.findAll("edge"):
        source = edge['source']
        target = edge['target']
        graph.add_edge( source, target )
    return graph

def read_gml(filename):
    graph = Graph()
    
    lastkey = "" 
    lastsource = None  
    graphstarted = False
    for l in open(filename):
        l = l.strip()
        if "[" in l:
            l = l.split("[")[0].strip()

        if not graphstarted:
            graphstarted = (l == "graph")
            continue

        if l in ["node","edge"]:
            lastkey = l
            continue
        if l in ["[","]",""]:
            continue
        if "label" in l:
            continue
        t, i = l.split(" ")

        if t == "id":
            assert lastkey == "node", lastkey
        elif t == "source":
            assert lastkey == "edge", lastkey
            lastsource = int(i)
        elif t == "target":
            assert lastkey == "edge", lastkey
            assert lastsource != None
            graph.add_edge(lastsource, int(i)) 
            lastsource = None
    return graph

def write_gml_attr(data, lvl, ostream):
    for attr in data:
        ostream.write('\t'*lvl)
        ostream.write(str(attr))
        ostream.write(' ')

        try:
            it = iter(data[attr])
        except TypeError:
            # Not iterable: plain data
            plain = True
        else:
            # Iterable: not plain EXCEPT if it is a string
            plain = type(data[attr]) is str

        if plain:
            ostream.write('{0}\n'.format(data[attr]))
        else:
            ostream.write('[\n')
            write_gml_attr(data[attr], lvl+1, ostream)
            ostream.write('\t'*lvl)
            ostream.write(']\n')


def write_gml(graph, ostream, nodedata=None, **kwargs):
    ostream.write('graph [\n')
    for v in graph:
        ostream.write('\tnode [\n')
        ostream.write('\t\tid {0}\n'.format(v))
        if nodedata != None and v in nodedata:
            write_gml_attr( nodedata[v], 2, ostream )

        ostream.write('\t]\n')
    for s,t in graph.edges():
        ostream.write('\tedge [\n')
        ostream.write('\t\tsource {0}\n\t\ttarget {1}\n'.format(s,t))
        ostream.write('\t]\n')
    ostream.write(']')

# TODO: refactor this into a stream writer.
def write_gexf(graph, labels, vertexlabels):    
    print( '<?xml version="1.0" encoding="UTF-8"?>')
    print( '<gexf xmlns="http://www.gexf.net/1.2draft" version="1.2">')
    print( '<meta lastmodifieddate="{0:%Y-%m-%d}">'.format(datetime.date.today()))
    print( '\t<creator>LuFGTI RWTH Aachen University</creator>')
    print( '\t<description></description>')
    print( '</meta>')
    print( '<graph defaultedgetype="undirected">')
    
    print( '\t<attributes class="node">')
    for i,l in enumerate(labels):
        print( '\t\t<attribute id="{0}" title="{1}" type="string" />'.format(i, l))
    print( '\t</attributes>')


    print( '\t<nodes>')
    for v in graph:
        lab = vertexlabels[v]
        # Note: quoteattr already adds quotes around the string.
        print( '\t\t<node id="{0}" label="{1}">'.format( v, v ))
        for i,l in enumerate(lab):
            print( '\t\t\t<attvalue for="{0}" value="{1}" />'.format(i,l))
        print( '\t\t</node>')
    print( '\t</nodes>')

    print( '\t<edges>')
    for i,e in enumerate(graph.edges()):
        print( '\t\t<edge id="{0}" source="{1}" target="{2}" />'.format (i, e[0], e[1]))
    print( '\t</edges>')

    print( '</graph>')
    print( '</gexf>')

def load_coloring(filename):
    from graph import Coloring as Coloring
    coloring = Coloring()
    for line in open(filename).readlines():
        line = line.strip()
        if ':' not in line:
            continue
        vertex, color = line.split(':')
        coloring[int(vertex)] = int(color)
    return coloring

def remove_loops(graph):
    for v in graph:
        if graph.adjacent(v,v):
            graph.remove_edge(v,v)

def get_parser(ext):
    if ext == ".gexf":
        return read_gexf
    elif ext == ".graphml":
        return read_graphml
    elif ext == ".gml":
        return read_gml
    elif ext == ".leda":
        return read_leda
    elif ext == ".txt" or ext == ".edges":
        return read_edgelist
    elif ext == ".net":
        return read_pajek    
    else:
        raise Exception('Unknown input file format: {0}'.format(ext))     

def get_writer(ext):
    if ext == ".leda":
        return write_leda
    elif ext == ".gml":
        return write_gml
    elif ext == ".txt" or ext == ".edges":
        return write_edgelist
    else:
        raise Exception('Unknown output file format: {0}'.format(ext))

def load_graph(filename):
    import os
    name, ext = os.path.splitext(filename)

    parser = get_parser(ext)

    return parser(filename)

def write_graph(graph, filename, **kwargs):
    import os
    name, ext = os.path.splitext(filename)

    writer = get_writer(ext)
    writer(graph, open(filename, 'w'), **kwargs)


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Loads a graph from file. If a filename is supplied as \
        arugment for --convert, the graph will be written back into said file. Both the \
        input and the output format are determined by the file ending. Supported inputs types are: \
            .leda .graphml .gml .txt (snap edge list). Supported outputs types are: \
            .leda \
         ")
    parser.add_argument("graph", help="Filename of the input graph.", type=str)
    parser.add_argument("--convert", "-c", help="Output file.", type=str)
    args = parser.parse_args()

    filename = sys.argv[1]
    try:
        g = load_graph(sys.argv[1])
    except Exception as ex:
        print( "Could not load graph.")
        print( ex)
        sys.exit()
    print( "Graph {0} has {1} vertices and {2} edge".format(filename, len(g), len(list(g.edges()))))
    print( "Graph hash: {0}".format( graph.graph_hash(g) ))

    if args.convert:
        print( "Converting to {0}".format(args.convert))
        try:
            write_graph(g, args.convert)
        except Exception as ex:
            print( "\nCould not convert.")
            print( ex)
            sys.exit()
