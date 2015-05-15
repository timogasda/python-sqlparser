import inspect

import sqlparser
from sqlparser import ENodeType

Continue = 0
Break = 1
ContinueAndNotify = 2

node_functions = {}


class NodeVisitor:

    def __init__(self, root_node):
        self.root = root_node
        self.node_functions = {}
        tmp = dir(self)

        if 'visit_node' in tmp:
            self.has_visit_node = True
        else:
            self.has_visit_node = False

        if 'visit_string' in tmp:
            self.has_visit_string = True
        else:
            self.has_visit_string = False

        if 'visit_generic' in tmp:
            self.has_visit_generic = True
        else:
            self.has_visit_generic = False

        for k in dir(self.__class__):
            func = getattr(self.__class__, k)
            if hasattr(func, 'when'):
                self.node_functions[func.when] = func

    def traverse(self, node=None, d=0, name='root'):
        if not node:
            node = self.root
        ret = Continue

        if self.has_visit_generic:
            self.visit_generic(node, d, name)
        if isinstance(node, sqlparser.Node):
            if node.node_type in self.node_functions:
                ret = self.node_functions[node.node_type](self, node, d, name)
            elif self.has_visit_node:
                ret = self.visit_node(node, d, name)

            if node.node_type == ENodeType.list:
                if ret == Continue or ret == ContinueAndNotify:
                    for idx, i in enumerate(node.list):
                        self.traverse(i, d + 1, 'item' + str(idx))
            else:
                if ret == Continue or ret == ContinueAndNotify:
                    for k, v in node.__dict__.iteritems():
                        if v:
                            self.traverse(v, d + 1, k)
                if ret == ContinueAndNotify:
                    self.leave_node(node, d, name)
        elif isinstance(node, str):
            if self.has_visit_string:
                self.visit_string(node, d, name)
        elif isinstance(node, int):
            pass


def when(*args):
    def decorate(f):
        setattr(f, 'when', args[0])
        return f
    return decorate
