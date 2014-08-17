#include "gsp_base.h"
#include "gsp_node.h"
#include "gsp_list.h"
#include "gsp_sourcetoken.h"
#include "gsp_sqlparser.h"
#include <stdlib.h>
#include <Python.h>
#include <structmember.h>

// Parser object
typedef struct {
	PyObject_HEAD
	gsp_sqlparser *_parser;
	int vendor;
} Parser;

void Parser_init_type(PyObject *m);
void Parser_dealloc(Parser *self);
PyObject *Parser_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
PyObject *Parser_check_syntax(PyObject* self, PyObject* args);
PyObject *Parser_tokenize(PyObject* self, PyObject* args);
PyObject *Parser_get_statement(PyObject* self, PyObject* args);
PyObject *Parser_get_statement_count(PyObject* self, PyObject* args);
PyObject *Parser_get_tokens(PyObject* self, PyObject *args);
int Parser_init(Parser* self, PyObject* args, PyObject *kwds);

// Members/properties
static PyMemberDef Parser_members[] = {
	{"vendor", T_INT, offsetof(Parser, vendor), 0, "DB Vendor"},
    {NULL}  /* Sentinel */
};

// Object methods
static PyMethodDef Parser_methods[] = {
    {"check_syntax", (PyCFunction)Parser_check_syntax, METH_VARARGS,  "check_syntax(query)\nChecks syntax of the given *query*. Returns 0 if the query is valid.\n\n:type query: str\n:returns: int -- 0 for success" },
    {"tokenize", (PyCFunction)Parser_tokenize, METH_VARARGS,  "tokenize(query)\nTokenizes the given *query*. Returns a list of (tokenCode, tokenValue) "},
    {"get_statement", (PyCFunction)Parser_get_statement, METH_VARARGS,  "get_statement(n)\nAfter parsing a query string with :meth:`Parser.check_syntax` this function will return the *n*-th :class:`Statement` in that string.\n\n:type n: int\n:returns: Statement" },
    {"get_statement_count", (PyCFunction)Parser_get_statement_count, METH_VARARGS,  "get_statement_count()\nReturns the number of statements for the Parser object\n:returns: int" },
    {NULL}  /* Sentinel */
};

// Type object for Parser
static PyTypeObject ParserType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "parsebridge.Parser",             /*tp_name*/
    sizeof(Parser), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Parser_dealloc,/*tp_dealloc*/
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
    "Represents a gsp_sqlParser object. Responsible for parsing SQL queries and retrieving statements.",           /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    Parser_methods,          /* tp_methods */
	Parser_members,                       /* tp_members */
    0,                       /* tp_getset */
    0,                       /* tp_base */
    0,                       /* tp_dict */
    0,                       /* tp_descr_get */
    0,                       /* tp_descr_set */
    0,                       /* tp_dictoffset */
    (initproc)Parser_init,                        /* tp_init */
    0,                       /* tp_alloc */
    Parser_new,                  /* tp_new */	
}; 