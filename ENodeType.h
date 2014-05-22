#include <stdlib.h>
#include <Python.h>

#include <structmember.h>

typedef struct {
	PyObject_HEAD

	PyObject *dict;
} Enum;

void Enum_init_type(PyObject *m);

void EVendor_init(Enum *type);

void gsp_efindsqlstatetype_init(Enum *type);
void EErrorType_init(Enum *type);
void gsp_token_code_init(Enum *type);
void EStmtType_init(Enum *type);
void ETokenStatus_init(Enum *type);
void EJoinSource_init(Enum *type);
void EJoinType_init(Enum *type);
void EFireMode_init(Enum *type);
void ETriggerMode_init(Enum *type);
void EStoreProcedureMode_init(Enum *type);
void EParameterMode_init(Enum *type);
void EHowtoSetValue_init(Enum *type);
void EWhatDeclared_init(Enum *type);
void EInsertValue_init(Enum *type);
void EIndexType_init(Enum *type);
void EAggregateType_init(Enum *type);
void EAlterTableOptionType_init(Enum *type);
void ETableSource_init(Enum *type);
void EConstraintType_init(Enum *type);
void EKeyReferenceType_init(Enum *type);
void ESetOperator_init(Enum *type);
void EDataType_init(Enum *type);
void EFunctionType_init(Enum *type);
void EDBObjectType_init(Enum *type);
void ENodeType_init(Enum *type);
void EExpressionType_init(Enum *type);
void gsp_dbvendor_init(Enum *type);
void EAccessMode_init(Enum *type);
void EQeuryClause_init(Enum *type);
void EConstantType_init(Enum *type);
void EKeyActionType_init(Enum *type);
void EMatchType_init(Enum *type);
void EDistinctType_init(Enum *type);
void gsp_EDeclareType_init(Enum *type);
void gsp_EVariableType_init(Enum *type);
void ECreateType_init(Enum *type);
void EExecType_init(Enum *type);
void ESetType_init(Enum *type);


static PyMemberDef NodeTypeE_members[] = {
	{"__dict__", T_OBJECT, offsetof(Enum, dict), READONLY},
    {NULL}  /* Sentinel */
};

static PyMethodDef NodeTypeE_methods[] = {
	// regular methods
    {NULL}  /* Sentinel */
};

static PyTypeObject EnumType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "parsebridge.Enum",             /*tp_name*/
    sizeof(Enum), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    0,                         /*tp_dealloc*/
    0,                         /*tp_print*/
	0, //(getattrfunc)NodeTypeE_getattr,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    PyObject_GenericGetAttr,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "Node Type enum",           /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    NodeTypeE_methods,          /* tp_methods */
	NodeTypeE_members,                       /* tp_members */
    0,                       /* tp_getset */
    0,                       /* tp_base */
    0,                       /* tp_dict */
    0,                       /* tp_descr_get */
    0,                       /* tp_descr_set */
	offsetof(Enum, dict),                       /* tp_dictoffset */
    0, //(initproc)Shoddy_init,   /* tp_init */
    0,                       /* tp_alloc */
    0,                       /* tp_new */	
}; 