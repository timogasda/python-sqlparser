//#include "node_visitor.h"
#include "gsp_base.h"
#include "gsp_node.h"
#include "gsp_list.h"
#include "gsp_sourcetoken.h"
#include "gsp_sqlparser.h"
#include <stdlib.h>
#include <Python.h>
#include <structmember.h>

#include "Parser.h"
#include "Statement.h"
#include "Node.h"
#include "ENodeType.h"

// Module functions
static PyMethodDef BridgeMethods[] =
{
     {NULL, NULL, 0, NULL}
};
 
PyMODINIT_FUNC
initsqlparser(void)
{
	PyObject *m;

	// initialize module
    m = Py_InitModule3("sqlparser", BridgeMethods, "Bridge between python and sqlparser");
	 
	if (m == NULL) return;
	
	// Initialize our custom types
	Parser_init_type(m);
	Node_init_type(m);
	Statement_init_type(m);
	Enum_init_type(m);
}