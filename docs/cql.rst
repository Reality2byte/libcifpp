CQL
===

The structure of cif files (even of STAR files, from which this format is derived) looks suspiciously like a relational database. When you consider categories to be tables and items to be columns you're almost there. The only problem is linking tables, in common cif files this is done based on multiple columns and the rules are a bit fuzzy allowing for empty columns to still match related columns that do have a value.

An early version of the tool *mmCQL* contained a SQL like language interpreter to SELECT and UPDATE values in cif files. This functionality has been expanded by implementing a full SQL interface using the `SQLite <https://sqlite.org>`_ library. Libcifpp categories are exposed as virtual tables in a SQLite environment and can be queried and manipulated using SQL syntax.

The current limitation is that CREATE TABLE and ALTER TABLE are not supported yet. Since SQLite has no way of supporting this, we will have to write a preprocessor to intercept these statements. That's on the to-do list.

The new *mmcql* tools in `cif-tools <https://github.com/PDB-REDO/cif-tools>`_ uses this new backend and is a command line application you can use similar to the *sqlite* or e.g. *psql* tools for regular SQLite files and postgresql databanks respectively.

Synopsis
--------

.. literalinclude:: ../examples/example-cql.cpp
	:language: c++
	:start-at: #include <cif++/cif++.hpp>

Usage
-----

To start using CQL, you will first have to create a :cpp:class:`connection` to a :cpp:class:`cif::cql::datablock`. Using this connection you can create a :cpp:class:`transaction`. And with the transaction can execute SQL statements using :cpp:func:`cif::cql::transaction::exec`. Or you can use the :cpp:func:`cif::cql::transaction::stream` function to directly pull values from the result.

The result of :cpp:func:`cif::cql::transaction::exec` is a :cpp:class:`cif::cql::result` class which uses a :cpp:class:`cif::category` as storage class.

Implementation Details
----------------------

When the datablock contains a validator (i.e. you loaded a dictionary) the SQL engine knows about all possible items/columns that are allowed. It also knows about links/relations between categories, just like the regular libcifpp query mechanism. So, updating and deleting will cascade automatically.

Another point is data types. cif files can have numbers, strings or NULL values. Same goes for SQLite. However, when a file was loaded without a dictionary, the type of an item is dependent on its content. If something was parsed as being a number, the type will be numeric. If however, the file does contain a dictionary/validator, the type is determined by this dictionary. So, even if it looks like a number, it still might be a string internally. Good example is the ID field in atom_site, or the auth_seq_id/auth_seq_num fields. In the WHERE clause this may have unexpected results, so you may have to fall back to using `CAST <https://sqlite.org/lang_expr.html#castexpr>`_.

The API for this functionality is a bit new, there may be room for improvement. Ideas are welcome.