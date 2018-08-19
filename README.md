B2T lua library
===============

To/From binary converter to/from lua or json.

Methods
-------

### to_bin(data, inType, binType)
`syntax: binData = b2t.to_bin(luaTable, b2t.json, b2t.cbor)`  
`syntax: binData = b2t.to_bin([[{"key": "value"}]], b2t.json, b2t.cbor)`  

Converts `data` (lua value or json string) to binary data.

### from_bin(binData, outType, binType, [jsonIndent])

`syntax: jsonString = b2t.from_bin(binData, b2t.json, b2t.cbor)`  
`syntax: jsonString = b2t.from_bin(binData, b2t.json, b2t.cbor, 4)`  
`syntax: luaTable = b2t.from_bin(binData, b2t.table, b2t.cbor)`  

Converts `binData` to `outType` (lua value or json string) with optional indent.

Values
------

### Types

#### b2t.json

#### b2t.table

### Binary Types

#### b2t.cbor

#### b2t.msgpack

#### b2t.ubjson