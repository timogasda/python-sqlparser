
#include "Parser.h"
#include "Statement.h"

// Initialize this Type
void Parser_init_type(PyObject *m) {
	if (PyType_Ready(&ParserType) < 0)
		return;
	Py_INCREF(&ParserType);
	PyModule_AddObject(m, "Parser", (PyObject*)&ParserType);
}

// Deallocate object (refcount is 0)
void Parser_dealloc(Parser *self)
{
	//printf("Parser_dealloc\n");
	if (self->_parser != NULL) {
		gsp_parser_free(self->_parser);
	}
	self->ob_type->tp_free((PyObject*)self);
}

// Allocate new Parser object
PyObject *Parser_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Parser *self;
	
	self = (Parser *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->_parser = NULL;
		self->vendor = (int)dbvmssql;
	}
	
	return (PyObject*) self;
}

// Parser.__init__(vendor=0)
int Parser_init(Parser* self, PyObject* args, PyObject *kwds)
{
	int vendor;
	static char *kwlist[] = { "vendor", NULL };
	vendor = dbvmssql;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|i", kwlist, &vendor)) {
		return -1;
	}

	self->vendor = vendor;

	if (gsp_parser_create((gsp_dbvendor) vendor, &self->_parser) != 0) {
		Py_DECREF(self);
		return -1;
	}

	//printf("Parser_init: %d -> %p\n", vendor, self->_parser);

	return 0;
}

// Parser.check_syntax(query)
PyObject* Parser_check_syntax(PyObject* self, PyObject* args)
{
	int rc;
	PyObject *r;
	char *query;
	//PyObject *query_object;
	//PyObject *query_unicode;
	Parser *parser = (Parser *)self;

	query = NULL;

	// get query from first argument
	if (args == NULL || args == Py_None) {
		PyErr_SetString(PyExc_TypeError, "check_syntax() takes at least 1 argument");
		return NULL;	
	}

	rc = PyArg_ParseTuple(args, "s", &query);
	// printf("parse s: %d / %p\n", rc, query);

	/*if (!rc || query == NULL) {
		rc = PyArg_ParseTuple(args, "u", &query);
		// printf("parse u: %d / %p\n", rc, query);
	}*/

	if (!rc || query == NULL) {
		PyErr_SetString(PyExc_TypeError, "check_syntax() takes exactly one string argument");
		return NULL;
	}

	//printf("check_syntax (%p): %s\n", parser->_parser, query);
	rc = gsp_check_syntax(parser->_parser, query);
	r = PyInt_FromLong(rc);
	//Py_XDECREF(r);
	return r;
}

// Parser.tokenize(query) + return tokens
PyObject* Parser_tokenize(PyObject* self, PyObject* args)
{
	/* NB: the query passed in here is held by the parser. If further
	 * processing will done it must be Py_INCREF()'d and stored in the
	 * parser object so it can later be cleaned up.
	 */
	int rc;
	char *query;
	PyObject *token_tuple;
	PyObject *list;
	//PyObject *query_object;
	//PyObject *query_unicode;
	Parser *parser = (Parser *)self;
	char * token_str;
	struct gsp_sourcetoken *token;
	int i;
	int token_cnt;

	query = NULL;

	// get query from first argument
	if (args == NULL || args == Py_None) {
		PyErr_SetString(PyExc_TypeError, "tokenize() takes at least 1 argument");
		return NULL;	
	}

	rc = PyArg_ParseTuple(args, "s", &query);
	/*if (!rc || query == NULL) {
		rc = PyArg_ParseTuple(args, "u", &query);
		// printf("parse u: %d / %p\n", rc, query);
	}*/

	if (!rc || query == NULL) {
		PyErr_SetString(PyExc_TypeError, "tokenize() takes exactly one string argument");
		return NULL;
	}

	rc = gsp_tokenize(parser->_parser, query);

	if (rc != 0) {
		PyErr_SetString(PyExc_BaseException, "tokenize() failed");
		return NULL;
	}

	token_cnt = parser->_parser->number_of_token;
	list = PyList_New(token_cnt);
	if (!list) {
		PyErr_SetString(PyExc_BaseException,
			"tokenize() unable to create list");
		return NULL;
	}

	for (i = 0; i < token_cnt; i++) {
		token = &parser->_parser->sourcetokenlist[i];

		// TODO: any unicode handling?
		token_str = gsp_token_text(token);

		if (!token_str) {
			PyErr_SetString(PyExc_BaseException, "tokenize() invalid token");
			Py_DECREF(list);
			return NULL;
		}

		//printf("STR: %s\n", token_str);
		//gsp_print_token(token);

		token_tuple = Py_BuildValue("(is)", token->nCode, token_str);
		free(token_str);

		if (!token_tuple) {
			PyErr_SetString(PyExc_BaseException, "tokenize() tuple failed");
			Py_DECREF(list);
			return NULL;
		}

		PyList_SET_ITEM(list, i, token_tuple);
	}

	return list;
}


// get nth statement
// Parser.get_statement(n)
PyObject* Parser_get_statement(PyObject* self, PyObject* args)
{
	int n;
	gsp_sql_statement *stmt;
	Statement *statement;
	Parser *parser = (Parser *)self;

	n = -1;

	if (!PyArg_ParseTuple(args, "i", &n)) {
		PyErr_SetString(PyExc_TypeError, "get_statement() takes exactly one integer argument");
		return NULL;
	}
	
	if (n < 0 || n >= parser->_parser->nStatement) {
		PyErr_SetString(PyExc_ValueError, "get_statement() index out of bounds");
		return NULL;
	}

	stmt = &parser->_parser->pStatement[n];

	if (stmt->parseTree == NULL && stmt->stmt == NULL) {
		// Invalid syntax
		Py_RETURN_NONE;
	}

	statement = (Statement*) Statement_FromStatement(stmt);
	//Py_XDECREF(statement);
	return (PyObject*) statement;
}

// Parser.get_statement_count()
PyObject* Parser_get_statement_count(PyObject* self, PyObject* args)
{
	PyObject *n;
	Parser *parser = (Parser *)self;

	n = PyLong_FromLong(parser->_parser->nStatement);

	return n;
}
