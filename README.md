python-sqlparser
================

A Python Module for the "General SQL Parser" library (sqlparser.com)

Supported Systems
-----------------
This library currently supports: Windows 32bit/64bit and Linux 32bit/64bit. 
There are currently no binaries for Mac OSX available.

Also, please note that this module does not support Python 3.*

Installation
------------
Simply clone or download this git and execute

	python setup.py build
	python setup.py install

The setup script will automatically download the right library (from sqlparser.com) for you.

Usage
-----
Simply import the module with
	import sqlparser
The following example will parse a simple query:
```python
import sqlparser

query = "SELECT a, b FROM table_1 WHERE c > 20"
parser = sqlparser.Parser()

# Check for syntax errors
if parser.check_syntax(query) == 0:
	# Get first statement from the query
	stmt = parser.get_statement(0)

	# Get root node
	root = stmt.get_root()

	print root.__dict__
```

And print the node information the SELECT node:
```javascript
{"computeClause": None,
 "cteList": None,
 "expandOnClause": None,
 "fetchFirstClause": None,
 "forupdateClause": None,
 "fromTableList": <sqlparser.Node object at 0x7ff48c5eed50>,
 "groupByClause": None,
 "hierarchicalClause": None,
 "intoClause": None,
 "intoTableClause": None,
 "isolationClause": None,
 "leftNode": None,
 "limitClause": None,
 "lockingClause": None,
 "node_type": 5,
 "optimizeForClause": None,
 "orderbyClause": None,
 "qualifyClause": None,
 "resultColumnList": <sqlparser.Node object at 0x7ff48c5ee618>,
 "rightNode": None,
 "sampleClause": None,
 "selectDistinct": None,
 "selectToken": "SELECT",
 "setOperator": 0,
 "topClause": None,
 "valueClause": None,
 "whereCondition": <sqlparser.Node object at 0x7ff48c5eea78>,
 "windowClause": None,
 "withClauses": None}
 ```

Examples
--------
For more examples please check the examples directory.