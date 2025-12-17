/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 NKI/AVL, Netherlands Cancer Institute
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cif++/cql/transaction.hpp"

#include "../sqlite3.h"
#include "cif++/category.hpp"
#include "cif++/condition.hpp"
#include "cif++/datablock.hpp"
#include "cif++/iterator.hpp"
#include "cif++/row.hpp"
#include "cif++/text.hpp"
#include "cif++/validate.hpp"

#include <cstdint>
#include <exception>
#include <iomanip>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace cif::cql
{

struct result_impl
{
	category m_cat;
	std::string m_query;
};

// --------------------------------------------------------------------

size_t row_ref::size() const noexcept
{
	return m_result_impl->m_cat.get_item_count();
}

field_ref row_ref::operator[](std::string_view name) const
{
	for (int ix = 0; auto &item : m_result_impl->m_cat.get_items())
	{
		if (iequals(item, name))
			return { m_row, ix, m_result_impl };
		++ix;
	}
	throw std::runtime_error("Column not defined in query result");
}

// --------------------------------------------------------------------

result::result(category &&cat, const std::string &query)
	: m_impl(new result_impl{ std::forward<category>(cat), query })
{
}

category &result::get_category() const
{
	return m_impl->m_cat;
}

size_t result::size() const noexcept
{
	return m_impl->m_cat.size();
}

size_t result::column_count() const
{
	return m_impl->m_cat.get_item_count();
}

result::iterator result::begin() const noexcept
{
	return { m_impl, m_impl->m_cat.begin() };
}

result::iterator result::cbegin() const noexcept
{
	return { m_impl, m_impl->m_cat.cbegin() };
}

result::iterator result::end() const noexcept
{
	return { m_impl, m_impl->m_cat.end() };
}

result::iterator result::cend() const noexcept
{
	return { m_impl, m_impl->m_cat.cend() };
}

row_ref result::front() const
{
	return { m_impl->m_cat.front(), m_impl };
}

row_ref result::back() const
{
	return { m_impl->m_cat.back(), m_impl };
}

// --------------------------------------------------------------------

struct connection_impl
{
	datablock &m_db;
	sqlite3 *m_sqlite_db = nullptr;

	connection_impl(datablock &db);

	~connection_impl()
	{
		sqlite3_close(m_sqlite_db);
	}

	int Connect(sqlite3 *db, int argc, const char *const *argv, sqlite3_vtab **ppVtab, char **pzErr);

	// The module interface

	static int Create(sqlite3 *db, void *pAux, int argc, const char *const *argv, sqlite3_vtab **ppVtab, char **pzErr);
	static int Connect(sqlite3 *db, void *pAux, int argc, const char *const *argv, sqlite3_vtab **ppVtab, char **pzErr);
	static int Disconnect(sqlite3_vtab *pVtab);
	static int Open(sqlite3_vtab *p, sqlite3_vtab_cursor **ppCursor);
	static int Close(sqlite3_vtab_cursor *cur);
	static int Next(sqlite3_vtab_cursor *cur);
	static int Column(
		sqlite3_vtab_cursor *cur, /* The cursor */
		sqlite3_context *ctx,     /* First argument to sqlite3_result_...() */
		int i);                   /* Which column to return */
	static int Rowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid);
	static int Eof(sqlite3_vtab_cursor *cur);
	static int Filter(sqlite3_vtab_cursor *pVtabCursor, int idxNum, const char *idxStr, int argc, sqlite3_value **argv);
	static int BestIndex(sqlite3_vtab *tab, sqlite3_index_info *pIdxInfo);

	static int Update(sqlite3_vtab *pVTab, int argc, sqlite3_value **argv, sqlite_int64 *pRowid);

	static int Rename(sqlite3_vtab *pVtab, const char *zNew);

	// Transaction support
	static int Begin(sqlite3_vtab *pVTab);
	static int Commit(sqlite3_vtab *pVTab);
	static int Rollback(sqlite3_vtab *pVTab);

	static sqlite3_module s_module;
};

struct virtual_table
{
	sqlite3_vtab base;
	category &m_cat;
	std::stack<category> m_rollback_buffer;
};

struct virtual_cursor
{
	sqlite3_vtab_cursor base;
	category &m_cat;

	std::unique_ptr<conditional_iterator_proxy<category>> m_result;
	conditional_iterator_proxy<category>::iterator m_cur;
};

sqlite3_module connection_impl::s_module{
	/* iVersion    */ 0,
	/* xCreate     */ Create,
	/* xConnect    */ Connect,
	/* xBestIndex  */ BestIndex,
	/* xDisconnect */ Disconnect,
	/* xDestroy    */ Disconnect,
	/* xOpen       */ Open,
	/* xClose      */ Close,
	/* xFilter     */ Filter,
	/* xNext       */ Next,
	/* xEof        */ Eof,
	/* xColumn     */ Column,
	/* xRowid      */ Rowid,
	/* xUpdate     */ Update,
	/* xBegin      */ Begin,
	/* xSync       */ 0,
	/* xCommit     */ Commit,
	/* xRollback   */ Rollback,
	/* xFindFunction */ 0,
	/* xRename     */ Rename,
	/* xSavepoint  */ 0,
	/* xRelease    */ 0,
	/* xRollbackTo */ 0,
	/* xShadowName */ 0,
	/* xIntegrity  */ 0
};

/*
** The templatevtabConnect() method is invoked to create a new
** template virtual table.
**
** Think of this routine as the constructor for connection_impl objects.
**
** All this routine needs to do is:
**
**    (1) Allocate the connection_impl object and initialize all fields.
**
**    (2) Tell SQLite (via the sqlite3_declare_vtab() interface) what the
**        result set of queries against the virtual table will look like.
*/

int connection_impl::Connect(sqlite3 *db, void *pAux, int argc, const char *const *argv, sqlite3_vtab **ppVtab, char **pzErr)
{
	connection_impl *impl = reinterpret_cast<connection_impl *>(pAux);
	try
	{
		return impl->Connect(db, argc, argv, ppVtab, pzErr);
	}
	catch (const std::exception &ex)
	{
		*pzErr = sqlite3_mprintf("%s", ex.what());
		return SQLITE_ERROR;
	}
}

int connection_impl::Create(sqlite3 *db, void *pAux, int argc, const char *const *argv, sqlite3_vtab **ppVtab, char **pzErr)
{
	return Connect(db, pAux, argc, argv, ppVtab, pzErr);
}

// --------------------------------------------------------------------

int connection_impl::Connect(sqlite3 *db, int argc, const char *const *argv, sqlite3_vtab **ppVtab, char **pzErr)
{
	if (argc < 3)
		throw std::runtime_error("Insufficient arguments to module connect");

	auto cat = m_db.get(argv[2]);
	if (cat == nullptr)
		throw std::runtime_error(std::format("Category {} is not known in this databank", argv[2]));

	std::vector<std::string> items;

	if (auto cv = cat->get_cat_validator(); cv != nullptr)
	{
		for (std::string item : cat->get_items())
		{
			auto iv = cv->get_validator_for_item(item);

			std::string primaryKey;
			if (cv->m_keys.size() == 1 and cv->m_keys.front() == item)
				primaryKey = " PRIMARY KEY";

			if (iv != nullptr)
			{
				if (iv->m_type->m_primitive_type == DDL_PrimitiveType::Numb)
				{
					if (iequals(iv->m_type->m_name, "int"))
					{
						items.emplace_back(std::format("'{}' {}", item, " INTEGER") + primaryKey);
						continue;
					}

					if (iequals(iv->m_type->m_name, "float"))
					{
						items.emplace_back(std::format("'{}' {}", item, " REAL") + primaryKey);
						continue;
					}
				}
			}

			items.emplace_back(std::format("'{}' {}", item, " TEXT") + primaryKey);
		}
	}
	else
	{
		for (auto item : cat->get_items())
			items.emplace_back(std::format("'{}'", item));
	}

	auto vtab = std::make_unique<virtual_table>(sqlite3_vtab{}, *cat);

	auto createStmt = std::format("CREATE TABLE {} ({})", cat->name(), join(items, ", "));

	int rc = sqlite3_declare_vtab(db, createStmt.c_str());
	if (rc == SQLITE_OK)
		*ppVtab = reinterpret_cast<sqlite3_vtab *>(vtab.release());

	return rc;
}

/*
** This method is the destructor for connection_impl objects.
*/
int connection_impl::Disconnect(sqlite3_vtab *pVtab)
{
	virtual_table *p = reinterpret_cast<virtual_table *>(pVtab);
	delete p;
	return SQLITE_OK;
}

/*
** Constructor for a new templatevtab_cursor object.
*/
int connection_impl::Open(sqlite3_vtab *pVtab, sqlite3_vtab_cursor **ppCursor)
{
	virtual_table *p = reinterpret_cast<virtual_table *>(pVtab);

	auto cursor = std::make_unique<virtual_cursor>(sqlite3_vtab_cursor{}, p->m_cat);
	*ppCursor = reinterpret_cast<sqlite3_vtab_cursor *>(cursor.release());
	return SQLITE_OK;
}

/*
** Destructor for a templatevtab_cursor.
*/
int connection_impl::Close(sqlite3_vtab_cursor *cur)
{
	auto pCur = reinterpret_cast<virtual_cursor *>(cur);
	delete pCur;
	return SQLITE_OK;
}

/*
** Advance a templatevtab_cursor to its next row of output.
*/
int connection_impl::Next(sqlite3_vtab_cursor *cur)
{
	auto pCur = reinterpret_cast<virtual_cursor *>(cur);
	++pCur->m_cur;
	return SQLITE_OK;
}

/*
** Return values of columns for the row at which the templatevtab_cursor
** is currently pointing.
*/
int connection_impl::Column(
	sqlite3_vtab_cursor *cur, /* The cursor */
	sqlite3_context *ctx,     /* First argument to sqlite3_result_...() */
	int i                     /* Which column to return */
)
{
	auto pCur = reinterpret_cast<virtual_cursor *>(cur);
	auto rh = *pCur->m_cur;
	auto item = rh[i];

	if (item.is_null() or item.is_unknown())
		sqlite3_result_null(ctx);
	else if (auto cv = pCur->m_cat.get_cat_validator(); cv != nullptr)
	{
		if (auto iv = cv->get_validator_for_item(pCur->m_cat.get_item_name(i));
			iv != nullptr and iv->m_type->m_primitive_type == DDL_PrimitiveType::Numb)
		{
			if (iequals(iv->m_type->m_name, "int"))
			{
				auto v = item.as<int64_t>();
				sqlite3_result_int64(ctx, v);
			}
			else if (iequals(iv->m_type->m_name, "float"))
			{
				auto v = item.as<double>();
				sqlite3_result_double(ctx, v);
			}
			else
				sqlite3_result_text(ctx, item.text().data(), item.text().size(), SQLITE_STATIC);
		}
		else
			sqlite3_result_text(ctx, item.text().data(), item.text().size(), SQLITE_STATIC);
	}
	else
		sqlite3_result_text(ctx, item.text().data(), item.text().size(), SQLITE_STATIC);

	return SQLITE_OK;
}

/*
** Return the rowid for the current row.  In this implementation, the
** rowid is the same as the output value.
*/
int connection_impl::Rowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid)
{
	auto pCur = reinterpret_cast<virtual_cursor *>(cur);
	row_handle rh = *pCur->m_cur;
	*pRowid = rh.row_id();
	return SQLITE_OK;
}

/*
** Return TRUE if the cursor has been moved off of the last
** row of output.
*/
int connection_impl::Eof(sqlite3_vtab_cursor *cur)
{
	auto pCur = reinterpret_cast<virtual_cursor *>(cur);
	return pCur->m_cur == pCur->m_cat.end();
}

/*
** This method is called to "rewind" the templatevtab_cursor object back
** to the first row of output.  This method is always called at least
** once prior to any call to templatevtabColumn() or templatevtabRowid() or
** templatevtabEof().
*/
int connection_impl::Filter(sqlite3_vtab_cursor *pVtabCursor, int idxNum, const char *idxStr, int argc, sqlite3_value **argv)
{
	auto pCur = reinterpret_cast<virtual_cursor *>(pVtabCursor);

	pCur->m_result.reset();

	try
	{
		if (idxStr != nullptr)
		{
			struct membuf : public std::streambuf
			{
				membuf(char *text, std::size_t length)
				{
					this->setg(text, text, text + length);
				}
			} buffer(const_cast<char *>(idxStr), strlen(idxStr));

			std::istream is(&buffer);

			std::regex rx("^(.+?)( IS NULL| IS NOT NULL|(?: < | <= | == | >= | > ))(.+)?$");

			condition cond;
			std::string line;
			while (std::getline(is, line))
			{
				std::smatch m;
				if (not std::regex_match(line, m, rx))
					throw std::runtime_error("Internal error in cql, no match");

				if (m[2] == " IS NULL")
					cond = std::move(cond) and cif::key(m[1]) == cif::null;
				else if (m[2] == " IS NOT NULL")
					cond = std::move(cond) and cif::key(m[1]) != cif::null;
				else if (m[3].str().starts_with("\""))
				{
					std::istringstream isv(m[3]);
					std::string value;
					isv >> std::quoted(value);

					if (m[2] == " < ")
						cond = std::move(cond) and cif::key(m[1]) < value;
					else if (m[2] == " <- ")
						cond = std::move(cond) and cif::key(m[1]) <= value;
					else if (m[2] == " == ")
						cond = std::move(cond) and cif::key(m[1]) == value;
					else if (m[2] == " >= ")
						cond = std::move(cond) and cif::key(m[1]) >= value;
					else if (m[2] == " > ")
						cond = std::move(cond) and cif::key(m[1]) > value;
				}
				else
				{
					double value;
					const auto &[ptr, ec] = std::from_chars(m[3].str().data(), m[3].str().data() + m[3].str().length(), value);
					if (ec != std::errc{})
						throw std::system_error(std::make_error_code(ec));

					if (m[2] == " < ")
						cond = std::move(cond) and cif::key(m[1]) < value;
					else if (m[2] == " <- ")
						cond = std::move(cond) and cif::key(m[1]) <= value;
					else if (m[2] == " == ")
						cond = std::move(cond) and cif::key(m[1]) == value;
					else if (m[2] == " >= ")
						cond = std::move(cond) and cif::key(m[1]) >= value;
					else if (m[2] == " > ")
						cond = std::move(cond) and cif::key(m[1]) > value;
				}
			}

			pCur->m_result = std::make_unique<conditional_iterator_proxy<category>>(pCur->m_cat.find(std::move(cond)));
			pCur->m_cur = pCur->m_result->begin();
		}
	}
	catch (const std::exception &ex)
	{
		std::cerr << "Internal error: " << ex.what() << "\n";
	}

	if (not pCur->m_result)
	{
		condition cond = all();
		pCur->m_result = std::make_unique<conditional_iterator_proxy<category>>(pCur->m_cat.find(std::move(cond)));
		pCur->m_cur = pCur->m_result->begin();
	}

	return SQLITE_OK;
}

/*
** SQLite will invoke this method one or more times while planning a query
** that uses the virtual table.  This routine needs to create
** a query plan for each invocation and compute an estimated cost for that
** plan.
*/
int connection_impl::BestIndex(sqlite3_vtab *pVtab, sqlite3_index_info *pIdxInfo)
{
	virtual_table *p = reinterpret_cast<virtual_table *>(pVtab);

	try
	{
		std::ostringstream os;
		bool ok = true;

		if (pIdxInfo->nConstraint > 0)
		{
			auto constraint = [&os](std::string_view item, sqlite3_value *val, unsigned char op)
			{
				bool result = true;
				switch (op)
				{
					case SQLITE_INDEX_CONSTRAINT_EQ:
						os << item << " == ";
						break;
					case SQLITE_INDEX_CONSTRAINT_GT:
						os << item << " > ";
						break;
					case SQLITE_INDEX_CONSTRAINT_LE:
						os << item << " <= ";
						break;
					case SQLITE_INDEX_CONSTRAINT_LT:
						os << item << " < ";
						break;
					case SQLITE_INDEX_CONSTRAINT_GE:
						os << item << " >= ";
						break;
					default:
						result = false;
						break;
				}

				if (result)
				{
					switch (sqlite3_value_type(val))
					{
						case SQLITE_INTEGER:
							os << sqlite3_value_int64(val) << "\n";
							break;
						case SQLITE_FLOAT:
							os << sqlite3_value_double(val) << "\n";
							break;
						default:
						{
							std::string s = (const char *)sqlite3_value_text(val);
							if (s.find("\n") == std::string::npos)
								os << std::quoted(s) << "\n";
							else
								result = false;
							break;
						}
					}
				}

				return result;
			};

			for (int i = 0; ok and i < pIdxInfo->nConstraint; ++i)
			{
				auto &info = pIdxInfo->aConstraint[i];
				auto item = p->m_cat.get_item_name(info.iColumn);

				sqlite3_value *pVal;

				switch (info.op)
				{
					case SQLITE_INDEX_CONSTRAINT_EQ:
					case SQLITE_INDEX_CONSTRAINT_GT:
					case SQLITE_INDEX_CONSTRAINT_LE:
					case SQLITE_INDEX_CONSTRAINT_LT:
					case SQLITE_INDEX_CONSTRAINT_GE:
						if (sqlite3_vtab_rhs_value(pIdxInfo, i, &pVal) == SQLITE_OK and constraint(item, pVal, info.op))
						{
							pIdxInfo->aConstraintUsage[i].omit = 1;
							if (i < 63 and sqlite3_libversion_number() >= 3010000)
								pIdxInfo->colUsed |= 1 << i;
						}
						else
							ok = false;
						break;
					// case SQLITE_INDEX_CONSTRAINT_MATCH:
					// 	break;
					// case SQLITE_INDEX_CONSTRAINT_LIKE:
					// 	break;
					// case SQLITE_INDEX_CONSTRAINT_GLOB:
					// 	break;
					// case SQLITE_INDEX_CONSTRAINT_REGEXP:
					// 	break;
					case SQLITE_INDEX_CONSTRAINT_NE:
						break;
					// case SQLITE_INDEX_CONSTRAINT_ISNOT:
					// 	break;
					case SQLITE_INDEX_CONSTRAINT_ISNOTNULL:
						os << item << " IS NOT NULL\n";
						pIdxInfo->aConstraintUsage[i].omit = 1;
						if (i < 63 and sqlite3_libversion_number() >= 3010000)
							pIdxInfo->colUsed |= 1 << i;
						break;
					case SQLITE_INDEX_CONSTRAINT_ISNULL:
						os << item << " IS NULL\n";
						pIdxInfo->aConstraintUsage[i].omit = 1;
						if (i < 63 and sqlite3_libversion_number() >= 3010000)
							pIdxInfo->colUsed |= 1 << i;
						break;
						// case SQLITE_INDEX_CONSTRAINT_IS:
						// 	break;
						// case SQLITE_INDEX_CONSTRAINT_LIMIT:
						// 	break;
						// case SQLITE_INDEX_CONSTRAINT_OFFSET:
						// 	break;
						// case SQLITE_INDEX_CONSTRAINT_FUNCTION				:
						// 	break;

					default:
						ok = false;
						break;
				}
			}
		}

		if (auto cs = os.str(); ok and not cs.empty())
		{
			pIdxInfo->idxStr = sqlite3_mprintf("%s", cs.c_str());
			pIdxInfo->needToFreeIdxStr = 1;
		}
	}
	catch (const std::exception &ex)
	{
		std::cerr << ex.what() << "\n";
		if (sqlite3_libversion_number() >= 3010000)
			pIdxInfo->colUsed = 0;
	}

	pIdxInfo->estimatedCost = p->m_cat.size();
	pIdxInfo->estimatedRows = p->m_cat.size();
	return SQLITE_OK;
}

int connection_impl::Update(sqlite3_vtab *pVTab, int argc, sqlite3_value **argv, sqlite_int64 *pRowid)
{
	virtual_table *p = reinterpret_cast<virtual_table *>(pVTab);

	int rc = SQLITE_ERROR;

	try
	{
		auto addr = sqlite3_value_int64(argv[0]);

		if (argc == 1) // DELETE
		{
			p->m_cat.erase(row_handle{ p->m_cat, *reinterpret_cast<const cif::row *>(addr) });
			rc = SQLITE_OK;
		}
		else if (addr == 0) // INSERT
		{
			addr = sqlite3_value_int64(argv[1]);
			if (addr == 0)	 // We do not accept rowid's here
			{
				row_initializer data;
				for (int i = 2; i < argc; ++i)
				{
					switch (sqlite3_value_type(argv[i]))
					{
						case SQLITE_INTEGER:
							data.emplace_back(p->m_cat.get_item_name(i - 2), sqlite3_value_int64(argv[i]));
							break;
						case SQLITE_FLOAT:
							data.emplace_back(p->m_cat.get_item_name(i - 2), sqlite3_value_double(argv[i]));
							break;
						default:
							data.emplace_back(p->m_cat.get_item_name(i - 2), (const char *)sqlite3_value_text(argv[i]));
							break;
					}
				}

				auto r = p->m_cat.emplace(std::move(data));
				*pRowid = r->row_id();
				rc = SQLITE_OK;
			}
		}
		else // UPDATE
		{
			row_handle rh{ p->m_cat, *reinterpret_cast<const cif::row *>(addr) };

			row_initializer data;
			for (int i = 2; i < argc; ++i)
			{
				switch (sqlite3_value_type(argv[i]))
				{
					case SQLITE_INTEGER:
						data.emplace_back(p->m_cat.get_item_name(i - 2), sqlite3_value_int64(argv[i]));
						break;
					case SQLITE_FLOAT:
						data.emplace_back(p->m_cat.get_item_name(i - 2), sqlite3_value_double(argv[i]));
						break;
					default:
						data.emplace_back(p->m_cat.get_item_name(i - 2), (const char *)sqlite3_value_text(argv[i]));
						break;
				}
			}

			rh.assign(data);
			*pRowid = addr;
			rc = SQLITE_OK;
		}
	}
	catch (const std::exception &ex)
	{
		rc = SQLITE_ERROR;
	}

	return rc;
}

int connection_impl::Rename(sqlite3_vtab *pVtab, const char *zNew)
{
	virtual_table *p = reinterpret_cast<virtual_table *>(pVtab);

	p->m_cat.name(zNew);

	return SQLITE_OK;
}

int connection_impl::Begin(sqlite3_vtab *pVTab)
{
	virtual_table *p = reinterpret_cast<virtual_table *>(pVTab);
	p->m_rollback_buffer.push(p->m_cat);
	return SQLITE_OK;
}

int connection_impl::Commit(sqlite3_vtab *pVTab)
{
	virtual_table *p = reinterpret_cast<virtual_table *>(pVTab);
	if (not p->m_rollback_buffer.empty())
		p->m_rollback_buffer.pop();
	return SQLITE_OK;
}

int connection_impl::Rollback(sqlite3_vtab *pVTab)
{
	virtual_table *p = reinterpret_cast<virtual_table *>(pVTab);
	if (not p->m_rollback_buffer.empty())
	{
		std::swap(p->m_cat, p->m_rollback_buffer.top());
		p->m_rollback_buffer.pop();
	}
	return SQLITE_OK;
}

// --------------------------------------------------------------------

connection_impl::connection_impl(datablock &db)
	: m_db(db)
{
	auto rc = sqlite3_open(":memory:", &m_sqlite_db);

	if (rc)
		throw std::runtime_error(std::format("Cannot open databank: {}", sqlite3_errmsg(m_sqlite_db)));

	rc = sqlite3_create_module_v2(m_sqlite_db, "CIFPP", &connection_impl::s_module, this, nullptr);

	if (rc)
		throw std::runtime_error(std::format("Cannot create module: {}", sqlite3_errmsg(m_sqlite_db)));

	// Now, create a table for all known categories in the datablock

	for (auto &cat : db)
	{
		char *errmsg;
		rc = sqlite3_exec(m_sqlite_db,
			("CREATE VIRTUAL TABLE " + cat.name() + " USING CIFPP;").c_str(),
			nullptr, nullptr, &errmsg);
		if (rc != SQLITE_OK)
		{
			if (errmsg != nullptr)
			{
				std::string err = errmsg;
				sqlite3_free(errmsg);

				throw std::runtime_error("Error creating virtual tables for the categories: " + err);
			}

			throw std::runtime_error("Error creating virtual tables for the categories");
		}
	}
}

connection::connection(datablock &db)
	: m_impl(new connection_impl(db))
{
}

connection::~connection()
{
	delete m_impl;
}

// --------------------------------------------------------------------

transaction::transaction(connection &conn)
	: m_conn(conn)
{
	char *errmsg = nullptr;
	std::string err;
	int rc = sqlite3_exec(m_conn.m_impl->m_sqlite_db, "BEGIN TRANSACTION;", nullptr, nullptr, &errmsg);
	if (errmsg)
	{
		err = errmsg;
		sqlite3_free(errmsg);
	}

	if (rc != SQLITE_OK)
		throw std::runtime_error("Error starting transaction: " + err);
	m_transaction_active = true;
}

transaction::~transaction()
{
	if (m_transaction_active)
		rollback();
}

void transaction::commit()
{
	if (m_transaction_active)
	{
		char *errmsg = nullptr;
		std::string err;
		int rc = sqlite3_exec(m_conn.m_impl->m_sqlite_db, "COMMIT TRANSACTION;", nullptr, nullptr, &errmsg);
		if (errmsg)
		{
			err = errmsg;
			sqlite3_free(errmsg);
		}
	
		if (rc != SQLITE_OK)
			throw std::runtime_error("Error committing transaction: " + err);

		m_transaction_active = false;
	}
}

void transaction::rollback()
{
	if (m_transaction_active)
	{
		char *errmsg = nullptr;
		std::string err;
		int rc = sqlite3_exec(m_conn.m_impl->m_sqlite_db, "ROLLBACK TRANSACTION;", nullptr, nullptr, &errmsg);
		if (errmsg)
		{
			err = errmsg;
			sqlite3_free(errmsg);
		}
	
		if (rc != SQLITE_OK)
			throw std::runtime_error("Error rolling back transaction: " + err);

		m_transaction_active = false;
	}
}

result transaction::exec(const std::string &query)
{
	category cat;

	sqlite3_stmt *stmt = nullptr;

	try
	{
		int rc = sqlite3_prepare_v2(m_conn.m_impl->m_sqlite_db, query.data(), query.size(),
			&stmt, nullptr);

		if (rc != SQLITE_OK)
			throw std::runtime_error(std::format("Error preparing statement: {}", sqlite3_errmsg(m_conn.m_impl->m_sqlite_db)));

		for (int i = 0; i < sqlite3_column_count(stmt); ++i)
			cat.add_item(sqlite3_column_name(stmt, i));

		for (;;)
		{
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW)
			{
				row_initializer data;

				for (int i = 0; i < sqlite3_column_count(stmt); ++i)
				{
					switch (sqlite3_column_type(stmt, i))
					{
						case SQLITE_INTEGER:
							data.emplace_back(sqlite3_column_name(stmt, i), sqlite3_column_int64(stmt, i));
							break;
						case SQLITE_FLOAT:
							data.emplace_back(sqlite3_column_name(stmt, i), sqlite3_column_double(stmt, i));
							break;
						case SQLITE_TEXT:
							data.emplace_back(sqlite3_column_name(stmt, i), (const char *)sqlite3_column_text(stmt, i));
							break;
						case SQLITE_BLOB:
							// data.emplace_back(sqlite3_column_name(stmt, i), sqlite3_column_int64(stmt, i));
							throw std::runtime_error("Unexpected: blob in result");
							break;
						case SQLITE_NULL:
							data.emplace_back(sqlite3_column_name(stmt, i), ".");
							break;
					}
				}

				cat.emplace(std::move(data));
				continue;
			}

			if (rc == SQLITE_BUSY)
				throw std::runtime_error("Oops, busy?");
			if (rc == SQLITE_DONE)
				break;
			if (rc == SQLITE_ERROR)
				throw std::runtime_error(std::format("Error in sqlite: {}", sqlite3_errmsg(m_conn.m_impl->m_sqlite_db)));

			throw std::runtime_error("Unknown result from step");
		}

		sqlite3_finalize(stmt);

		return result(std::move(cat), query);
	}
	catch (const std::exception &ex)
	{
		if (stmt)
			sqlite3_finalize(stmt);
		throw;
	}
}

} // namespace cif::cql