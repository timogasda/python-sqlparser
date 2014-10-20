import sqlparser
from sqlparser import ENodeType, EExpressionType, ETableSource
import nodevisitor
from nodevisitor import NodeVisitor

class TableVisitor(NodeVisitor):

    def __init__(self, root_node):
        NodeVisitor.__init__(self, root_node)

        # Keeps track of the subquery select statements
        # each time we find a subquery, we push the root node onto here
        # when we leave a subquery, we pop it
        self.current_select = []

        # Keeps a list of all tables that were used in the *root* select statement
        self.used_tables = []

    def get_fromTable_names(self, node):
        # This is a helper function that will acccept a fromTable node
        # and return a tuple of (alias, real_name)

        name = None
        alias = None

        # Check if the node has an alias clause
        if node.aliasClause and node.aliasClause.aliasName:
            # if it is a simple alias (objectname)
            if node.aliasClause.aliasName.node_type == ENodeType.objectname:
                # objectnames can be either "partToken" or "objectToken.partToken"
                # but an alias should only have one token
                alias = node.aliasClause.aliasName.partToken.upper().strip('"')

        # Check if the tableName is available
        if node.tableName:
            if node.tableName.node_type == ENodeType.objectname:
                name = node.tableName.partToken.upper().strip('"')

        return name, alias

    # Use nodevisitor.when to provide a callback for certain node types
    @nodevisitor.when(ENodeType.selectStatement)
    def visit_selectStatement(self, node, d, name):
        # found a new select statement.
        # push it into the list
        self.current_select.append(node)

        # continue traversing but notify me when you leave this node's subtree
        return nodevisitor.ContinueAndNotify

    # this function will be called for ContinueAndNotify
    def leave_node(self, node, d, name):
        if node.node_type == ENodeType.selectStatement:
            # remove the current select node from the list
            self.current_select.pop()

    # This will be called for every from table name
    @nodevisitor.when(ENodeType.fromTable)
    def visit_fromTable(self, node, d, name):
        if len(self.current_select) == 0 or self.current_select[-1] != self.root:
            # only save tables for the root statement
            return nodevisitor.Continue

        table = {}

        # Retrieve table name + alias
        name, alias = self.get_fromTable_names(node)
        if name is not None:
            table['position'] = node.get_position()
            table['name'] = name
            table['alias'] = alias

            self.used_tables.append(table)

        # continue traversing this tree
        return nodevisitor.Continue

if __name__ == '__main__':
    query = """SELECT a, b, c FROM table1 "FOO" inner join "TABLE2" ON a = b"""

    # Create a new parser
    parser = sqlparser.Parser(vendor=0)
    # Check syntax
    assert parser.check_syntax(query) == 0
    # Get first statement
    stmt = parser.get_statement(0)
    # Get root node
    root = stmt.get_root()

    # Create new visitor instance from root
    visitor = TableVisitor(root)
    # Traverse the syntax tree
    visitor.traverse()

    for table in visitor.used_tables:
        if table['alias']:
            print "%s => %s @ %s" % (table['alias'], table['name'], table['position'])
        else:
            print "%s @ %s" % (table['name'], table['position'])
