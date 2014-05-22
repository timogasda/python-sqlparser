#include "node_visitor.h"
#include "gsp_base.h"
#include "gsp_node.h"
#include "gsp_list.h"
#include "gsp_sourcetoken.h"
#include "gsp_sqlparser.h"
#include <stdlib.h>
#include <Python.h>
#include <structmember.h>

// Statement object
typedef struct {
	PyObject_HEAD
	gsp_sql_statement *_statement;
} Statement;


void Statement_init_type(PyObject *m);

PyObject *Statement_FromStatement(gsp_sql_statement *stmt);
void Statement_dealloc(Statement *self);
PyObject *Statement_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
PyObject *Statement_getattr(Statement *obj, char *name);
PyObject* Statement_iterate_node_next(PyObject* self, PyObject* args);
PyObject* Statement_get_root(PyObject* self, PyObject* args);
PyObject* Statement_remove_whereclause(PyObject* self, PyObject* args);
PyObject* Statement_remove_orderby(PyObject* self, PyObject* args);
PyObject* Statement_remove_groupby(PyObject* self, PyObject* args);
PyObject* Statement_remove_havingclause(PyObject* self, PyObject* args);
PyObject* Statement_remove_expression(PyObject* self, PyObject* args);
PyObject* Statement_remove_joinitem(PyObject* self, PyObject* args);
PyObject* Statement_remove_resultcolumn(PyObject* self, PyObject* args);
PyObject* Statement_remove_orderbyitem(PyObject* self, PyObject* args);
PyObject* Statement_remove_groupbyitem(PyObject* self, PyObject* args);
PyObject* Statement_add_whereclause(PyObject* self, PyObject* args);
PyObject* Statement_add_orderby(PyObject* self, PyObject* args);
PyObject* Statement_add_groupby(PyObject* self, PyObject* args);
PyObject* Statement_add_havingclause(PyObject* self, PyObject* args);
PyObject* Statement_add_joinitem(PyObject* self, PyObject* args);
PyObject* Statement_add_resultcolumn(PyObject* self, PyObject* args);


// Members/properties
static PyMemberDef Statement_members[] = {
    {NULL}  /* Sentinel */
};

// Object methods
static PyMethodDef Statement_methods[] = {
	{"get_root", (PyCFunction)Statement_get_root, METH_VARARGS,  "get_root()\nGets the root :class:`Node` of the statement.\n\n:returns: :class:`Node`" },
	{"remove_whereclause", (PyCFunction)Statement_remove_whereclause, METH_VARARGS, "remove_whereclause(node)\nRemoves *node*'s Where clause (if it exists)\n\n:param node: A select statement Node\n:type node: :class:`Node`\n:returns: :class:`Node` -- The updated version of the *node*" },
	{"remove_orderby", (PyCFunction)Statement_remove_orderby, METH_VARARGS, "remove_orderby(node)\nRemoves *node*'s OrderBy clause (if it exists)\n\n:param node: A select statement Node\n:type node: :class:`Node`\n:returns: :class:`Node` -- The updated version of the *node*" },
	{"remove_groupby", (PyCFunction)Statement_remove_groupby, METH_VARARGS, "remove_groupby(node)\nRemoves *node*'s GroupBy clause (if it exists)\n\n:param node: A select statement Node\n:type node: :class:`Node`\n:returns: :class:`Node` -- The updated version of the *node*" },
	{"remove_havingclause", (PyCFunction)Statement_remove_havingclause, METH_VARARGS, "remove_havingclause(node)\nRemoves *node*'s Having clause (if it exists)\n\n:param node: A select statement Node\n:type node: :class:`Node`\n:returns: :class:`Node` -- The updated version of the *node*" },
	{"remove_expression", (PyCFunction)Statement_remove_expression, METH_VARARGS, "remove_expression(node)\nRemoves the expression *node*.\n\n:param node: An expression Node\n:type node: :class:`Node`\n:returns: :class:`Node` -- The updated version of the *node*" },
	{"remove_joinitem", (PyCFunction)Statement_remove_joinitem, METH_VARARGS, "remove_joinitem(node, index)\nRemoves *node*'s Join item at the specified *index*\n\n:param node: A base statement Node\n:type node: :class:`Node`\n:param index: The item's index in the Join clause\n:type index: int\n:returns: :class:`Node` -- The updated version of the *node*" },
	{"remove_resultcolumn", (PyCFunction)Statement_remove_resultcolumn, METH_VARARGS, "remove_resultcolumn(node, index)\nRemoves *node*'s Result column at the specified *index*\n\n:param node: A select statement Node\n:type node: :class:`Node`\n:param index: The item's index in the Join clause\n:type index: int\n:returns: :class:`Node` -- The updated version of the *node*" },
	{"remove_orderbyitem", (PyCFunction)Statement_remove_orderbyitem, METH_VARARGS, "remove_orderbyitem(node, index)\nRemoves *node*'s OrderBy item at the specified *index*\n\n:param node: A base statement Node\n:type node: :class:`Node`\n:param index: The item's index in the Join clause\n:type index: int\n:returns: :class:`Node` -- The updated version of the *node*" },
	{"remove_groupbyitem", (PyCFunction)Statement_remove_groupbyitem, METH_VARARGS, "remove_groupbyitem(node, index)\nRemoves *node*'s GroupBy item at the specified *index*\n\n:param node: A base statement Node\n:type node: :class:`Node`\n:param index: The item's index in the Join clause\n:type index: int\n:returns: :class:`Node` -- The updated version of the *node*" },
	{"add_whereclause", (PyCFunction)Statement_add_whereclause, METH_VARARGS, "add_whereclause(node, text)\nAdds a new Where clause to *node*.\n\n:param node: A base statement Node\n:type node: :class:`Node`\n:param text: String representation of a Where clause\n:type text: str\n:returns: str -- The updated query string" },
	{"add_orderby", (PyCFunction)Statement_add_orderby, METH_VARARGS, "add_orderby(node, text)\nAdds a new OrderBy clause to *node*.\n\n:param node: A select statement Node\n:type node: :class:`Node`\n:param text: String representation of an OrderBy clause\n:type text: str\n:returns: str -- The updated query string" },
	{"add_groupby", (PyCFunction)Statement_add_groupby, METH_VARARGS, "add_groupby(node, text)\nAdds a new GroupBy clause to *node*.\n\n:param node: A select statement Node\n:type node: :class:`Node`\n:param text: String representation of a GroupBy clause\n:type text: str\n:returns: str -- The updated query string" },
	{"add_havingclause", (PyCFunction)Statement_add_havingclause, METH_VARARGS, "add_havingclause(node, text)\nAdds a new Having clause to *node*.\n\n:param node: A select statement Node\n:type node: :class:`Node`\n:param text: String representation of a Having clause\n:type text: str\n:returns: str -- The updated query string" },
	{"add_joinitem", (PyCFunction)Statement_add_joinitem, METH_VARARGS, "add_joinitem(node, text)\nAdds a new Join item to *node*.\n\n:param node: A select statement Node\n:type node: :class:`Node`\n:param text: String representation of a Joint item\n:type text: str\n:returns: str -- The updated query string" },
	{"add_resultcolumn", (PyCFunction)Statement_add_resultcolumn, METH_VARARGS, "add_resultcolumn(node, text)\nAdds a new Result column to *node*.\n\n:param node: A base statement Node\n:type node: :class:`Node`\n:param text: String representation of a Result column\n:type text: str\n:returns: str -- The updated query string" },
    {NULL}  /* Sentinel */
};

// Type object for Statement
static PyTypeObject StatementType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "parsebridge.Statement",             /*tp_name*/
    sizeof(Statement), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
	(destructor)Statement_dealloc,                         /*tp_dealloc*/
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
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "Statement objects",           /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    Statement_methods,          /* tp_methods */
	Statement_members,                       /* tp_members */
    0,                       /* tp_getset */
    0,                       /* tp_base */
    0,                       /* tp_dict */
    0,                       /* tp_descr_get */
    0,                       /* tp_descr_set */
    0,                       /* tp_dictoffset */
    0,                      /* tp_init */
    0,                       /* tp_alloc */
    Statement_new,                       /* tp_new */	
}; 