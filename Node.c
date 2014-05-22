#include "Statement.h"
#include "Node.h"
#include "cstring.h"
#include "modifysql.h"

// Initialize this Type
void Node_init_type(PyObject *m) {
	// Fill node parsing functions
	Node_init();

	if (PyType_Ready(&NodeType) < 0)
		return;
	Py_INCREF(&NodeType);
	PyModule_AddObject(m, "Node", (PyObject*)&NodeType);
}

/*char* gsp_getNodeText( gsp_node* node )
{
	gsp_sourcetoken* startToken = NULL;
	gsp_sourcetoken* endToken = NULL;
	gsp_sourcetoken* currentToken = NULL;
	CString* content;
	char* rc; 

	if(node == NULL)
		return NULL;
	if (node->nodeType == t_gsp_list)
	{
		if(((gsp_list*)node)->length>0){
			startToken = (gsp_list_first((gsp_list *)node))->fragment.startToken;
			endToken = (gsp_list_last((gsp_list *)node))->fragment.endToken;
		}

	}else{
		startToken = node->fragment.startToken;
		endToken = node->fragment.endToken;
	}

	currentToken = startToken;
	if(currentToken == NULL)
		return NULL;


	while( startToken!=NULL && startToken!=endToken && startToken->tokenStatus == ets_deleted){
		startToken = startToken->pNext;
	}

	if(startToken == NULL || startToken->tokenStatus == ets_deleted){
		startToken = NULL;
		endToken = NULL;
		return NULL;
	}
	else{
		while(endToken!=NULL && endToken!=startToken && endToken->tokenStatus == ets_deleted){
			endToken = endToken->pPrev;
		}

		if(endToken == NULL || endToken->tokenStatus == ets_deleted){
			startToken = NULL;
			endToken = NULL;
			return NULL;
		}

		content = CStringNew();

		if(currentToken->tokenStatus!=ets_deleted)
			CStringNAppend(content, currentToken->pStr, currentToken->nStrLen);

		while(currentToken != endToken && currentToken->pNext!=NULL){
			currentToken = currentToken->pNext;
			if(currentToken->tokenStatus!=ets_deleted)
				CStringNAppend(content, currentToken->pStr, currentToken->nStrLen);
			if(currentToken == endToken)
				break;
		}

		rc = content->buffer;
		CStringDeleteWithoutBuffer(content);
		return rc;
	}
}*/

char* gsp_getSimpleNodeText(gsp_node* node, gsp_sqlparser *parser)
{
	gsp_sourcetoken* startToken = NULL;
	gsp_sourcetoken* endToken = NULL;
	gsp_sourcetoken* currentToken = NULL;
	CString* content;
	char* rc;

	if (node == NULL)
		return NULL;
	if (node->nodeType == t_gsp_list)
	{
		if (((gsp_list*)node)->length>0){
			startToken = (gsp_list_first((gsp_list *)node))->fragment.startToken;
			endToken = (gsp_list_last((gsp_list *)node))->fragment.endToken;
		}

	}
	else{
		startToken = node->fragment.startToken;
		endToken = node->fragment.endToken;
	}

	currentToken = startToken;
	if (currentToken == NULL)
		return NULL;


	while (startToken != NULL && startToken != endToken && startToken->tokenStatus == ets_deleted){
		startToken = startToken->pNext;
	}

	if (startToken == NULL || startToken->tokenStatus == ets_deleted){
		startToken = NULL;
		endToken = NULL;
		return NULL;
	}
	else{
		int start, stop, len;

		while (endToken != NULL && endToken != startToken && endToken->tokenStatus == ets_deleted){
			endToken = endToken->pPrev;
		}

		if (endToken == NULL || endToken->tokenStatus == ets_deleted){
			startToken = NULL;
			endToken = NULL;
			return NULL;
		}

		content = CStringNew();

		if (startToken == endToken) {
			start = startToken->nColumn - 1;
			stop = start + startToken->nStrLen;
			len = startToken->nStrLen;
		}
		else {
			start = startToken->nColumn - 1;
			stop = endToken->nColumn + endToken->nStrLen - 1;
			len = stop - start;
		}


		//printf("%d, %d, %d\n", start, stop, len);

		CStringNAppend(content, parser->sqltext + start, len);

		rc = content->buffer;
		CStringDeleteWithoutBuffer(content);
		return rc;
	}
}

PyObject *Node_get_text(SqlNode *self, PyObject *args)
{
	char *tmp;
	PyObject *name;

	//tmp = gsp_getNodeText(self->_node);
	tmp = gsp_getSimpleNodeText(self->_node, self->_parser);
	if (tmp == NULL) {
		name = PyString_FromString("");
	}
	else {
		name = PyString_FromString(tmp);
		gsp_free(tmp);
	}
	
	//Py_XINCREF(name);
	return name;
}

PyObject *Node_get_position(SqlNode *self, PyObject *args)
{
	if (self->_node->fragment.startToken) {
		char *name;
		PyObject *pos;

		//name = gsp_getNodeText(self->_node);
		name = gsp_getSimpleNodeText(self->_node, self->_parser);

		if (name == NULL) {
			Py_RETURN_NONE;
		}

		pos = Py_BuildValue("(ii)", self->_node->fragment.startToken->nColumn, strlen(name));
		free(name);
		return pos;
	}
	Py_RETURN_NONE;
}

// Provide a (standard) iterator if it's a list node
PyObject *Node_list_iterator(PyObject *o) {
	SqlNode *node = (SqlNode*) o;

	if (node->_node->nodeType == t_gsp_list) {
		PyObject *iter = PyObject_GetIter(PyDict_GetItemString(node->dict, "list"));
		Py_XDECREF(iter);
		return iter;
	}

	Py_RETURN_NONE;
}

PyObject *Node_getattro(SqlNode *self, PyObject *name)
{
	if (strcmp(PyString_AS_STRING(name), "node_text") == 0) {
		return Node_get_text(self, NULL);
	}

	return PyObject_GenericGetAttr((PyObject*)self, name);
}

PyObject *Node_FromNode(gsp_node *node, Statement *stmt) {
	PyObject *self;
	PyObject *type;
	//PyObject *name;
	//char *tmp;

	//printf("Node_FromNode(%d) : %p\n", node->nodeType, node);
	
	if (Node_parse_functions[node->nodeType] != NULL) {
		self = Node_parse_functions[node->nodeType](node, stmt);

		if (self->ob_type == &NodeType) {
			// it's a SqlNode object!
			type = PyInt_FromLong(node->nodeType);
			PyDict_SetItemString(((SqlNode*)self)->dict, "node_type", type);

			//Py_XDECREF(type);
			if (node->nodeType == t_gsp_list) {
				/*name = PyString_FromString("LIST");
				PyDict_SetItemString(((SqlNode*)self)->dict, "node_text", name);
				Py_XDECREF(name);*/
			} else {
				/*tmp = gsp_getNodeText(node);
				name = PyString_FromString(tmp);
				gsp_free(tmp);
				PyDict_SetItemString(((SqlNode*)self)->dict, "node_text", name);
				Py_XDECREF(name);*/
			}
		} else {
			// it's something else! maybe a list. we dont judge.
		}
	} else {
		type = PyInt_FromLong(node->nodeType);
		//printf("NO PARSER FOR NODE TYPE: %d\n", node->nodeType);
		self = Node_new(&NodeType, NULL, NULL);
		PyDict_SetItemString(((SqlNode*)self)->dict, "node_type", type);

		/*tmp = gsp_getNodeText(node);
		name = PyString_FromString(tmp);
		gsp_free(tmp);
		PyDict_SetItemString(((SqlNode*)self)->dict, "node_text", name);
		Py_XDECREF(name);*/
	}

	//printf("Node_FromNode: %p %p\n", stmt->_statement, stmt->_statement->sqlparser);
	((SqlNode*)self)->_parser = stmt->_statement->sqlparser;

	return (PyObject*)self;
}

void Node_dealloc(SqlNode *self)
{
	//printf("Node_dealloc %p (%d)\n", self, self->ob_refcnt);
	Py_XDECREF(self->dict);
    self->ob_type->tp_free((PyObject*)self);
}

PyObject *Node_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	SqlNode *self;
	
	self = (SqlNode *)type->tp_alloc(type, 0);

	if (self != NULL) {
		self->_node = NULL;
		self->_parser = NULL;
		self->dict = PyDict_New();
	}
	return (PyObject*) self;
}


#define ADD_TOKEN(d, path, name) if (path->name) { PyObject *o = PyString_FromStringAndSize(path->name->pStr, path->name->nStrLen); PyDict_SetItemString(d, #name, o); Py_XDECREF(o); } else { PyDict_SetItemString(d, #name, Py_None); } 
#define ADD_INT(d, path, name) if (true) { PyObject *o = PyInt_FromLong(path->name); PyDict_SetItemString(d, #name, o); Py_XDECREF(o); }
#define ADD_NODE(d, path, name) if (path->name) { PyObject *o = Node_FromNode((gsp_node*)path->name, stmt); PyDict_SetItemString(d, #name, o); Py_XDECREF(o); } else { PyDict_SetItemString(d, #name, Py_None); } 
#define ADD_LIST(d, path, name) if (path->name) { PyObject *o = Node_parsepath->name; PyDict_SetItemString(d, #name, o); Py_XDECREF(o); } else { PyDict_SetItemString(d, #name, Py_None); } 

PyObject *Node_parse_list(gsp_node *node, Statement *stmt)
{
	PyObject *list;
	struct gsp_listcell *cell;

	

	// generate new Node object
	SqlNode *obj;
	obj = (SqlNode*) Node_new(&NodeType, Py_None, Py_None); obj->_node = node;

	// New list
	list = PyList_New(0);
	foreach(cell, ((gsp_list*)node)) {
		// generate new Node object from list item
		PyObject *o;
		
		o= Node_FromNode((gsp_node*)cell->node, stmt);
		// add to list
		PyList_Append(list, (PyObject*) o);
		Py_XDECREF(o);
	}

	// Add list to node
	PyDict_SetItemString(obj->dict, "list", list);
	Py_XDECREF(list);

	return (PyObject*) obj;
}

/// AUTO-GENERATED PARSING FUNCTIONS
PyObject *Node_parse_listcell(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_listcell*)node), node);
	ADD_NODE(obj->dict, ((gsp_listcell*)node), nextCell);
	return (PyObject*)obj;
}
PyObject *Node_parse_sql_statement(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_sql_statement*)node), stmtType);
	ADD_NODE(obj->dict, ((gsp_sql_statement*)node), parseTree);
	ADD_INT(obj->dict, ((gsp_sql_statement*)node), start_token_pos);
	ADD_INT(obj->dict, ((gsp_sql_statement*)node), end_token_pos);
	ADD_INT(obj->dict, ((gsp_sql_statement*)node), bCteQuery);
	ADD_INT(obj->dict, ((gsp_sql_statement*)node), isParsed);
	ADD_INT(obj->dict, ((gsp_sql_statement*)node), dummyTag);
	return (PyObject*)obj;
}
PyObject *Node_parse_dummy(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_dummy*)node), node1);
	ADD_INT(obj->dict, ((gsp_dummy*)node), int1);
	return (PyObject*)obj;
}
PyObject *Node_parse_constant(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_TOKEN(obj->dict, ((gsp_constant*)node), signToken);
	ADD_TOKEN(obj->dict, ((gsp_constant*)node), stringToken);
	ADD_INT(obj->dict, ((gsp_constant*)node), constantType);
	return (PyObject*)obj;
}
PyObject *Node_parse_objectname(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_objectname*)node), objectType);
	ADD_TOKEN(obj->dict, ((gsp_objectname*)node), serverToken);
	ADD_TOKEN(obj->dict, ((gsp_objectname*)node), databaseToken);
	ADD_TOKEN(obj->dict, ((gsp_objectname*)node), schemaToken);
	ADD_TOKEN(obj->dict, ((gsp_objectname*)node), objectToken);
	ADD_TOKEN(obj->dict, ((gsp_objectname*)node), partToken);
	ADD_TOKEN(obj->dict, ((gsp_objectname*)node), propertyToken);
	ADD_NODE(obj->dict, ((gsp_objectname*)node), fields);
	ADD_INT(obj->dict, ((gsp_objectname*)node), nTokens);
	ADD_NODE(obj->dict, ((gsp_objectname*)node), dblink);
	ADD_NODE(obj->dict, ((gsp_objectname*)node), indices);
	return (PyObject*)obj;
}
PyObject *Node_parse_expr(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_expr*)node), expressionType);
	ADD_NODE(obj->dict, ((gsp_expr*)node), objectOperand);
	ADD_TOKEN(obj->dict, ((gsp_expr*)node), operatorToken);
	ADD_NODE(obj->dict, ((gsp_expr*)node), leftOperand);
	ADD_NODE(obj->dict, ((gsp_expr*)node), rightOperand);
	ADD_NODE(obj->dict, ((gsp_expr*)node), subQueryNode);
	ADD_NODE(obj->dict, ((gsp_expr*)node), subQueryStmt);
	ADD_NODE(obj->dict, ((gsp_expr*)node), constantOperand);
	ADD_NODE(obj->dict, ((gsp_expr*)node), exprList);
	ADD_NODE(obj->dict, ((gsp_expr*)node), functionCall);
	ADD_NODE(obj->dict, ((gsp_expr*)node), objectAccess);
	ADD_NODE(obj->dict, ((gsp_expr*)node), caseExpression);
	ADD_TOKEN(obj->dict, ((gsp_expr*)node), quantifier);
	ADD_TOKEN(obj->dict, ((gsp_expr*)node), notToken);
	ADD_NODE(obj->dict, ((gsp_expr*)node), likeEscapeOperand);
	ADD_NODE(obj->dict, ((gsp_expr*)node), betweenOperand);
	ADD_NODE(obj->dict, ((gsp_expr*)node), arrayAccess);
	ADD_NODE(obj->dict, ((gsp_expr*)node), dataTypeName);
	ADD_NODE(obj->dict, ((gsp_expr*)node), intervalExpression);
	ADD_NODE(obj->dict, ((gsp_expr*)node), indices);
	ADD_NODE(obj->dict, ((gsp_expr*)node), newVariantTypeArgumentList);
	return (PyObject*)obj;
}
PyObject *Node_parse_objectAccess(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_objectAccess*)node), objectExpr);
	ADD_NODE(obj->dict, ((gsp_objectAccess*)node), attributes);
	ADD_NODE(obj->dict, ((gsp_objectAccess*)node), method);
	return (PyObject*)obj;
}
PyObject *Node_parse_aliasClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_aliasClause*)node), aliasName);
	ADD_TOKEN(obj->dict, ((gsp_aliasClause*)node), AsToken);
	ADD_NODE(obj->dict, ((gsp_aliasClause*)node), nameList);
	return (PyObject*)obj;
}
PyObject *Node_parse_resultColumn(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_resultColumn*)node), expr);
	ADD_NODE(obj->dict, ((gsp_resultColumn*)node), aliasClause);
	return (PyObject*)obj;
}
PyObject *Node_parse_trimArgument(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_TOKEN(obj->dict, ((gsp_trimArgument*)node), both_trailing_leading);
	ADD_NODE(obj->dict, ((gsp_trimArgument*)node), stringExpression);
	ADD_NODE(obj->dict, ((gsp_trimArgument*)node), trimCharacter);
	return (PyObject*)obj;
}
PyObject *Node_parse_orderByItem(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_orderByItem*)node), sortKey);
	ADD_TOKEN(obj->dict, ((gsp_orderByItem*)node), sortToken);
	return (PyObject*)obj;
}
PyObject *Node_parse_orderBy(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_orderBy*)node), items);
	return (PyObject*)obj;
}
PyObject *Node_parse_keepDenseRankClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_keepDenseRankClause*)node), orderBy);
	return (PyObject*)obj;
}
PyObject *Node_parse_analyticFunction(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_analyticFunction*)node), keepDenseRankClause);
	ADD_NODE(obj->dict, ((gsp_analyticFunction*)node), partitionBy_ExprList);
	ADD_NODE(obj->dict, ((gsp_analyticFunction*)node), orderBy);
	return (PyObject*)obj;
}
PyObject *Node_parse_functionCall(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_functionCall*)node), functionType);
	ADD_NODE(obj->dict, ((gsp_functionCall*)node), functionName);
	ADD_INT(obj->dict, ((gsp_functionCall*)node), aggregateType);
	ADD_NODE(obj->dict, ((gsp_functionCall*)node), trimArgument);
	ADD_NODE(obj->dict, ((gsp_functionCall*)node), analyticFunction);
	ADD_NODE(obj->dict, ((gsp_functionCall*)node), Args);
	ADD_TOKEN(obj->dict, ((gsp_functionCall*)node), extract_time_token);
	ADD_NODE(obj->dict, ((gsp_functionCall*)node), expr1);
	ADD_NODE(obj->dict, ((gsp_functionCall*)node), expr2);
	ADD_NODE(obj->dict, ((gsp_functionCall*)node), expr3);
	ADD_NODE(obj->dict, ((gsp_functionCall*)node), dataTypename);
	ADD_NODE(obj->dict, ((gsp_functionCall*)node), windowDef);
	ADD_NODE(obj->dict, ((gsp_functionCall*)node), sortClause);
	ADD_NODE(obj->dict, ((gsp_functionCall*)node), sortList);
	return (PyObject*)obj;
}
PyObject *Node_parse_whenClauseItem(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_whenClauseItem*)node), comparison_expr);
	ADD_NODE(obj->dict, ((gsp_whenClauseItem*)node), stmts);
	ADD_NODE(obj->dict, ((gsp_whenClauseItem*)node), return_expr);
	ADD_NODE(obj->dict, ((gsp_whenClauseItem*)node), countFractionDescriptionList);
	return (PyObject*)obj;
}
PyObject *Node_parse_caseExpression(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_caseExpression*)node), input_expr);
	ADD_NODE(obj->dict, ((gsp_caseExpression*)node), else_expr);
	ADD_NODE(obj->dict, ((gsp_caseExpression*)node), whenClauseItemList);
	ADD_NODE(obj->dict, ((gsp_caseExpression*)node), else_statement_node_list);
	return (PyObject*)obj;
}
PyObject *Node_parse_intervalExpression(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_precisionScale(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_precisionScale*)node), precision);
	ADD_NODE(obj->dict, ((gsp_precisionScale*)node), scale);
	return (PyObject*)obj;
}
PyObject *Node_parse_typename(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_typename*)node), dataType);
	ADD_NODE(obj->dict, ((gsp_typename*)node), precisionScale);
	ADD_NODE(obj->dict, ((gsp_typename*)node), secondsPrecision);
	ADD_NODE(obj->dict, ((gsp_typename*)node), length);
	ADD_NODE(obj->dict, ((gsp_typename*)node), genericName);
	ADD_NODE(obj->dict, ((gsp_typename*)node), indices);
	ADD_NODE(obj->dict, ((gsp_typename*)node), datatypeAttributeList);
	return (PyObject*)obj;
}
PyObject *Node_parse_keyReference(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_keyReference*)node), referenceType);
	return (PyObject*)obj;
}
PyObject *Node_parse_keyAction(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_keyAction*)node), actionType);
	ADD_NODE(obj->dict, ((gsp_keyAction*)node), keyReference);
	return (PyObject*)obj;
}
PyObject *Node_parse_constraint(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_constraint*)node), constraintType);
	ADD_NODE(obj->dict, ((gsp_constraint*)node), constraintName);
	ADD_NODE(obj->dict, ((gsp_constraint*)node), checkCondition);
	ADD_NODE(obj->dict, ((gsp_constraint*)node), columnList);
	ADD_NODE(obj->dict, ((gsp_constraint*)node), automaticProperties);
	ADD_NODE(obj->dict, ((gsp_constraint*)node), referencedObject);
	ADD_NODE(obj->dict, ((gsp_constraint*)node), referencedColumnList);
	ADD_NODE(obj->dict, ((gsp_constraint*)node), keyActions);
	ADD_NODE(obj->dict, ((gsp_constraint*)node), defaultValue);
	ADD_NODE(obj->dict, ((gsp_constraint*)node), seedExpr);
	ADD_NODE(obj->dict, ((gsp_constraint*)node), incrementExpr);
	return (PyObject*)obj;
}
PyObject *Node_parse_mergeInsertClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mergeInsertClause*)node), columnList);
	ADD_NODE(obj->dict, ((gsp_mergeInsertClause*)node), valuelist);
	ADD_NODE(obj->dict, ((gsp_mergeInsertClause*)node), deleteWhereClause);
	return (PyObject*)obj;
}
PyObject *Node_parse_mergeUpdateClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mergeUpdateClause*)node), updateColumnList);
	ADD_NODE(obj->dict, ((gsp_mergeUpdateClause*)node), updateWhereClause);
	ADD_NODE(obj->dict, ((gsp_mergeUpdateClause*)node), deleteWhereClause);
	return (PyObject*)obj;
}
PyObject *Node_parse_mergeDeleteClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_mergeWhenClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mergeWhenClause*)node), condition);
	ADD_NODE(obj->dict, ((gsp_mergeWhenClause*)node), updateClause);
	ADD_NODE(obj->dict, ((gsp_mergeWhenClause*)node), insertClause);
	ADD_NODE(obj->dict, ((gsp_mergeWhenClause*)node), deleteClause);
	ADD_NODE(obj->dict, ((gsp_mergeWhenClause*)node), signalStmt);
	return (PyObject*)obj;
}
PyObject *Node_parse_dataChangeTable(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_dataChangeTable*)node), stmtNode);
	return (PyObject*)obj;
}
PyObject *Node_parse_fromTable(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_fromTable*)node), aliasClause);
	ADD_INT(obj->dict, ((gsp_fromTable*)node), fromtableType);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), subQueryNode);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), tableName);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), pxGranule);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), tableSample);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), flashback);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), joinExpr);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), tableExpr);
	ADD_TOKEN(obj->dict, ((gsp_fromTable*)node), tableonly);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), dataChangeTable);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), tableHints);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), functionCall);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), openQuery);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), openDatasource);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), containsTable);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), freeTable);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), openRowSet);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), openXML);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), pivotClause);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), unPivotClause);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), rowList);
	ADD_NODE(obj->dict, ((gsp_fromTable*)node), outerClause);
	return (PyObject*)obj;
}
PyObject *Node_parse_table(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_table*)node), tableSource);
	ADD_NODE(obj->dict, ((gsp_table*)node), aliasClause);
	ADD_NODE(obj->dict, ((gsp_table*)node), tableName);
	ADD_NODE(obj->dict, ((gsp_table*)node), tableExpr);
	return (PyObject*)obj;
}
PyObject *Node_parse_mergeSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mergeSqlNode*)node), cteList);
	ADD_NODE(obj->dict, ((gsp_mergeSqlNode*)node), whenClauses);
	ADD_NODE(obj->dict, ((gsp_mergeSqlNode*)node), targetTableNode);
	ADD_NODE(obj->dict, ((gsp_mergeSqlNode*)node), usingTableNode);
	ADD_NODE(obj->dict, ((gsp_mergeSqlNode*)node), condition);
	ADD_NODE(obj->dict, ((gsp_mergeSqlNode*)node), outputClause);
	ADD_NODE(obj->dict, ((gsp_mergeSqlNode*)node), columnList);
	ADD_NODE(obj->dict, ((gsp_mergeSqlNode*)node), targetTable);
	ADD_NODE(obj->dict, ((gsp_mergeSqlNode*)node), usingTable);
	return (PyObject*)obj;
}
PyObject *Node_parse_alterTableOption(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_alterTableOption*)node), optionType);
	ADD_NODE(obj->dict, ((gsp_alterTableOption*)node), constraintName);
	ADD_NODE(obj->dict, ((gsp_alterTableOption*)node), newConstraintName);
	ADD_NODE(obj->dict, ((gsp_alterTableOption*)node), columnName);
	ADD_NODE(obj->dict, ((gsp_alterTableOption*)node), newColumnName);
	ADD_NODE(obj->dict, ((gsp_alterTableOption*)node), referencedObjectName);
	ADD_NODE(obj->dict, ((gsp_alterTableOption*)node), newTableName);
	ADD_NODE(obj->dict, ((gsp_alterTableOption*)node), indexName);
	ADD_NODE(obj->dict, ((gsp_alterTableOption*)node), columnDefinitionList);
	ADD_NODE(obj->dict, ((gsp_alterTableOption*)node), constraintList);
	ADD_NODE(obj->dict, ((gsp_alterTableOption*)node), columnNameList);
	ADD_NODE(obj->dict, ((gsp_alterTableOption*)node), referencedColumnList);
	ADD_NODE(obj->dict, ((gsp_alterTableOption*)node), datatype);
	return (PyObject*)obj;
}
PyObject *Node_parse_alterTableSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_alterTableSqlNode*)node), tableName);
	ADD_NODE(obj->dict, ((gsp_alterTableSqlNode*)node), tableElementList);
	ADD_NODE(obj->dict, ((gsp_alterTableSqlNode*)node), alterTableOptionList);
	return (PyObject*)obj;
}
PyObject *Node_parse_createSequenceSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_createSequenceSqlNode*)node), sequenceName);
	ADD_NODE(obj->dict, ((gsp_createSequenceSqlNode*)node), options);
	return (PyObject*)obj;
}
PyObject *Node_parse_createSynonymSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_createSynonymSqlNode*)node), synonymName);
	ADD_NODE(obj->dict, ((gsp_createSynonymSqlNode*)node), forName);
	ADD_INT(obj->dict, ((gsp_createSynonymSqlNode*)node), isPublic);
	ADD_INT(obj->dict, ((gsp_createSynonymSqlNode*)node), isReplace);
	return (PyObject*)obj;
}
PyObject *Node_parse_createDirectorySqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_createDirectorySqlNode*)node), directoryName);
	ADD_NODE(obj->dict, ((gsp_createDirectorySqlNode*)node), path);
	return (PyObject*)obj;
}
PyObject *Node_parse_dropViewSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_dropViewSqlNode*)node), viewNameList);
	return (PyObject*)obj;
}
PyObject *Node_parse_dropTableSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_dropTableSqlNode*)node), tableNameList);
	return (PyObject*)obj;
}
PyObject *Node_parse_dropIndexItem(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_dropIndexItem*)node), indexName);
	ADD_NODE(obj->dict, ((gsp_dropIndexItem*)node), tableName);
	return (PyObject*)obj;
}
PyObject *Node_parse_dropIndexSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_dropIndexSqlNode*)node), indexName);
	ADD_NODE(obj->dict, ((gsp_dropIndexSqlNode*)node), itemList);
	return (PyObject*)obj;
}
PyObject *Node_parse_truncateTableSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_truncateTableSqlNode*)node), tableName);
	return (PyObject*)obj;
}
PyObject *Node_parse_viewAliasItem(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_viewAliasItem*)node), alias);
	ADD_NODE(obj->dict, ((gsp_viewAliasItem*)node), columnConstraintList);
	ADD_NODE(obj->dict, ((gsp_viewAliasItem*)node), tableConstraint);
	return (PyObject*)obj;
}
PyObject *Node_parse_viewAliasClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_viewAliasClause*)node), viewAliasItemList);
	return (PyObject*)obj;
}
PyObject *Node_parse_createViewSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_createViewSqlNode*)node), viewName);
	ADD_NODE(obj->dict, ((gsp_createViewSqlNode*)node), selectSqlNode);
	ADD_NODE(obj->dict, ((gsp_createViewSqlNode*)node), viewAliasClause);
	ADD_INT(obj->dict, ((gsp_createViewSqlNode*)node), isForce);
	ADD_INT(obj->dict, ((gsp_createViewSqlNode*)node), isReplace);
	ADD_NODE(obj->dict, ((gsp_createViewSqlNode*)node), rowTypeName);
	return (PyObject*)obj;
}
PyObject *Node_parse_createMaterializedViewSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_createMaterializedViewSqlNode*)node), viewName);
	ADD_NODE(obj->dict, ((gsp_createMaterializedViewSqlNode*)node), selectSqlNode);
	ADD_NODE(obj->dict, ((gsp_createMaterializedViewSqlNode*)node), viewAliasClause);
	return (PyObject*)obj;
}
PyObject *Node_parse_createMaterializedViewLogSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_createMaterializedViewLogSqlNode*)node), mvName);
	return (PyObject*)obj;
}
PyObject *Node_parse_createIndexSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_createIndexSqlNode*)node), indexType);
	ADD_NODE(obj->dict, ((gsp_createIndexSqlNode*)node), indexName);
	ADD_NODE(obj->dict, ((gsp_createIndexSqlNode*)node), tableName);
	ADD_NODE(obj->dict, ((gsp_createIndexSqlNode*)node), indexItemList);
	return (PyObject*)obj;
}
PyObject *Node_parse_commitSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_commitSqlNode*)node), transName);
	return (PyObject*)obj;
}
PyObject *Node_parse_rollbackSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_rollbackSqlNode*)node), transName);
	return (PyObject*)obj;
}
PyObject *Node_parse_saveTransSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_saveTransSqlNode*)node), transName);
	return (PyObject*)obj;
}
PyObject *Node_parse_columnDefinition(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_columnDefinition*)node), columnName);
	ADD_NODE(obj->dict, ((gsp_columnDefinition*)node), datatype);
	ADD_NODE(obj->dict, ((gsp_columnDefinition*)node), constraints);
	ADD_NODE(obj->dict, ((gsp_columnDefinition*)node), defaultExpression);
	ADD_NODE(obj->dict, ((gsp_columnDefinition*)node), computedColumnExpression);
	ADD_INT(obj->dict, ((gsp_columnDefinition*)node), isNull);
	ADD_INT(obj->dict, ((gsp_columnDefinition*)node), isRowGuidCol);
	return (PyObject*)obj;
}
PyObject *Node_parse_tableElement(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_tableElement*)node), columnDefinition);
	ADD_NODE(obj->dict, ((gsp_tableElement*)node), constraint);
	return (PyObject*)obj;
}
PyObject *Node_parse_createTableSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_createTableSqlNode*)node), table);
	ADD_NODE(obj->dict, ((gsp_createTableSqlNode*)node), oftable);
	ADD_NODE(obj->dict, ((gsp_createTableSqlNode*)node), tableElementList);
	ADD_NODE(obj->dict, ((gsp_createTableSqlNode*)node), columnList);
	ADD_NODE(obj->dict, ((gsp_createTableSqlNode*)node), columnName);
	ADD_NODE(obj->dict, ((gsp_createTableSqlNode*)node), subQueryNode);
	ADD_NODE(obj->dict, ((gsp_createTableSqlNode*)node), superTableName);
	ADD_NODE(obj->dict, ((gsp_createTableSqlNode*)node), ofTypeName);
	ADD_NODE(obj->dict, ((gsp_createTableSqlNode*)node), likeTableName);
	return (PyObject*)obj;
}
PyObject *Node_parse_returningClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_returningClause*)node), columnValueList);
	ADD_NODE(obj->dict, ((gsp_returningClause*)node), variableList);
	return (PyObject*)obj;
}
PyObject *Node_parse_isolationClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_includeColumns(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_includeColumns*)node), columnList);
	return (PyObject*)obj;
}
PyObject *Node_parse_deleteSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_TOKEN(obj->dict, ((gsp_deleteSqlNode*)node), deleteToken);
	ADD_NODE(obj->dict, ((gsp_deleteSqlNode*)node), cteList);
	ADD_NODE(obj->dict, ((gsp_deleteSqlNode*)node), whereCondition);
	ADD_NODE(obj->dict, ((gsp_deleteSqlNode*)node), returningClause);
	ADD_NODE(obj->dict, ((gsp_deleteSqlNode*)node), isolationClause);
	ADD_NODE(obj->dict, ((gsp_deleteSqlNode*)node), includeColumns);
	ADD_NODE(obj->dict, ((gsp_deleteSqlNode*)node), targetTableNode);
	ADD_NODE(obj->dict, ((gsp_deleteSqlNode*)node), topClause);
	ADD_NODE(obj->dict, ((gsp_deleteSqlNode*)node), outputClause);
	ADD_NODE(obj->dict, ((gsp_deleteSqlNode*)node), sourceTableList);
	ADD_NODE(obj->dict, ((gsp_deleteSqlNode*)node), targetTableList);
	ADD_NODE(obj->dict, ((gsp_deleteSqlNode*)node), limitClause);
	ADD_NODE(obj->dict, ((gsp_deleteSqlNode*)node), sortClause);
	return (PyObject*)obj;
}
PyObject *Node_parse_updateSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_TOKEN(obj->dict, ((gsp_updateSqlNode*)node), updateToken);
	ADD_NODE(obj->dict, ((gsp_updateSqlNode*)node), cteList);
	ADD_NODE(obj->dict, ((gsp_updateSqlNode*)node), resultColumnList);
	ADD_NODE(obj->dict, ((gsp_updateSqlNode*)node), whereCondition);
	ADD_NODE(obj->dict, ((gsp_updateSqlNode*)node), returningClause);
	ADD_NODE(obj->dict, ((gsp_updateSqlNode*)node), isolationClause);
	ADD_NODE(obj->dict, ((gsp_updateSqlNode*)node), includeColumns);
	ADD_NODE(obj->dict, ((gsp_updateSqlNode*)node), targetTableNode);
	ADD_NODE(obj->dict, ((gsp_updateSqlNode*)node), topClause);
	ADD_NODE(obj->dict, ((gsp_updateSqlNode*)node), outputClause);
	ADD_NODE(obj->dict, ((gsp_updateSqlNode*)node), sourceTableList);
	ADD_NODE(obj->dict, ((gsp_updateSqlNode*)node), targetTableList);
	ADD_NODE(obj->dict, ((gsp_updateSqlNode*)node), limitClause);
	ADD_NODE(obj->dict, ((gsp_updateSqlNode*)node), sortClause);
	return (PyObject*)obj;
}
PyObject *Node_parse_multiTarget(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_multiTarget*)node), subQueryNode);
	ADD_NODE(obj->dict, ((gsp_multiTarget*)node), resultColumnList);
	return (PyObject*)obj;
}
PyObject *Node_parse_insertRest(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_insertRest*)node), valueType);
	ADD_NODE(obj->dict, ((gsp_insertRest*)node), multiTargetList);
	ADD_NODE(obj->dict, ((gsp_insertRest*)node), subQueryNode);
	ADD_NODE(obj->dict, ((gsp_insertRest*)node), functionCall);
	ADD_NODE(obj->dict, ((gsp_insertRest*)node), recordName);
	ADD_NODE(obj->dict, ((gsp_insertRest*)node), updateTargetList);
	return (PyObject*)obj;
}
PyObject *Node_parse_insertValuesClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_insertValuesClause*)node), multiTargetList);
	return (PyObject*)obj;
}
PyObject *Node_parse_insertIntoValue(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_insertIntoValue*)node), fromTable);
	ADD_NODE(obj->dict, ((gsp_insertIntoValue*)node), columnList);
	ADD_NODE(obj->dict, ((gsp_insertIntoValue*)node), valuesClause);
	ADD_NODE(obj->dict, ((gsp_insertIntoValue*)node), table);
	return (PyObject*)obj;
}
PyObject *Node_parse_insertCondition(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_insertCondition*)node), condition);
	ADD_NODE(obj->dict, ((gsp_insertCondition*)node), insertIntoValues);
	return (PyObject*)obj;
}
PyObject *Node_parse_insertSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_TOKEN(obj->dict, ((gsp_insertSqlNode*)node), insertToken);
	ADD_INT(obj->dict, ((gsp_insertSqlNode*)node), valueType);
	ADD_NODE(obj->dict, ((gsp_insertSqlNode*)node), cteList);
	ADD_NODE(obj->dict, ((gsp_insertSqlNode*)node), columnList);
	ADD_NODE(obj->dict, ((gsp_insertSqlNode*)node), insertIntoValues);
	ADD_NODE(obj->dict, ((gsp_insertSqlNode*)node), insertConditions);
	ADD_NODE(obj->dict, ((gsp_insertSqlNode*)node), returningClause);
	ADD_NODE(obj->dict, ((gsp_insertSqlNode*)node), isolationClause);
	ADD_NODE(obj->dict, ((gsp_insertSqlNode*)node), includeColumns);
	ADD_NODE(obj->dict, ((gsp_insertSqlNode*)node), targetTableNode);
	ADD_NODE(obj->dict, ((gsp_insertSqlNode*)node), insertRest);
	ADD_NODE(obj->dict, ((gsp_insertSqlNode*)node), subQueryNode);
	ADD_NODE(obj->dict, ((gsp_insertSqlNode*)node), topClause);
	ADD_NODE(obj->dict, ((gsp_insertSqlNode*)node), outputClause);
	ADD_NODE(obj->dict, ((gsp_insertSqlNode*)node), onDuplicateUpdateList);
	ADD_NODE(obj->dict, ((gsp_insertSqlNode*)node), multiTargetList);
	ADD_NODE(obj->dict, ((gsp_insertSqlNode*)node), recordName);
	ADD_NODE(obj->dict, ((gsp_insertSqlNode*)node), functionCall);
	return (PyObject*)obj;
}
PyObject *Node_parse_whereClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_whereClause*)node), condition);
	return (PyObject*)obj;
}
PyObject *Node_parse_joinExpr(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_joinExpr*)node), jointype);
	ADD_INT(obj->dict, ((gsp_joinExpr*)node), original_jontype);
	ADD_NODE(obj->dict, ((gsp_joinExpr*)node), aliasClause);
	ADD_NODE(obj->dict, ((gsp_joinExpr*)node), leftOperand);
	ADD_NODE(obj->dict, ((gsp_joinExpr*)node), rightOperand);
	ADD_NODE(obj->dict, ((gsp_joinExpr*)node), onCondition);
	ADD_NODE(obj->dict, ((gsp_joinExpr*)node), usingColumns);
	return (PyObject*)obj;
}
PyObject *Node_parse_tableSamplePart(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_tableSample(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_pxGranule(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_flashback(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_forUpdate(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_forUpdate*)node), columnRefs);
	return (PyObject*)obj;
}
PyObject *Node_parse_groupingSetItem(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_groupingSetItem*)node), rollupCubeClause);
	ADD_NODE(obj->dict, ((gsp_groupingSetItem*)node), expressionItem);
	return (PyObject*)obj;
}
PyObject *Node_parse_groupingSet(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_groupingSet*)node), items);
	return (PyObject*)obj;
}
PyObject *Node_parse_rollupCube(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_rollupCube*)node), items);
	return (PyObject*)obj;
}
PyObject *Node_parse_gruopByItem(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_gruopByItem*)node), expr);
	ADD_NODE(obj->dict, ((gsp_gruopByItem*)node), rollupCube);
	ADD_NODE(obj->dict, ((gsp_gruopByItem*)node), groupingSet);
	ADD_NODE(obj->dict, ((gsp_gruopByItem*)node), aliasClause);
	return (PyObject*)obj;
}
PyObject *Node_parse_groupBy(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_TOKEN(obj->dict, ((gsp_groupBy*)node), stGroup);
	ADD_TOKEN(obj->dict, ((gsp_groupBy*)node), stBy);
	ADD_TOKEN(obj->dict, ((gsp_groupBy*)node), stHaving);
	ADD_NODE(obj->dict, ((gsp_groupBy*)node), havingClause);
	ADD_NODE(obj->dict, ((gsp_groupBy*)node), items);
	return (PyObject*)obj;
}
PyObject *Node_parse_selectDistinct(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_selectDistinct*)node), exprList);
	return (PyObject*)obj;
}
PyObject *Node_parse_topClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_topClause*)node), bPercent);
	ADD_INT(obj->dict, ((gsp_topClause*)node), bWithTies);
	ADD_NODE(obj->dict, ((gsp_topClause*)node), expr);
	return (PyObject*)obj;
}
PyObject *Node_parse_hierarchical(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_hierarchical*)node), connectByClause);
	ADD_NODE(obj->dict, ((gsp_hierarchical*)node), startWithClause);
	return (PyObject*)obj;
}
PyObject *Node_parse_intoClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_intoClause*)node), exprList);
	return (PyObject*)obj;
}
PyObject *Node_parse_valueClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_valueClause*)node), valueList);
	ADD_NODE(obj->dict, ((gsp_valueClause*)node), nameList);
	return (PyObject*)obj;
}
PyObject *Node_parse_fetchFirstClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_optimizeForClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_valueRowItem(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_valueRowItem*)node), expr);
	ADD_NODE(obj->dict, ((gsp_valueRowItem*)node), exprList);
	return (PyObject*)obj;
}
PyObject *Node_parse_selectSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_selectSqlNode*)node), setOperator);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), cteList);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), valueClause);
	ADD_TOKEN(obj->dict, ((gsp_selectSqlNode*)node), selectToken);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), selectDistinct);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), resultColumnList);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), intoClause);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), whereCondition);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), hierarchicalClause);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), groupByClause);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), orderbyClause);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), forupdateClause);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), fetchFirstClause);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), optimizeForClause);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), isolationClause);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), computeClause);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), topClause);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), intoTableClause);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), limitClause);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), lockingClause);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), windowClause);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), withClauses);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), qualifyClause);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), sampleClause);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), expandOnClause);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), leftNode);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), rightNode);
	ADD_NODE(obj->dict, ((gsp_selectSqlNode*)node), fromTableList);
	return (PyObject*)obj;
}
PyObject *Node_parse_cte(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_cte*)node), tableName);
	ADD_NODE(obj->dict, ((gsp_cte*)node), selectSqlNode);
	ADD_NODE(obj->dict, ((gsp_cte*)node), insertSqlNode);
	ADD_NODE(obj->dict, ((gsp_cte*)node), updateSqlNode);
	ADD_NODE(obj->dict, ((gsp_cte*)node), deleteSqlNode);
	ADD_NODE(obj->dict, ((gsp_cte*)node), columnList);
	return (PyObject*)obj;
}
PyObject *Node_parse_commentSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_commentSqlNode*)node), dbObjType);
	ADD_NODE(obj->dict, ((gsp_commentSqlNode*)node), objectName);
	ADD_NODE(obj->dict, ((gsp_commentSqlNode*)node), message);
	return (PyObject*)obj;
}
PyObject *Node_parse_callSpec(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_callSpec*)node), declaration);
	return (PyObject*)obj;
}
PyObject *Node_parse_returnSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_returnSqlNode*)node), expr);
	ADD_NODE(obj->dict, ((gsp_returnSqlNode*)node), subQueryNode);
	return (PyObject*)obj;
}
PyObject *Node_parse_continueSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_breakSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_grantSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_grantSqlNode*)node), nameList);
	return (PyObject*)obj;
}
PyObject *Node_parse_executeSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_executeSqlNode*)node), moduleName);
	ADD_NODE(obj->dict, ((gsp_executeSqlNode*)node), paramList);
	return (PyObject*)obj;
}
PyObject *Node_parse_dropRoleSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_dropRoleSqlNode*)node), roleNameList);
	return (PyObject*)obj;
}
PyObject *Node_parse_dropTriggerSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_dropTriggerSqlNode*)node), triggerName);
	ADD_NODE(obj->dict, ((gsp_dropTriggerSqlNode*)node), tableName);
	return (PyObject*)obj;
}
PyObject *Node_parse_lockTableSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_revokeSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_fetchSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_fetchSqlNode*)node), cursorName);
	ADD_NODE(obj->dict, ((gsp_fetchSqlNode*)node), variableNames);
	return (PyObject*)obj;
}
PyObject *Node_parse_openSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_openSqlNode*)node), cursorName);
	ADD_NODE(obj->dict, ((gsp_openSqlNode*)node), nameList);
	return (PyObject*)obj;
}
PyObject *Node_parse_closeSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_closeSqlNode*)node), cursorName);
	return (PyObject*)obj;
}
PyObject *Node_parse_iterateSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_iterateSqlNode*)node), cursorName);
	return (PyObject*)obj;
}
PyObject *Node_parse_leaveSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_leaveSqlNode*)node), cursorName);
	return (PyObject*)obj;
}
PyObject *Node_parse_createFunctionSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_createFunctionSqlNode*)node), kind);
	ADD_NODE(obj->dict, ((gsp_createFunctionSqlNode*)node), functionName);
	ADD_NODE(obj->dict, ((gsp_createFunctionSqlNode*)node), parameters);
	ADD_NODE(obj->dict, ((gsp_createFunctionSqlNode*)node), returnDataType);
	ADD_NODE(obj->dict, ((gsp_createFunctionSqlNode*)node), declareStmts);
	ADD_NODE(obj->dict, ((gsp_createFunctionSqlNode*)node), stmts);
	ADD_NODE(obj->dict, ((gsp_createFunctionSqlNode*)node), exceptionClause);
	ADD_NODE(obj->dict, ((gsp_createFunctionSqlNode*)node), returnSqlNode);
	ADD_NODE(obj->dict, ((gsp_createFunctionSqlNode*)node), compoundSqlNode);
	ADD_NODE(obj->dict, ((gsp_createFunctionSqlNode*)node), callSpec);
	ADD_NODE(obj->dict, ((gsp_createFunctionSqlNode*)node), blockSqlNode);
	ADD_NODE(obj->dict, ((gsp_createFunctionSqlNode*)node), stmtSqlNode);
	return (PyObject*)obj;
}
PyObject *Node_parse_parameterDeclaration(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_parameterDeclaration*)node), parameterName);
	ADD_NODE(obj->dict, ((gsp_parameterDeclaration*)node), dataType);
	ADD_NODE(obj->dict, ((gsp_parameterDeclaration*)node), defaultValue);
	ADD_INT(obj->dict, ((gsp_parameterDeclaration*)node), isVarying);
	ADD_NODE(obj->dict, ((gsp_parameterDeclaration*)node), varyPrecision);
	ADD_INT(obj->dict, ((gsp_parameterDeclaration*)node), isNotNull);
	ADD_INT(obj->dict, ((gsp_parameterDeclaration*)node), mode);
	ADD_INT(obj->dict, ((gsp_parameterDeclaration*)node), howtoSetValue);
	return (PyObject*)obj;
}
PyObject *Node_parse_createProcedureSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_createProcedureSqlNode*)node), kind);
	ADD_NODE(obj->dict, ((gsp_createProcedureSqlNode*)node), procedureName);
	ADD_NODE(obj->dict, ((gsp_createProcedureSqlNode*)node), parameters);
	ADD_NODE(obj->dict, ((gsp_createProcedureSqlNode*)node), declareStmts);
	ADD_NODE(obj->dict, ((gsp_createProcedureSqlNode*)node), innerStmts);
	ADD_NODE(obj->dict, ((gsp_createProcedureSqlNode*)node), stmts);
	ADD_NODE(obj->dict, ((gsp_createProcedureSqlNode*)node), exceptionClause);
	ADD_NODE(obj->dict, ((gsp_createProcedureSqlNode*)node), callSpec);
	ADD_NODE(obj->dict, ((gsp_createProcedureSqlNode*)node), blockSqlNode);
	ADD_NODE(obj->dict, ((gsp_createProcedureSqlNode*)node), stmtSqlNode);
	return (PyObject*)obj;
}
PyObject *Node_parse_exceptionClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_exceptionClause*)node), Handlers);
	return (PyObject*)obj;
}
PyObject *Node_parse_blockSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_blockSqlNode*)node), stmts);
	ADD_NODE(obj->dict, ((gsp_blockSqlNode*)node), exceptionClause);
	ADD_NODE(obj->dict, ((gsp_blockSqlNode*)node), declareStmts);
	return (PyObject*)obj;
}
PyObject *Node_parse_arrayAccess(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_arrayAccess*)node), arrayName);
	ADD_NODE(obj->dict, ((gsp_arrayAccess*)node), index1);
	ADD_NODE(obj->dict, ((gsp_arrayAccess*)node), index2);
	ADD_NODE(obj->dict, ((gsp_arrayAccess*)node), index3);
	ADD_NODE(obj->dict, ((gsp_arrayAccess*)node), propertyName);
	return (PyObject*)obj;
}
PyObject *Node_parse_declareVariable(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_declareVariable*)node), varName);
	ADD_NODE(obj->dict, ((gsp_declareVariable*)node), varDatatype);
	ADD_NODE(obj->dict, ((gsp_declareVariable*)node), defaultValue);
	ADD_NODE(obj->dict, ((gsp_declareVariable*)node), tableTypeDefinitions);
	ADD_INT(obj->dict, ((gsp_declareVariable*)node), variableType);
	return (PyObject*)obj;
}
PyObject *Node_parse_declareSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_declareSqlNode*)node), declareType);
	ADD_NODE(obj->dict, ((gsp_declareSqlNode*)node), vars);
	ADD_NODE(obj->dict, ((gsp_declareSqlNode*)node), varType);
	ADD_INT(obj->dict, ((gsp_declareSqlNode*)node), howtoSetValue);
	ADD_NODE(obj->dict, ((gsp_declareSqlNode*)node), defaultValue);
	ADD_NODE(obj->dict, ((gsp_declareSqlNode*)node), conditionName);
	ADD_NODE(obj->dict, ((gsp_declareSqlNode*)node), cursorName);
	ADD_NODE(obj->dict, ((gsp_declareSqlNode*)node), subQueryNode);
	ADD_NODE(obj->dict, ((gsp_declareSqlNode*)node), stmt);
	return (PyObject*)obj;
}
PyObject *Node_parse_createTriggerUpdateColumn(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_createTriggerUpdateColumn*)node), columnName);
	return (PyObject*)obj;
}
PyObject *Node_parse_ifSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_ifSqlNode*)node), condition);
	ADD_NODE(obj->dict, ((gsp_ifSqlNode*)node), stmts);
	ADD_NODE(obj->dict, ((gsp_ifSqlNode*)node), elseStmts);
	ADD_NODE(obj->dict, ((gsp_ifSqlNode*)node), elseifList);
	ADD_NODE(obj->dict, ((gsp_ifSqlNode*)node), stmt);
	ADD_NODE(obj->dict, ((gsp_ifSqlNode*)node), elseStmt);
	ADD_NODE(obj->dict, ((gsp_ifSqlNode*)node), updateColumnList);
	return (PyObject*)obj;
}
PyObject *Node_parse_elseIfSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_elseIfSqlNode*)node), condition);
	ADD_NODE(obj->dict, ((gsp_elseIfSqlNode*)node), stmts);
	return (PyObject*)obj;
}
PyObject *Node_parse_whileSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_whileSqlNode*)node), condition);
	ADD_NODE(obj->dict, ((gsp_whileSqlNode*)node), stmt);
	ADD_NODE(obj->dict, ((gsp_whileSqlNode*)node), stmts);
	return (PyObject*)obj;
}
PyObject *Node_parse_repeatSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_repeatSqlNode*)node), condition);
	ADD_NODE(obj->dict, ((gsp_repeatSqlNode*)node), stmts);
	return (PyObject*)obj;
}
PyObject *Node_parse_loopSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_loopSqlNode*)node), stmts);
	return (PyObject*)obj;
}
PyObject *Node_parse_unknownStatement(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_createTriggerSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_createTriggerSqlNode*)node), fireMode);
	ADD_NODE(obj->dict, ((gsp_createTriggerSqlNode*)node), triggerName);
	ADD_NODE(obj->dict, ((gsp_createTriggerSqlNode*)node), tableName);
	ADD_NODE(obj->dict, ((gsp_createTriggerSqlNode*)node), whenCondition);
	ADD_NODE(obj->dict, ((gsp_createTriggerSqlNode*)node), triggerEvent);
	ADD_NODE(obj->dict, ((gsp_createTriggerSqlNode*)node), triggerAction);
	ADD_NODE(obj->dict, ((gsp_createTriggerSqlNode*)node), stmt);
	ADD_NODE(obj->dict, ((gsp_createTriggerSqlNode*)node), stmts);
	ADD_NODE(obj->dict, ((gsp_createTriggerSqlNode*)node), blockSqlNode);
	ADD_NODE(obj->dict, ((gsp_createTriggerSqlNode*)node), stmtSqlNode);
	return (PyObject*)obj;
}
PyObject *Node_parse_exceptionHandler(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_exceptionHandler*)node), exceptionNames);
	ADD_NODE(obj->dict, ((gsp_exceptionHandler*)node), stmts);
	return (PyObject*)obj;
}
PyObject *Node_parse_pivotClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_pivotClause*)node), aggregationFunction);
	ADD_NODE(obj->dict, ((gsp_pivotClause*)node), privotColumn);
	ADD_NODE(obj->dict, ((gsp_pivotClause*)node), privotColumnList);
	ADD_NODE(obj->dict, ((gsp_pivotClause*)node), inResultList);
	ADD_NODE(obj->dict, ((gsp_pivotClause*)node), aliasClause);
	return (PyObject*)obj;
}
PyObject *Node_parse_unPivotClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_unPivotClause*)node), aggregationFunction);
	ADD_NODE(obj->dict, ((gsp_unPivotClause*)node), valueColumn);
	ADD_NODE(obj->dict, ((gsp_unPivotClause*)node), privotColumn);
	ADD_NODE(obj->dict, ((gsp_unPivotClause*)node), privotColumnList);
	ADD_NODE(obj->dict, ((gsp_unPivotClause*)node), inResultList);
	ADD_NODE(obj->dict, ((gsp_unPivotClause*)node), aliasClause);
	return (PyObject*)obj;
}
PyObject *Node_parse_renameColumnSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_renameColumnSqlNode*)node), columnName);
	ADD_NODE(obj->dict, ((gsp_renameColumnSqlNode*)node), newColumnName);
	return (PyObject*)obj;
}
PyObject *Node_parse_renameSequenceSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_renameSequenceSqlNode*)node), sequenceName);
	ADD_NODE(obj->dict, ((gsp_renameSequenceSqlNode*)node), newSequenceName);
	return (PyObject*)obj;
}
PyObject *Node_parse_renameTableSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_renameTableSqlNode*)node), tableName);
	ADD_NODE(obj->dict, ((gsp_renameTableSqlNode*)node), newTableName);
	return (PyObject*)obj;
}
PyObject *Node_parse_renameIndexSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_renameIndexSqlNode*)node), indexName);
	ADD_NODE(obj->dict, ((gsp_renameIndexSqlNode*)node), newIndexName);
	return (PyObject*)obj;
}
PyObject *Node_parse_dropSequenceSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_dropSequenceSqlNode*)node), sequenceName);
	return (PyObject*)obj;
}
PyObject *Node_parse_dropSynonymSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_dropSynonymSqlNode*)node), synonymName);
	return (PyObject*)obj;
}
PyObject *Node_parse_dropRowTypeSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_dropRowTypeSqlNode*)node), rowTypeName);
	return (PyObject*)obj;
}
PyObject *Node_parse_alterIndexSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_alterIndexSqlNode*)node), indexName);
	return (PyObject*)obj;
}
PyObject *Node_parse_alterSequenceSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_alterSequenceSqlNode*)node), sequenceName);
	return (PyObject*)obj;
}
PyObject *Node_parse_alterViewSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_alterViewSqlNode*)node), viewName);
	return (PyObject*)obj;
}
PyObject *Node_parse_intoTableClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_intoTableClause*)node), tableName);
	return (PyObject*)obj;
}
PyObject *Node_parse_informixOuterClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_informixOuterClause*)node), fromTable);
	ADD_NODE(obj->dict, ((gsp_informixOuterClause*)node), fromTableList);
	return (PyObject*)obj;
}
PyObject *Node_parse_createRowTypeSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_createRowTypeSqlNode*)node), rowTypeName);
	ADD_NODE(obj->dict, ((gsp_createRowTypeSqlNode*)node), superTableName);
	ADD_NODE(obj->dict, ((gsp_createRowTypeSqlNode*)node), columnDefList);
	return (PyObject*)obj;
}
PyObject *Node_parse_subscripts(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_subscripts*)node), first);
	ADD_NODE(obj->dict, ((gsp_subscripts*)node), last);
	return (PyObject*)obj;
}
PyObject *Node_parse_limitClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_limitClause*)node), offset);
	ADD_NODE(obj->dict, ((gsp_limitClause*)node), rowCount);
	ADD_NODE(obj->dict, ((gsp_limitClause*)node), limitValue);
	return (PyObject*)obj;
}
PyObject *Node_parse_callSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_callSqlNode*)node), functionName);
	ADD_NODE(obj->dict, ((gsp_callSqlNode*)node), args);
	return (PyObject*)obj;
}
PyObject *Node_parse_createDatabaseSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_createDatabaseSqlNode*)node), databaseName);
	return (PyObject*)obj;
}
PyObject *Node_parse_lockingClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_lockingClause*)node), lockedObjects);
	return (PyObject*)obj;
}
PyObject *Node_parse_windowClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_windowClause*)node), windownsDefs);
	return (PyObject*)obj;
}
PyObject *Node_parse_partitionClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_partitionClause*)node), exprList);
	return (PyObject*)obj;
}
PyObject *Node_parse_windowDef(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_windowDef*)node), referenceName);
	ADD_NODE(obj->dict, ((gsp_windowDef*)node), partitionClause);
	ADD_NODE(obj->dict, ((gsp_windowDef*)node), sortClause);
	ADD_NODE(obj->dict, ((gsp_windowDef*)node), frameClause);
	ADD_NODE(obj->dict, ((gsp_windowDef*)node), startOffset);
	ADD_NODE(obj->dict, ((gsp_windowDef*)node), endOffset);
	ADD_NODE(obj->dict, ((gsp_windowDef*)node), windowName);
	return (PyObject*)obj;
}
PyObject *Node_parse_indices(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_indices*)node), attributeName);
	ADD_NODE(obj->dict, ((gsp_indices*)node), lowerSubscript);
	ADD_NODE(obj->dict, ((gsp_indices*)node), upperSubscript);
	return (PyObject*)obj;
}
PyObject *Node_parse_collectStatisticsSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_collectStatisticsSqlNode*)node), tableName);
	ADD_NODE(obj->dict, ((gsp_collectStatisticsSqlNode*)node), indexName);
	ADD_NODE(obj->dict, ((gsp_collectStatisticsSqlNode*)node), columnName);
	ADD_NODE(obj->dict, ((gsp_collectStatisticsSqlNode*)node), columnList);
	return (PyObject*)obj;
}
PyObject *Node_parse_teradataWithClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_teradataWithClause*)node), exprList);
	ADD_NODE(obj->dict, ((gsp_teradataWithClause*)node), byList);
	return (PyObject*)obj;
}
PyObject *Node_parse_qualifyClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_qualifyClause*)node), expr);
	return (PyObject*)obj;
}
PyObject *Node_parse_sampleClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_sampleClause*)node), countFractionDescriptionList);
	ADD_NODE(obj->dict, ((gsp_sampleClause*)node), whenThenList);
	return (PyObject*)obj;
}
PyObject *Node_parse_expandOnClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_expandOnClause*)node), expr);
	ADD_NODE(obj->dict, ((gsp_expandOnClause*)node), columnAlias);
	ADD_NODE(obj->dict, ((gsp_expandOnClause*)node), periodExpr);
	return (PyObject*)obj;
}
PyObject *Node_parse_datatypeAttribute(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_newVariantTypeArgument(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_newVariantTypeArgument*)node), expr);
	ADD_NODE(obj->dict, ((gsp_newVariantTypeArgument*)node), aliasName);
	return (PyObject*)obj;
}
PyObject *Node_parse_outputFormatPhrase(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_outputFormatPhrase*)node), dataTypeName);
	ADD_NODE(obj->dict, ((gsp_outputFormatPhrase*)node), datatypeAttribute);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlCreateTypeBody(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_plsqlCreateTypeBody*)node), typeName);
	ADD_NODE(obj->dict, ((gsp_plsqlCreateTypeBody*)node), stmts);
	return (PyObject*)obj;
}
PyObject *Node_parse_typeAttribute(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_typeAttribute*)node), attributeName);
	ADD_NODE(obj->dict, ((gsp_typeAttribute*)node), datatype);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlCreateType(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_plsqlCreateType*)node), typeName);
	ADD_NODE(obj->dict, ((gsp_plsqlCreateType*)node), attributes);
	return (PyObject*)obj;
}
PyObject *Node_parse_dmlEventClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_dmlEventClause*)node), tableName);
	ADD_NODE(obj->dict, ((gsp_dmlEventClause*)node), dml_event_items);
	return (PyObject*)obj;
}
PyObject *Node_parse_nonDmlTriggerClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_nonDmlTriggerClause*)node), schemaName);
	ADD_INT(obj->dict, ((gsp_nonDmlTriggerClause*)node), fireMode);
	ADD_INT(obj->dict, ((gsp_nonDmlTriggerClause*)node), isSchema);
	ADD_INT(obj->dict, ((gsp_nonDmlTriggerClause*)node), isDatabase);
	ADD_NODE(obj->dict, ((gsp_nonDmlTriggerClause*)node), ddl_event_list);
	ADD_NODE(obj->dict, ((gsp_nonDmlTriggerClause*)node), database_event_list);
	return (PyObject*)obj;
}
PyObject *Node_parse_compoundDmlTriggerClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_compoundDmlTriggerClause*)node), dmlEventClause);
	return (PyObject*)obj;
}
PyObject *Node_parse_simpleDmlTriggerClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_simpleDmlTriggerClause*)node), fireMode);
	ADD_NODE(obj->dict, ((gsp_simpleDmlTriggerClause*)node), dmlEventClause);
	return (PyObject*)obj;
}
PyObject *Node_parse_createPackageSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_createPackageSqlNode*)node), packageName);
	ADD_INT(obj->dict, ((gsp_createPackageSqlNode*)node), kind);
	ADD_NODE(obj->dict, ((gsp_createPackageSqlNode*)node), definitions_or_declarations);
	ADD_NODE(obj->dict, ((gsp_createPackageSqlNode*)node), stmts);
	ADD_NODE(obj->dict, ((gsp_createPackageSqlNode*)node), exceptionClause);
	ADD_NODE(obj->dict, ((gsp_createPackageSqlNode*)node), declareStatements);
	ADD_NODE(obj->dict, ((gsp_createPackageSqlNode*)node), bodyStatements);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlVarDeclStmt(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_plsqlVarDeclStmt*)node), whatDeclared);
	ADD_NODE(obj->dict, ((gsp_plsqlVarDeclStmt*)node), exception_name);
	ADD_NODE(obj->dict, ((gsp_plsqlVarDeclStmt*)node), error_number);
	ADD_NODE(obj->dict, ((gsp_plsqlVarDeclStmt*)node), elementName);
	ADD_NODE(obj->dict, ((gsp_plsqlVarDeclStmt*)node), dataType);
	ADD_NODE(obj->dict, ((gsp_plsqlVarDeclStmt*)node), value);
	ADD_INT(obj->dict, ((gsp_plsqlVarDeclStmt*)node), isNotNull);
	ADD_INT(obj->dict, ((gsp_plsqlVarDeclStmt*)node), howtoSetValue);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlRaiseStmt(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_plsqlRaiseStmt*)node), exceptionName);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlLoopStmt(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_plsqlLoopStmt*)node), stmts);
	ADD_NODE(obj->dict, ((gsp_plsqlLoopStmt*)node), condition);
	ADD_NODE(obj->dict, ((gsp_plsqlLoopStmt*)node), indexName);
	ADD_NODE(obj->dict, ((gsp_plsqlLoopStmt*)node), cursorName);
	ADD_NODE(obj->dict, ((gsp_plsqlLoopStmt*)node), lower_bound);
	ADD_NODE(obj->dict, ((gsp_plsqlLoopStmt*)node), upper_bound);
	ADD_NODE(obj->dict, ((gsp_plsqlLoopStmt*)node), cursorParameterNames);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlCaseStmt(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_plsqlCaseStmt*)node), caseExpr);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlForallStmt(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_plsqlForallStmt*)node), indexName);
	ADD_NODE(obj->dict, ((gsp_plsqlForallStmt*)node), stmt);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlElsifStmt(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_plsqlElsifStmt*)node), condition);
	ADD_NODE(obj->dict, ((gsp_plsqlElsifStmt*)node), thenStmts);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlIfStmt(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_plsqlIfStmt*)node), condition);
	ADD_NODE(obj->dict, ((gsp_plsqlIfStmt*)node), thenStmts);
	ADD_NODE(obj->dict, ((gsp_plsqlIfStmt*)node), elseStmts);
	ADD_NODE(obj->dict, ((gsp_plsqlIfStmt*)node), elsifStmts);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlGotoStmt(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_plsqlGotoStmt*)node), gotolabelName);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlExitStmt(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_plsqlExitStmt*)node), whenCondition);
	ADD_NODE(obj->dict, ((gsp_plsqlExitStmt*)node), exitlabelName);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlAssignStmt(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_plsqlAssignStmt*)node), left);
	ADD_NODE(obj->dict, ((gsp_plsqlAssignStmt*)node), right);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlCursorDeclStmt(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_plsqlCursorDeclStmt*)node), subqueryNode);
	ADD_NODE(obj->dict, ((gsp_plsqlCursorDeclStmt*)node), cursorName);
	ADD_NODE(obj->dict, ((gsp_plsqlCursorDeclStmt*)node), cursorParameterDeclarations);
	ADD_NODE(obj->dict, ((gsp_plsqlCursorDeclStmt*)node), rowtype);
	ADD_NODE(obj->dict, ((gsp_plsqlCursorDeclStmt*)node), cursorTypeName);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlRecordTypeDefStmt(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_plsqlRecordTypeDefStmt*)node), typeName);
	ADD_NODE(obj->dict, ((gsp_plsqlRecordTypeDefStmt*)node), fieldDeclarations);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlVarrayTypeDefStmt(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_plsqlVarrayTypeDefStmt*)node), typeName);
	ADD_NODE(obj->dict, ((gsp_plsqlVarrayTypeDefStmt*)node), elementDataType);
	ADD_NODE(obj->dict, ((gsp_plsqlVarrayTypeDefStmt*)node), sizeLimit);
	ADD_INT(obj->dict, ((gsp_plsqlVarrayTypeDefStmt*)node), isNotNull);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlTableTypeDefStmt(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_plsqlTableTypeDefStmt*)node), typeName);
	ADD_NODE(obj->dict, ((gsp_plsqlTableTypeDefStmt*)node), elementDataType);
	ADD_INT(obj->dict, ((gsp_plsqlTableTypeDefStmt*)node), isNotNull);
	ADD_NODE(obj->dict, ((gsp_plsqlTableTypeDefStmt*)node), indexByDataType);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlNullStmt(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlPipeRowStmt(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_bindArgument(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_bindArgument*)node), mode);
	ADD_NODE(obj->dict, ((gsp_bindArgument*)node), bindArgumentExpr);
	return (PyObject*)obj;
}
PyObject *Node_parse_execImmeNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_execImmeNode*)node), dynamicStringExpr);
	ADD_NODE(obj->dict, ((gsp_execImmeNode*)node), intoVariables);
	ADD_NODE(obj->dict, ((gsp_execImmeNode*)node), bindArguments);
	ADD_NODE(obj->dict, ((gsp_execImmeNode*)node), returnNames);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlOpenforStmt(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_plsqlOpenforStmt*)node), cursorVariableName);
	ADD_NODE(obj->dict, ((gsp_plsqlOpenforStmt*)node), subqueryNode);
	ADD_NODE(obj->dict, ((gsp_plsqlOpenforStmt*)node), dynamic_string);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlOpenStmt(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_plsqlOpenStmt*)node), cursorName);
	ADD_NODE(obj->dict, ((gsp_plsqlOpenStmt*)node), cursorParameterNames);
	return (PyObject*)obj;
}
PyObject *Node_parse_plsqlBasicStmt(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_plsqlBasicStmt*)node), expr);
	return (PyObject*)obj;
}
PyObject *Node_parse_trigger_event(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_executeAsSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_executeSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_mssql_executeSqlNode*)node), execType);
	ADD_NODE(obj->dict, ((gsp_mssql_executeSqlNode*)node), moduleName);
	ADD_NODE(obj->dict, ((gsp_mssql_executeSqlNode*)node), returnStatus);
	ADD_NODE(obj->dict, ((gsp_mssql_executeSqlNode*)node), parameterList);
	ADD_NODE(obj->dict, ((gsp_mssql_executeSqlNode*)node), stringValues);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_execParameter(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_mssql_execParameter*)node), parameterMode);
	ADD_NODE(obj->dict, ((gsp_mssql_execParameter*)node), parameterName);
	ADD_NODE(obj->dict, ((gsp_mssql_execParameter*)node), parameterValue);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_dropDbObjectSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_mssql_dropDbObjectSqlNode*)node), dbObjectType);
	ADD_NODE(obj->dict, ((gsp_mssql_dropDbObjectSqlNode*)node), objectNames);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_setSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_INT(obj->dict, ((gsp_mssql_setSqlNode*)node), setType);
	ADD_NODE(obj->dict, ((gsp_mssql_setSqlNode*)node), varName);
	ADD_NODE(obj->dict, ((gsp_mssql_setSqlNode*)node), varExpr);
	ADD_NODE(obj->dict, ((gsp_mssql_setSqlNode*)node), selectSqlNode);
	ADD_NODE(obj->dict, ((gsp_mssql_setSqlNode*)node), exprList);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_beginTranSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_beginTranSqlNode*)node), transactionName);
	ADD_INT(obj->dict, ((gsp_mssql_beginTranSqlNode*)node), distributed);
	ADD_INT(obj->dict, ((gsp_mssql_beginTranSqlNode*)node), withMark);
	ADD_NODE(obj->dict, ((gsp_mssql_beginTranSqlNode*)node), withMarkDescription);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_raiserrorSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_raiserrorSqlNode*)node), msgs);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_gotoSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_gotoSqlNode*)node), labelName);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_labelSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_deallocateSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_deallocateSqlNode*)node), cursorName);
	ADD_INT(obj->dict, ((gsp_mssql_deallocateSqlNode*)node), bGlobal);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_beginDialogSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_beginDialogSqlNode*)node), dialogHandle);
	ADD_NODE(obj->dict, ((gsp_mssql_beginDialogSqlNode*)node), initiatorServiceName);
	ADD_NODE(obj->dict, ((gsp_mssql_beginDialogSqlNode*)node), targetServiceName);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_sendOnConversationSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_sendOnConversationSqlNode*)node), conversationHandle);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_endConversationSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_endConversationSqlNode*)node), conversationHandle);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_revertSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_goSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_useSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_useSqlNode*)node), databaseName);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_printSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_printSqlNode*)node), vars);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_computeClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_computeClause*)node), items);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_computeClauseItem(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_computeClauseItem*)node), computeExprList);
	ADD_NODE(obj->dict, ((gsp_mssql_computeClauseItem*)node), exprList);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_computeExpr(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_computeExpr*)node), expr);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_containsTable(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_containsTable*)node), tableName);
	ADD_NODE(obj->dict, ((gsp_mssql_containsTable*)node), containExpr);
	ADD_NODE(obj->dict, ((gsp_mssql_containsTable*)node), searchCondition);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_freeTable(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_freeTable*)node), tableName);
	ADD_NODE(obj->dict, ((gsp_mssql_freeTable*)node), containExpr);
	ADD_NODE(obj->dict, ((gsp_mssql_freeTable*)node), searchCondition);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_openXML(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_openRowSet(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_openQuery(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_openQuery*)node), exprList);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_openDatasource(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_openDatasource*)node), providerName);
	ADD_NODE(obj->dict, ((gsp_mssql_openDatasource*)node), initString);
	ADD_NODE(obj->dict, ((gsp_mssql_openDatasource*)node), tableName);
	ADD_NODE(obj->dict, ((gsp_mssql_openDatasource*)node), exprList);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_tableHint(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_tableHint*)node), hintName);
	ADD_NODE(obj->dict, ((gsp_mssql_tableHint*)node), hintNameList);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_bulkInsertSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_bulkInsertSqlNode*)node), tableName);
	ADD_NODE(obj->dict, ((gsp_mssql_bulkInsertSqlNode*)node), datafile);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_outputClause(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_outputClause*)node), selectItemList);
	ADD_NODE(obj->dict, ((gsp_mssql_outputClause*)node), selectItemList2);
	ADD_NODE(obj->dict, ((gsp_mssql_outputClause*)node), intoColumnList);
	ADD_NODE(obj->dict, ((gsp_mssql_outputClause*)node), tableName);
	return (PyObject*)obj;
}
PyObject *Node_parse_mssql_updateTextSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_mssql_updateTextSqlNode*)node), destColumnName);
	ADD_NODE(obj->dict, ((gsp_mssql_updateTextSqlNode*)node), destTextPtr);
	return (PyObject*)obj;
}
PyObject *Node_parse_db2_signal(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_db2_compoundSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_db2_compoundSqlNode*)node), declareStmts);
	ADD_NODE(obj->dict, ((gsp_db2_compoundSqlNode*)node), bodyStmts);
	return (PyObject*)obj;
}
PyObject *Node_parse_db2_triggerAction(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_db2_triggerAction*)node), whenCondition);
	ADD_NODE(obj->dict, ((gsp_db2_triggerAction*)node), compoundSqlNode);
	ADD_NODE(obj->dict, ((gsp_db2_triggerAction*)node), stmt);
	return (PyObject*)obj;
}
PyObject *Node_parse_db2_callStmtSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_db2_callStmtSqlNode*)node), procedureName);
	ADD_NODE(obj->dict, ((gsp_db2_callStmtSqlNode*)node), args);
	return (PyObject*)obj;
}
PyObject *Node_parse_db2_forSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_db2_forSqlNode*)node), loopName);
	ADD_NODE(obj->dict, ((gsp_db2_forSqlNode*)node), cursorName);
	ADD_NODE(obj->dict, ((gsp_db2_forSqlNode*)node), subQueryNode);
	ADD_NODE(obj->dict, ((gsp_db2_forSqlNode*)node), stmts);
	return (PyObject*)obj;
}
PyObject *Node_parse_db2_iterateStmtSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_db2_iterateStmtSqlNode*)node), labelName);
	return (PyObject*)obj;
}
PyObject *Node_parse_db2_leaveStmtSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_db2_leaveStmtSqlNode*)node), labelName);
	return (PyObject*)obj;
}
PyObject *Node_parse_db2_setSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	return (PyObject*)obj;
}
PyObject *Node_parse_db2_whileSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_db2_whileSqlNode*)node), condition);
	ADD_NODE(obj->dict, ((gsp_db2_whileSqlNode*)node), stmts);
	return (PyObject*)obj;
}
PyObject *Node_parse_db2_repeatSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_db2_repeatSqlNode*)node), condition);
	ADD_NODE(obj->dict, ((gsp_db2_repeatSqlNode*)node), stmts);
	return (PyObject*)obj;
}
PyObject *Node_parse_db2_gotoSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_db2_gotoSqlNode*)node), labelName);
	return (PyObject*)obj;
}
PyObject *Node_parse_db2_loopSqlNode(gsp_node *node, Statement *stmt) {
	SqlNode *obj = (SqlNode*)Node_new(&NodeType, Py_None, Py_None); obj->_node = node;	ADD_NODE(obj->dict, ((gsp_db2_loopSqlNode*)node), stmts);
	return (PyObject*)obj;
}
///

void Node_init() {
	int i;
	for (i=0; i<MAX_NODE_PARSE_FUNCS;i++) Node_parse_functions[i] = NULL;
	
	Node_parse_functions[t_gsp_list] = Node_parse_list;
	Node_parse_functions[t_gsp_createTableStatement] = Node_parse_createTableSqlNode;
	Node_parse_functions[t_gsp_insertStatement] = Node_parse_insertSqlNode;
	Node_parse_functions[t_gsp_updateStatement] = Node_parse_updateSqlNode;
	Node_parse_functions[t_gsp_deleteStatement] = Node_parse_deleteSqlNode;
	
	Node_parse_functions[t_gsp_listcell] = Node_parse_listcell;
	Node_parse_functions[t_gsp_sql_statement] = Node_parse_sql_statement;
	Node_parse_functions[t_gsp_dummy] = Node_parse_dummy;
	Node_parse_functions[t_gsp_constant] = Node_parse_constant;
	Node_parse_functions[t_gsp_objectname] = Node_parse_objectname;
	Node_parse_functions[t_gsp_expr] = Node_parse_expr;
	Node_parse_functions[t_gsp_objectAccess] = Node_parse_objectAccess;
	Node_parse_functions[t_gsp_aliasClause] = Node_parse_aliasClause;
	Node_parse_functions[t_gsp_resultColumn] = Node_parse_resultColumn;
	Node_parse_functions[t_gsp_trimArgument] = Node_parse_trimArgument;
	Node_parse_functions[t_gsp_orderByItem] = Node_parse_orderByItem;
	Node_parse_functions[t_gsp_orderBy] = Node_parse_orderBy;
	Node_parse_functions[t_gsp_keepDenseRankClause] = Node_parse_keepDenseRankClause;
	Node_parse_functions[t_gsp_analyticFunction] = Node_parse_analyticFunction;
	Node_parse_functions[t_gsp_functionCall] = Node_parse_functionCall;
	Node_parse_functions[t_gsp_whenClauseItem] = Node_parse_whenClauseItem;
	Node_parse_functions[t_gsp_caseExpression] = Node_parse_caseExpression;
	Node_parse_functions[t_gsp_intervalExpression] = Node_parse_intervalExpression;
	Node_parse_functions[t_gsp_precisionScale] = Node_parse_precisionScale;
	Node_parse_functions[t_gsp_typename] = Node_parse_typename;
	Node_parse_functions[t_gsp_keyReference] = Node_parse_keyReference;
	Node_parse_functions[t_gsp_keyAction] = Node_parse_keyAction;
	Node_parse_functions[t_gsp_constraint] = Node_parse_constraint;
	Node_parse_functions[t_gsp_mergeInsertClause] = Node_parse_mergeInsertClause;
	Node_parse_functions[t_gsp_mergeUpdateClause] = Node_parse_mergeUpdateClause;
	Node_parse_functions[t_gsp_mergeDeleteClause] = Node_parse_mergeDeleteClause;
	Node_parse_functions[t_gsp_mergeWhenClause] = Node_parse_mergeWhenClause;
	Node_parse_functions[t_gsp_dataChangeTable] = Node_parse_dataChangeTable;
	Node_parse_functions[t_gsp_fromTable] = Node_parse_fromTable;
	Node_parse_functions[t_gsp_table] = Node_parse_table;
	Node_parse_functions[t_gsp_mergeSqlNode] = Node_parse_mergeSqlNode;
	Node_parse_functions[t_gsp_alterTableOption] = Node_parse_alterTableOption;
	Node_parse_functions[t_gsp_alterTableSqlNode] = Node_parse_alterTableSqlNode;
	Node_parse_functions[t_gsp_createSequenceSqlNode] = Node_parse_createSequenceSqlNode;
	Node_parse_functions[t_gsp_createSynonymSqlNode] = Node_parse_createSynonymSqlNode;
	Node_parse_functions[t_gsp_createDirectorySqlNode] = Node_parse_createDirectorySqlNode;
	Node_parse_functions[t_gsp_dropViewSqlNode] = Node_parse_dropViewSqlNode;
	Node_parse_functions[t_gsp_dropTableSqlNode] = Node_parse_dropTableSqlNode;
	Node_parse_functions[t_gsp_dropIndexItem] = Node_parse_dropIndexItem;
	Node_parse_functions[t_gsp_dropIndexSqlNode] = Node_parse_dropIndexSqlNode;
	Node_parse_functions[t_gsp_truncateTableSqlNode] = Node_parse_truncateTableSqlNode;
	Node_parse_functions[t_gsp_viewAliasItem] = Node_parse_viewAliasItem;
	Node_parse_functions[t_gsp_viewAliasClause] = Node_parse_viewAliasClause;
	Node_parse_functions[t_gsp_createViewSqlNode] = Node_parse_createViewSqlNode;
	Node_parse_functions[t_gsp_createMaterializedViewSqlNode] = Node_parse_createMaterializedViewSqlNode;
	Node_parse_functions[t_gsp_createMaterializedViewLogSqlNode] = Node_parse_createMaterializedViewLogSqlNode;
	Node_parse_functions[t_gsp_createIndexSqlNode] = Node_parse_createIndexSqlNode;
	Node_parse_functions[t_gsp_commitSqlNode] = Node_parse_commitSqlNode;
	Node_parse_functions[t_gsp_rollbackSqlNode] = Node_parse_rollbackSqlNode;
	Node_parse_functions[t_gsp_saveTransSqlNode] = Node_parse_saveTransSqlNode;
	Node_parse_functions[t_gsp_columnDefinition] = Node_parse_columnDefinition;
	Node_parse_functions[t_gsp_tableElement] = Node_parse_tableElement;
	Node_parse_functions[t_gsp_createTableSqlNode] = Node_parse_createTableSqlNode;
	Node_parse_functions[t_gsp_returningClause] = Node_parse_returningClause;
	Node_parse_functions[t_gsp_isolationClause] = Node_parse_isolationClause;
	Node_parse_functions[t_gsp_includeColumns] = Node_parse_includeColumns;
	Node_parse_functions[t_gsp_deleteSqlNode] = Node_parse_deleteSqlNode;
	Node_parse_functions[t_gsp_updateSqlNode] = Node_parse_updateSqlNode;
	Node_parse_functions[t_gsp_multiTarget] = Node_parse_multiTarget;
	Node_parse_functions[t_gsp_insertRest] = Node_parse_insertRest;
	Node_parse_functions[t_gsp_insertValuesClause] = Node_parse_insertValuesClause;
	Node_parse_functions[t_gsp_insertIntoValue] = Node_parse_insertIntoValue;
	Node_parse_functions[t_gsp_insertCondition] = Node_parse_insertCondition;
	Node_parse_functions[t_gsp_insertSqlNode] = Node_parse_insertSqlNode;
	Node_parse_functions[t_gsp_whereClause] = Node_parse_whereClause;
	Node_parse_functions[t_gsp_joinExpr] = Node_parse_joinExpr;
	Node_parse_functions[t_gsp_tableSamplePart] = Node_parse_tableSamplePart;
	Node_parse_functions[t_gsp_tableSample] = Node_parse_tableSample;
	Node_parse_functions[t_gsp_pxGranule] = Node_parse_pxGranule;
	Node_parse_functions[t_gsp_flashback] = Node_parse_flashback;
	Node_parse_functions[t_gsp_forUpdate] = Node_parse_forUpdate;
	Node_parse_functions[t_gsp_groupingSetItem] = Node_parse_groupingSetItem;
	Node_parse_functions[t_gsp_groupingSet] = Node_parse_groupingSet;
	Node_parse_functions[t_gsp_rollupCube] = Node_parse_rollupCube;
	Node_parse_functions[t_gsp_gruopByItem] = Node_parse_gruopByItem;
	Node_parse_functions[t_gsp_groupBy] = Node_parse_groupBy;
	Node_parse_functions[t_gsp_selectDistinct] = Node_parse_selectDistinct;
	Node_parse_functions[t_gsp_topClause] = Node_parse_topClause;
	Node_parse_functions[t_gsp_hierarchical] = Node_parse_hierarchical;
	Node_parse_functions[t_gsp_intoClause] = Node_parse_intoClause;
	Node_parse_functions[t_gsp_valueClause] = Node_parse_valueClause;
	Node_parse_functions[t_gsp_fetchFirstClause] = Node_parse_fetchFirstClause;
	Node_parse_functions[t_gsp_optimizeForClause] = Node_parse_optimizeForClause;
	Node_parse_functions[t_gsp_valueRowItem] = Node_parse_valueRowItem;
	Node_parse_functions[t_gsp_selectSqlNode] = Node_parse_selectSqlNode;
	Node_parse_functions[t_gsp_selectStatement] = Node_parse_selectSqlNode;
	Node_parse_functions[t_gsp_cte] = Node_parse_cte;
	Node_parse_functions[t_gsp_commentSqlNode] = Node_parse_commentSqlNode;
	Node_parse_functions[t_gsp_callSpec] = Node_parse_callSpec;
	Node_parse_functions[t_gsp_returnSqlNode] = Node_parse_returnSqlNode;
	Node_parse_functions[t_gsp_continueSqlNode] = Node_parse_continueSqlNode;
	Node_parse_functions[t_gsp_breakSqlNode] = Node_parse_breakSqlNode;
	Node_parse_functions[t_gsp_grantSqlNode] = Node_parse_grantSqlNode;
	Node_parse_functions[t_gsp_executeSqlNode] = Node_parse_executeSqlNode;
	Node_parse_functions[t_gsp_dropRoleSqlNode] = Node_parse_dropRoleSqlNode;
	Node_parse_functions[t_gsp_dropTriggerSqlNode] = Node_parse_dropTriggerSqlNode;
	Node_parse_functions[t_gsp_lockTableSqlNode] = Node_parse_lockTableSqlNode;
	Node_parse_functions[t_gsp_revokeSqlNode] = Node_parse_revokeSqlNode;
	Node_parse_functions[t_gsp_fetchSqlNode] = Node_parse_fetchSqlNode;
	Node_parse_functions[t_gsp_openSqlNode] = Node_parse_openSqlNode;
	Node_parse_functions[t_gsp_closeSqlNode] = Node_parse_closeSqlNode;
	Node_parse_functions[t_gsp_iterateSqlNode] = Node_parse_iterateSqlNode;
	Node_parse_functions[t_gsp_leaveSqlNode] = Node_parse_leaveSqlNode;
	Node_parse_functions[t_gsp_createFunctionSqlNode] = Node_parse_createFunctionSqlNode;
	Node_parse_functions[t_gsp_parameterDeclaration] = Node_parse_parameterDeclaration;
	Node_parse_functions[t_gsp_createProcedureSqlNode] = Node_parse_createProcedureSqlNode;
	Node_parse_functions[t_gsp_exceptionClause] = Node_parse_exceptionClause;
	Node_parse_functions[t_gsp_blockSqlNode] = Node_parse_blockSqlNode;
	Node_parse_functions[t_gsp_arrayAccess] = Node_parse_arrayAccess;
	Node_parse_functions[t_gsp_declareVariable] = Node_parse_declareVariable;
	Node_parse_functions[t_gsp_declareSqlNode] = Node_parse_declareSqlNode;
	Node_parse_functions[t_gsp_createTriggerUpdateColumn] = Node_parse_createTriggerUpdateColumn;
	Node_parse_functions[t_gsp_ifSqlNode] = Node_parse_ifSqlNode;
	Node_parse_functions[t_gsp_elseIfSqlNode] = Node_parse_elseIfSqlNode;
	Node_parse_functions[t_gsp_whileSqlNode] = Node_parse_whileSqlNode;
	Node_parse_functions[t_gsp_repeatSqlNode] = Node_parse_repeatSqlNode;
	Node_parse_functions[t_gsp_loopSqlNode] = Node_parse_loopSqlNode;
	Node_parse_functions[t_gsp_unknownStatement] = Node_parse_unknownStatement;
	Node_parse_functions[t_gsp_createTriggerSqlNode] = Node_parse_createTriggerSqlNode;
	Node_parse_functions[t_gsp_exceptionHandler] = Node_parse_exceptionHandler;
	Node_parse_functions[t_gsp_pivotClause] = Node_parse_pivotClause;
	Node_parse_functions[t_gsp_unPivotClause] = Node_parse_unPivotClause;
	Node_parse_functions[t_gsp_renameColumnSqlNode] = Node_parse_renameColumnSqlNode;
	Node_parse_functions[t_gsp_renameSequenceSqlNode] = Node_parse_renameSequenceSqlNode;
	Node_parse_functions[t_gsp_renameTableSqlNode] = Node_parse_renameTableSqlNode;
	Node_parse_functions[t_gsp_renameIndexSqlNode] = Node_parse_renameIndexSqlNode;
	Node_parse_functions[t_gsp_dropSequenceSqlNode] = Node_parse_dropSequenceSqlNode;
	Node_parse_functions[t_gsp_dropSynonymSqlNode] = Node_parse_dropSynonymSqlNode;
	Node_parse_functions[t_gsp_dropRowTypeSqlNode] = Node_parse_dropRowTypeSqlNode;
	Node_parse_functions[t_gsp_alterIndexSqlNode] = Node_parse_alterIndexSqlNode;
	Node_parse_functions[t_gsp_alterSequenceSqlNode] = Node_parse_alterSequenceSqlNode;
	Node_parse_functions[t_gsp_alterViewSqlNode] = Node_parse_alterViewSqlNode;
	Node_parse_functions[t_gsp_intoTableClause] = Node_parse_intoTableClause;
	Node_parse_functions[t_gsp_informixOuterClause] = Node_parse_informixOuterClause;
	Node_parse_functions[t_gsp_createRowTypeSqlNode] = Node_parse_createRowTypeSqlNode;
	Node_parse_functions[t_gsp_subscripts] = Node_parse_subscripts;
	Node_parse_functions[t_gsp_limitClause] = Node_parse_limitClause;
	Node_parse_functions[t_gsp_callSqlNode] = Node_parse_callSqlNode;
	Node_parse_functions[t_gsp_createDatabaseSqlNode] = Node_parse_createDatabaseSqlNode;
	Node_parse_functions[t_gsp_lockingClause] = Node_parse_lockingClause;
	Node_parse_functions[t_gsp_windowClause] = Node_parse_windowClause;
	Node_parse_functions[t_gsp_partitionClause] = Node_parse_partitionClause;
	Node_parse_functions[t_gsp_windowDef] = Node_parse_windowDef;
	Node_parse_functions[t_gsp_indices] = Node_parse_indices;
	Node_parse_functions[t_gsp_collectStatisticsSqlNode] = Node_parse_collectStatisticsSqlNode;
	Node_parse_functions[t_gsp_teradataWithClause] = Node_parse_teradataWithClause;
	Node_parse_functions[t_gsp_qualifyClause] = Node_parse_qualifyClause;
	Node_parse_functions[t_gsp_sampleClause] = Node_parse_sampleClause;
	Node_parse_functions[t_gsp_expandOnClause] = Node_parse_expandOnClause;
	Node_parse_functions[t_gsp_datatypeAttribute] = Node_parse_datatypeAttribute;
	Node_parse_functions[t_gsp_newVariantTypeArgument] = Node_parse_newVariantTypeArgument;
	Node_parse_functions[t_gsp_outputFormatPhrase] = Node_parse_outputFormatPhrase;
	Node_parse_functions[t_gsp_plsqlCreateTypeBody] = Node_parse_plsqlCreateTypeBody;
	Node_parse_functions[t_gsp_typeAttribute] = Node_parse_typeAttribute;
	Node_parse_functions[t_gsp_plsqlCreateType] = Node_parse_plsqlCreateType;
	Node_parse_functions[t_gsp_dmlEventClause] = Node_parse_dmlEventClause;
	Node_parse_functions[t_gsp_nonDmlTriggerClause] = Node_parse_nonDmlTriggerClause;
	Node_parse_functions[t_gsp_compoundDmlTriggerClause] = Node_parse_compoundDmlTriggerClause;
	Node_parse_functions[t_gsp_simpleDmlTriggerClause] = Node_parse_simpleDmlTriggerClause;
	Node_parse_functions[t_gsp_createPackageSqlNode] = Node_parse_createPackageSqlNode;
	Node_parse_functions[t_gsp_plsqlVarDeclStmt] = Node_parse_plsqlVarDeclStmt;
	Node_parse_functions[t_gsp_plsqlRaiseStmt] = Node_parse_plsqlRaiseStmt;
	Node_parse_functions[t_gsp_plsqlLoopStmt] = Node_parse_plsqlLoopStmt;
	Node_parse_functions[t_gsp_plsqlCaseStmt] = Node_parse_plsqlCaseStmt;
	Node_parse_functions[t_gsp_plsqlForallStmt] = Node_parse_plsqlForallStmt;
	Node_parse_functions[t_gsp_plsqlElsifStmt] = Node_parse_plsqlElsifStmt;
	Node_parse_functions[t_gsp_plsqlIfStmt] = Node_parse_plsqlIfStmt;
	Node_parse_functions[t_gsp_plsqlGotoStmt] = Node_parse_plsqlGotoStmt;
	Node_parse_functions[t_gsp_plsqlExitStmt] = Node_parse_plsqlExitStmt;
	Node_parse_functions[t_gsp_plsqlAssignStmt] = Node_parse_plsqlAssignStmt;
	Node_parse_functions[t_gsp_plsqlCursorDeclStmt] = Node_parse_plsqlCursorDeclStmt;
	Node_parse_functions[t_gsp_plsqlRecordTypeDefStmt] = Node_parse_plsqlRecordTypeDefStmt;
	Node_parse_functions[t_gsp_plsqlVarrayTypeDefStmt] = Node_parse_plsqlVarrayTypeDefStmt;
	Node_parse_functions[t_gsp_plsqlTableTypeDefStmt] = Node_parse_plsqlTableTypeDefStmt;
	Node_parse_functions[t_gsp_plsqlNullStmt] = Node_parse_plsqlNullStmt;
	Node_parse_functions[t_gsp_plsqlPipeRowStmt] = Node_parse_plsqlPipeRowStmt;
	Node_parse_functions[t_gsp_bindArgument] = Node_parse_bindArgument;
	Node_parse_functions[t_gsp_execImmeNode] = Node_parse_execImmeNode;
	Node_parse_functions[t_gsp_plsqlOpenforStmt] = Node_parse_plsqlOpenforStmt;
	Node_parse_functions[t_gsp_plsqlOpenStmt] = Node_parse_plsqlOpenStmt;
	Node_parse_functions[t_gsp_plsqlBasicStmt] = Node_parse_plsqlBasicStmt;
	Node_parse_functions[t_gsp_trigger_event] = Node_parse_trigger_event;
	Node_parse_functions[t_gsp_mssql_executeAsSqlNode] = Node_parse_mssql_executeAsSqlNode;
	Node_parse_functions[t_gsp_mssql_executeSqlNode] = Node_parse_mssql_executeSqlNode;
	Node_parse_functions[t_gsp_mssql_execParameter] = Node_parse_mssql_execParameter;
	Node_parse_functions[t_gsp_mssql_dropDbObjectSqlNode] = Node_parse_mssql_dropDbObjectSqlNode;
	Node_parse_functions[t_gsp_mssql_setSqlNode] = Node_parse_mssql_setSqlNode;
	Node_parse_functions[t_gsp_mssql_beginTranSqlNode] = Node_parse_mssql_beginTranSqlNode;
	Node_parse_functions[t_gsp_mssql_raiserrorSqlNode] = Node_parse_mssql_raiserrorSqlNode;
	Node_parse_functions[t_gsp_mssql_gotoSqlNode] = Node_parse_mssql_gotoSqlNode;
	Node_parse_functions[t_gsp_mssql_labelSqlNode] = Node_parse_mssql_labelSqlNode;
	Node_parse_functions[t_gsp_mssql_deallocateSqlNode] = Node_parse_mssql_deallocateSqlNode;
	Node_parse_functions[t_gsp_mssql_beginDialogSqlNode] = Node_parse_mssql_beginDialogSqlNode;
	Node_parse_functions[t_gsp_mssql_sendOnConversationSqlNode] = Node_parse_mssql_sendOnConversationSqlNode;
	Node_parse_functions[t_gsp_mssql_endConversationSqlNode] = Node_parse_mssql_endConversationSqlNode;
	Node_parse_functions[t_gsp_mssql_revertSqlNode] = Node_parse_mssql_revertSqlNode;
	Node_parse_functions[t_gsp_mssql_goSqlNode] = Node_parse_mssql_goSqlNode;
	Node_parse_functions[t_gsp_mssql_useSqlNode] = Node_parse_mssql_useSqlNode;
	Node_parse_functions[t_gsp_mssql_printSqlNode] = Node_parse_mssql_printSqlNode;
	Node_parse_functions[t_gsp_mssql_computeClause] = Node_parse_mssql_computeClause;
	Node_parse_functions[t_gsp_mssql_computeClauseItem] = Node_parse_mssql_computeClauseItem;
	Node_parse_functions[t_gsp_mssql_computeExpr] = Node_parse_mssql_computeExpr;
	Node_parse_functions[t_gsp_mssql_containsTable] = Node_parse_mssql_containsTable;
	Node_parse_functions[t_gsp_mssql_freeTable] = Node_parse_mssql_freeTable;
	Node_parse_functions[t_gsp_mssql_openXML] = Node_parse_mssql_openXML;
	Node_parse_functions[t_gsp_mssql_openRowSet] = Node_parse_mssql_openRowSet;
	Node_parse_functions[t_gsp_mssql_openQuery] = Node_parse_mssql_openQuery;
	Node_parse_functions[t_gsp_mssql_openDatasource] = Node_parse_mssql_openDatasource;
	Node_parse_functions[t_gsp_mssql_tableHint] = Node_parse_mssql_tableHint;
	Node_parse_functions[t_gsp_mssql_bulkInsertSqlNode] = Node_parse_mssql_bulkInsertSqlNode;
	Node_parse_functions[t_gsp_mssql_outputClause] = Node_parse_mssql_outputClause;
	Node_parse_functions[t_gsp_mssql_updateTextSqlNode] = Node_parse_mssql_updateTextSqlNode;
	Node_parse_functions[t_gsp_db2_signal] = Node_parse_db2_signal;
	Node_parse_functions[t_gsp_db2_compoundSqlNode] = Node_parse_db2_compoundSqlNode;
	Node_parse_functions[t_gsp_db2_triggerAction] = Node_parse_db2_triggerAction;
	Node_parse_functions[t_gsp_db2_callStmtSqlNode] = Node_parse_db2_callStmtSqlNode;
	Node_parse_functions[t_gsp_db2_forSqlNode] = Node_parse_db2_forSqlNode;
	Node_parse_functions[t_gsp_db2_iterateStmtSqlNode] = Node_parse_db2_iterateStmtSqlNode;
	Node_parse_functions[t_gsp_db2_leaveStmtSqlNode] = Node_parse_db2_leaveStmtSqlNode;
	Node_parse_functions[t_gsp_db2_setSqlNode] = Node_parse_db2_setSqlNode;
	Node_parse_functions[t_gsp_db2_whileSqlNode] = Node_parse_db2_whileSqlNode;
	Node_parse_functions[t_gsp_db2_repeatSqlNode] = Node_parse_db2_repeatSqlNode;
	Node_parse_functions[t_gsp_db2_gotoSqlNode] = Node_parse_db2_gotoSqlNode;
	Node_parse_functions[t_gsp_db2_loopSqlNode] = Node_parse_db2_loopSqlNode;
}