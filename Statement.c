#include "Statement.h"
#include "Node.h"
#include "modifysql.h"

// Initialize Statement Type
void Statement_init_type(PyObject *m) {
	if (PyType_Ready(&StatementType) < 0)
		return;
	Py_INCREF(&StatementType);
	PyModule_AddObject(m, "Statement", (PyObject*)&StatementType);
}

PyObject *Statement_FromStatement(gsp_sql_statement *stmt) {
	Statement *self;
	self = (Statement*)Statement_new(&StatementType, NULL, NULL);
	self->_statement = stmt;
	return (PyObject*)self;
}

// Get root node
// Statement.get_root()
PyObject* Statement_get_root(PyObject* self, PyObject* args) 
{
	SqlNode *n;
	Statement *stmt = (Statement*) self;

	//printf("Statement_get_root\n");

	if (((Statement*)self)->_statement->stmt == NULL) {
		Py_RETURN_NONE;
	}

	//n = (SqlNode*) Node_FromNode( ((Statement*)self)->_statement->parseTree, (Statement*) self );
	n = (SqlNode*) Node_FromNode( (gsp_node*) ((Statement*)self)->_statement->stmt, (Statement*) self );
	
	//Py_XINCREF(n);
	return (PyObject*)n;
}

// Statement.remove_whereclause(node)
PyObject* Statement_remove_whereclause(PyObject* self, PyObject* args) 
{
	PyObject* node;
	PyObject *newNode;
	Statement *stmt = (Statement*) self;

	if (!PyArg_ParseTuple(args, "O", &node)) {
		PyErr_SetString(PyExc_TypeError, "remove_whereclause() takes exactly one node argument");
		return NULL;		
	}

	gsp_removeWhereClause((gsp_base_statement*) ((SqlNode*)node)->_node);
	newNode = Node_FromNode(((SqlNode*)node)->_node, (Statement*) self);

	return newNode;
}

// Statement.remove_orderby(selectNode)
PyObject* Statement_remove_orderby(PyObject* self, PyObject* args)
{
	PyObject* node;
	PyObject *newNode;
	Statement *stmt = (Statement*) self;

	if (!PyArg_ParseTuple(args, "O", &node)) {
		PyErr_SetString(PyExc_TypeError, "remove_orderby() takes exactly one node argument");
		return NULL;		
	}

	gsp_removeOrderBy((gsp_selectStatement*) ((SqlNode*)node)->_node);
	newNode = Node_FromNode(((SqlNode*)node)->_node, (Statement*) self);

	return newNode;
}


// Statement.remove_groupby(selectNode)
PyObject* Statement_remove_groupby(PyObject* self, PyObject* args)
{
	PyObject* node;
	PyObject *newNode;
	Statement *stmt = (Statement*) self;

	if (!PyArg_ParseTuple(args, "O", &node)) {
		PyErr_SetString(PyExc_TypeError, "remove_groupby() takes exactly one node argument");
		return NULL;		
	}

	gsp_removeGroupBy((gsp_selectStatement*) ((SqlNode*)node)->_node);
	newNode = Node_FromNode(((SqlNode*)node)->_node, (Statement*) self);

	return newNode;
}


// Statement.remove_havingclause(selectNode)
PyObject* Statement_remove_havingclause(PyObject* self, PyObject* args)
{
	PyObject* node;
	PyObject *newNode;
	Statement *stmt = (Statement*) self;

	if (!PyArg_ParseTuple(args, "O", &node)) {
		PyErr_SetString(PyExc_TypeError, "remove_havingclause() takes exactly one node argument");
		return NULL;		
	}

	gsp_removeHavingClause((gsp_selectStatement*) ((SqlNode*)node)->_node);
	newNode = Node_FromNode(((SqlNode*)node)->_node, (Statement*) self);

	return newNode;
}


// Statement.remove_expression(exprNode)
PyObject* Statement_remove_expression(PyObject* self, PyObject* args)
{
	PyObject* node;
	PyObject *newNode;
	Statement *stmt = (Statement*) self;

	if (!PyArg_ParseTuple(args, "O", &node)) {
		PyErr_SetString(PyExc_TypeError, "remove_expression() takes exactly one node argument");
		return NULL;
	}

	gsp_removeExpression((gsp_expr*) ((SqlNode*)node)->_node);
	newNode = Node_FromNode(((SqlNode*)node)->_node, (Statement*) self);

	return newNode;
}


// Statement.remove_joinitem(selectNode, joinIndex)
PyObject* Statement_remove_joinitem(PyObject* self, PyObject* args)
{
	PyObject* node;
	PyObject *newNode;
	int i;
	Statement *stmt = (Statement*) self;

	if (!PyArg_ParseTuple(args, "Oi", &node, &i)) {
		PyErr_SetString(PyExc_TypeError, "remove_joinitem() takes exactly two arguments");
		return NULL;
	}

	gsp_removeJoinItem((gsp_selectStatement*) ((SqlNode*)node)->_node, i);
	newNode = Node_FromNode(((SqlNode*)node)->_node, (Statement*) self);

	return newNode;
}


// Statement.remove_resultcolumn(node, columnIndex)
PyObject* Statement_remove_resultcolumn(PyObject* self, PyObject* args)
{
	PyObject* node;
	PyObject *newNode;
	int i;
	Statement *stmt = (Statement*) self;

	if (!PyArg_ParseTuple(args, "Oi", &node, &i)) {
		PyErr_SetString(PyExc_TypeError, "remove_resultcolumn() takes exactly two arguments");
		return NULL;
	}

	gsp_removeResultColumn((gsp_base_statement*) ((Statement*)self)->_statement->stmt, i);
	newNode = Node_FromNode((gsp_node*)((Statement*)self)->_statement->stmt, (Statement*) self);
	return newNode;
}


// Statement.remove_orderbyitem(node, orderByIndex)
PyObject* Statement_remove_orderbyitem(PyObject* self, PyObject* args)
{
	PyObject* node;
	PyObject *newNode;
	int i;
	Statement *stmt = (Statement*) self;

	if (!PyArg_ParseTuple(args, "Oi", &node, &i)) {
		PyErr_SetString(PyExc_TypeError, "remove_orderbyitem() takes exactly two arguments");
		return NULL;
	}

	gsp_removeOrderByItem((gsp_selectStatement*) ((SqlNode*)node)->_node, i);
	newNode = Node_FromNode(((SqlNode*)node)->_node, (Statement*) self);

	return newNode;
}


// Statement.remove_groupbyitem(selectNode, groupbyIndex)
PyObject* Statement_remove_groupbyitem(PyObject* self, PyObject* args)
{
	PyObject* node;
	PyObject *newNode;
	int i;
	Statement *stmt = (Statement*) self;

	if (!PyArg_ParseTuple(args, "Oi", &node, &i)) {
		PyErr_SetString(PyExc_TypeError, "remove_groupby() takes exactly two arguments");
		return NULL;
	}

	gsp_removeGroupByItem((gsp_selectStatement*) ((SqlNode*)node)->_node, i);
	newNode = Node_FromNode(((SqlNode*)node)->_node, (Statement*) self);

	return newNode;
}

PyObject* Statement_add_whereclause(PyObject* self, PyObject* args)
{
	PyObject* node;
	char *text;
	char *newQuery;
	PyObject *newString;
	Statement *stmt = (Statement*) self;

	if (!PyArg_ParseTuple(args, "Os", &node, &text)) {
		PyErr_SetString(PyExc_TypeError, "add_whereclause() takes exactly two arguments");
		return NULL;
	}

	//printf("add_whereclause: %s\n", text);
	//printf("old: %s\n", gsp_getNodeText( (gsp_node*) ((Statement*)self)->_statement->stmt));
	gsp_addWhereClause( ((Statement*)self)->_statement->sqlparser, (gsp_base_statement*) ((SqlNode*)node)->_node, text);
	//printf("new: %s\n", gsp_getNodeText( (gsp_node*) ((Statement*)self)->_statement->stmt));

	newQuery = gsp_getNodeText( (gsp_node*) ((Statement*)self)->_statement->stmt );
	newString = PyString_FromString(newQuery);
	free(newQuery);

	return newString;
}
PyObject* Statement_add_orderby(PyObject* self, PyObject* args)
{
	PyObject* node;
	char *text;
	char *newQuery;
	PyObject *newString;
	Statement *stmt = (Statement*) self;

	if (!PyArg_ParseTuple(args, "Os", &node, &text)) {
		PyErr_SetString(PyExc_TypeError, "add_orderby() takes exactly two arguments");
		return NULL;
	}

	gsp_addOrderBy( ((Statement*)self)->_statement->sqlparser, (gsp_selectStatement*) ((SqlNode*)node)->_node, text);

	newQuery = gsp_getNodeText( (gsp_node*) ((Statement*)self)->_statement->stmt );
	newString = PyString_FromString(newQuery);
	free(newQuery);

	return newString;
}
PyObject* Statement_add_groupby(PyObject* self, PyObject* args)
{
	PyObject* node;
	char *text;
	char *newQuery;
	PyObject *newString;
	Statement *stmt = (Statement*) self;

	if (!PyArg_ParseTuple(args, "Os", &node, &text)) {
		PyErr_SetString(PyExc_TypeError, "add_groupby() takes exactly two arguments");
		return NULL;
	}

	gsp_addGroupBy( ((Statement*)self)->_statement->sqlparser, (gsp_selectStatement*) ((SqlNode*)node)->_node, text);

	newQuery = gsp_getNodeText( (gsp_node*) ((Statement*)self)->_statement->stmt );
	newString = PyString_FromString(newQuery);
	free(newQuery);

	return newString;
}
PyObject* Statement_add_havingclause(PyObject* self, PyObject* args)
{
	PyObject* node;
	char *text;
	char *newQuery;
	PyObject *newString;
	Statement *stmt = (Statement*) self;

	if (!PyArg_ParseTuple(args, "Os", &node, &text)) {
		PyErr_SetString(PyExc_TypeError, "add_havingclause() takes exactly two arguments");
		return NULL;
	}

	gsp_addHavingClause( ((Statement*)self)->_statement->sqlparser, (gsp_selectStatement*) ((SqlNode*)node)->_node, text);

	newQuery = gsp_getNodeText( (gsp_node*) ((Statement*)self)->_statement->stmt );
	newString = PyString_FromString(newQuery);
	free(newQuery);

	return newString;
}
PyObject* Statement_add_joinitem(PyObject* self, PyObject* args)
{
	PyObject* node;
	char *text;
	char *newQuery;
	PyObject *newString;
	Statement *stmt = (Statement*) self;

	if (!PyArg_ParseTuple(args, "Os", &node, &text)) {
		PyErr_SetString(PyExc_TypeError, "add_joinitem() takes exactly two arguments");
		return NULL;
	}

	gsp_addJoinItem( ((Statement*)self)->_statement->sqlparser, (gsp_selectStatement*) ((SqlNode*)node)->_node, text);

	newQuery = gsp_getNodeText( (gsp_node*) ((Statement*)self)->_statement->stmt );
	newString = PyString_FromString(newQuery);
	free(newQuery);

	return newString;
}
PyObject* Statement_add_resultcolumn(PyObject* self, PyObject* args)
{
	PyObject* node;
	char *text;
	char *newQuery;
	PyObject *newString;
	Statement *stmt = (Statement*) self;

	if (!PyArg_ParseTuple(args, "Os", &node, &text)) {
		PyErr_SetString(PyExc_TypeError, "add_resultcolumn() takes exactly two arguments");
		return NULL;
	}

	gsp_addResultColumn( ((Statement*)self)->_statement->sqlparser, (gsp_base_statement*) ((SqlNode*)node)->_node, text);
	newQuery = gsp_getNodeText( (gsp_node*) ((Statement*)self)->_statement->stmt );
	newString = PyString_FromString(newQuery);
	free(newQuery);
	return newString;
}

void Statement_dealloc(Statement *self)
{   
	//printf("Statement_dealloc\n"); 
    self->ob_type->tp_free((PyObject*)self);
}

PyObject *Statement_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Statement *self;

	self = (Statement *)type->tp_alloc(type, 0);
	
	if (self != NULL) {
		self->_statement = NULL;
	}
	
	return (PyObject*) self;
}