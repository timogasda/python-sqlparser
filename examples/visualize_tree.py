import sqlparser
from sqlparser import ENodeType


# Translates a node_type ID into a string
def translate_node_type(node_type):
    for k,v in ENodeType.__dict__.iteritems():
        if v == node_type:
            return k
    return None

# Print node and traverse recursively
def process_node(node, depth=0, name='root'):

    # print attributes like ints and strings
    if not isinstance(node, sqlparser.Node):
        print "%s%s: '%s'" % ("   " * depth, name, str(node))
        return

    # print node attribute
    if node is not None and isinstance(node, sqlparser.Node):
        print "%s%s: %s (%d), text: '%s'" % ("   " * depth, name, translate_node_type(node.node_type), node.node_type, node.get_text())

    # Go through the list (if the current node is a list)
    if node.node_type == ENodeType.list:
        for i, subnode in enumerate(node.list):
            process_node(subnode, depth + 1, 'list_item#%s' % i)
        return

    # Iterate through all attributes
    for k,v in node.__dict__.items():
        process_node(v, depth + 1, k)


parser = sqlparser.Parser(vendor=0)
query = "SELECT a, b, c FROM tbl1 WHERE d IN (1,2,3)"
parser.check_syntax(query)
stmt = parser.get_statement(0)
root = stmt.get_root()

process_node(root)
