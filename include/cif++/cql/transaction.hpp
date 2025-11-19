/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025  NKI/AVL, Netherlands Cancer Institute
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

#pragma once

#include "cif++/datablock.hpp"
#include "cif++/item.hpp"
#include "cif++/row.hpp"
#include "cif++/validate.hpp"

#include <stdexcept>
#include <vector>

// --------------------------------------------------------------------

namespace cif::cql
{

class result;
class row;
class transaction;

// --------------------------------------------------------------------

class field : private cif::item_handle
{
  public:
	std::string_view name() const &;
	// DDL_PrimitiveType type() const;

	constexpr size_t num() const noexcept;

	std::string_view text() const &
	{
		return cif::item_handle::text();
	}

	/** Return the contents of this item as type @tparam T */
	template <typename T = std::string>
	auto as() const -> T
	{
		return cif::item_handle::as<T>();
	}

	/** Return the contents of this item as type @tparam T or, if not
	 * set, use @a dv as the default value.
	 */
	template <typename T>
	auto value_or(const T &dv) const
	{
		return cif::item_handle::value_or(dv);
	}

  protected:
	friend class row;

	field(cif::item_handle ih)
		: cif::item_handle(ih)
	{
	}
};

// --------------------------------------------------------------------

class row
{
  public:
	row() = default;
	row(row const &rhs) = default;
	row(row &&rhs) = default;
	row &operator=(row const &rhs) noexcept = default;
	row &operator=(row &&rhs) noexcept = default;

	field operator[](size_t ix) const;

  protected:
	friend class result;

	row(row_handle rh, std::vector<int> cols) noexcept
		: m_row(rh)
		, m_cols(cols)
	{
	}

	row_handle m_row;
	std::vector<int> m_cols;
};

// --------------------------------------------------------------------

class const_row_iterator : public row
{
  public:

	using row::operator[];
};

class result
{
  public:
	result();
	result(result const &rhs) noexcept = default;
	result(result &&rhs) noexcept = default;
	result &operator=(result const &rhs) noexcept = default;
	result &operator=(result &&rhs) noexcept = default;

	row one_row() const;
	field one_field() const;

	size_t size() const;

	size_t columns() const;




  private:
	friend class transaction;
	friend class SelectStatement;

	result(std::string query,
		std::vector<row_handle> rows, std::vector<int> columns);

	result expect_columns(size_t cols) const
	{
		if (auto actual = columns(); cols != actual)
			throw std::runtime_error("Unexpected number of columns");
		return *this;
	}

	std::shared_ptr<struct result_impl> m_impl;
};

// --------------------------------------------------------------------

class transaction
{
  public:
	transaction(const datablock &db)
		: m_db(const_cast<datablock &>(db))
	{
	}

	result exec(std::string_view query);

  private:
	datablock &m_db;
};

} // namespace cif::cql