#include "node_visitor.h"
#include "gsp_base.h"
#include "gsp_node.h"
#include "gsp_list.h"
#include "gsp_sourcetoken.h"
#include "gsp_sqlparser.h"
#include <stdlib.h>
#include <Python.h>
#include <structmember.h>

// Node object
typedef struct {
	PyObject_HEAD
	PyObject *dict;
	gsp_node *_node;
	gsp_sqlparser *_parser;
} SqlNode;

void Node_init_type(PyObject *m);
void Node_init();
PyObject *Node_FromNode(gsp_node *node, Statement *stmt);
PyObject *Node_list_iterator(PyObject *o);
void Node_dealloc(SqlNode *self);
PyObject *Node_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
PyObject *Node_free(PyObject *self, PyObject *args);

PyObject *Node_get_text(SqlNode *self, PyObject *args);
PyObject *Node_get_position(SqlNode *self, PyObject *args);
PyObject *Node_getattro(SqlNode *self, PyObject *args);

// Members/properties
static PyMemberDef Node_members[] = {
	{"__dict__", T_OBJECT, offsetof(SqlNode, dict), READONLY},
    {NULL}  /* Sentinel */
};

// Object methods
static PyMethodDef Node_methods[] = {
	{"get_text", (PyCFunction)Node_get_text, METH_VARARGS,  "get_text()\nGets the string representation of the node.\n\n:returns: str" },
	{"get_position", (PyCFunction)Node_get_position, METH_NOARGS,  "get_position()\nGets the node's position in the query.\n\n:returns: tuple" },
    {NULL}  /* Sentinel */
};

// Type object for Statement
static PyTypeObject NodeType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "parsebridge.Node",             /*tp_name*/
    sizeof(Node), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Node_dealloc,       /*tp_dealloc*/
    0,                         /*tp_print*/
	0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    (getattrofunc)Node_getattro,                          /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "Node objects",           /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
	Node_list_iterator,                       /* tp_iter */
    0,                       /* tp_iternext */
    Node_methods,          /* tp_methods */
	Node_members,                       /* tp_members */
    0,                       /* tp_getset */
    0,                       /* tp_base */
    0,                       /* tp_dict */
    0,                       /* tp_descr_get */
    0,                       /* tp_descr_set */
	offsetof(SqlNode, dict),          /* tp_dictoffset */
    0,                       /* tp_init */
    0,                       /* tp_alloc */
    Node_new,                       /* tp_new */	
}; 


#define MAX_NODE_PARSE_FUNCS 300 // arbitrary

typedef PyObject *(*NodeParseFunc)(gsp_node *, Statement *);
static NodeParseFunc Node_parse_functions[MAX_NODE_PARSE_FUNCS];
