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

class field_ref
{
  public:
	std::string_view name() const &;
	// DDL_PrimitiveType type() const;

	constexpr size_t num() const noexcept
	{
		return m_index;
	}

	std::string_view text() const &
	{
		return m_row[m_index].text();
	}

	/** Return the contents of this item as type @tparam T */
	template <typename T = std::string>
	auto as() const -> T
	{
		return m_row[m_index].as<T>();
	}

	/** Return the contents of this item as type @tparam T or, if not
	 * set, use @a dv as the default value.
	 */
	template <typename T>
	auto value_or(const T &dv) const
	{
		return m_row[m_index].value_or(dv);
	}

	field_ref(row_handle rh, size_t index)
		: m_row(rh)
		, m_index(index)
	{
	}

	field_ref(const field_ref &) = default;
	field_ref(field_ref &&) = default;

	field_ref &operator=(const field_ref &) = default;
	field_ref &operator=(field_ref &&) = default;

  private:
	row_handle m_row;
	size_t m_index = 0;
};

// --------------------------------------------------------------------

class row_ref final
{
  public:
	class const_field_iterator
	{
	  public:
		friend class result;

		using iterator_category = std::forward_iterator_tag;
		using value_type = const field_ref;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type *;
		using reference = value_type &;

		const_field_iterator(const const_field_iterator &) = default;
		const_field_iterator(const_field_iterator &&) = default;

		const_field_iterator &operator=(const const_field_iterator &) = default;
		const_field_iterator &operator=(const_field_iterator &&) = default;

		reference operator*()
		{
			return m_current;
		}

		pointer operator->()
		{
			return &m_current;
		}

		const_field_iterator &operator++()
		{
			if (m_row)
			{
				++m_index;
				m_current = field_ref(m_row, m_index);
			}

			return *this;
		}

		const_field_iterator operator++(int)
		{
			const_field_iterator result(*this);
			this->operator++();
			return result;
		}

		bool operator==(const const_field_iterator &rhs) const
		{
			return m_row == rhs.m_row and m_index == rhs.m_index;
		}

		bool operator!=(const const_field_iterator &rhs) const
		{
			return m_row != rhs.m_row or m_index != rhs.m_index;
		}

	  private:
		friend class row_ref;

		const_field_iterator(const row_handle &row, std::vector<int> field_indices, size_t index = 0)
			: m_row(row)
			, m_field_indices(field_indices)
			, m_index(index)
			, m_current(m_row, m_field_indices[m_index])
		{
		}

		row_handle m_row;
		std::vector<int> m_field_indices;
		size_t m_index = 0;
		field_ref m_current;
	};

	// --------------------------------------------------------------------

	row_ref(row_handle rh, const std::vector<int> &cols)
		: m_row(rh)
		, m_cols(&cols)
	{
	}

	row_ref(const row_ref &) = default;
	row_ref &operator=(const row_ref &) = default;

	// --------------------------------------------------------------------

	const_field_iterator cbegin() const noexcept { return const_field_iterator(m_row, *m_cols, m_cols->front()); }
	const_field_iterator begin() const noexcept { return const_field_iterator(m_row, *m_cols, m_cols->front()); }
	const_field_iterator cend() const noexcept { return const_field_iterator(m_row, *m_cols, m_cols->back()); }
	const_field_iterator end() const noexcept { return const_field_iterator(m_row, *m_cols, m_cols->back()); }

	field_ref front() const noexcept { return field_ref(m_row, m_cols->front()); }
	field_ref back() const noexcept { return field_ref(m_row, m_cols->back()); }

	size_t size() const noexcept { return m_cols->size(); }
	bool empty() const noexcept { return m_cols->empty(); }

	field_ref operator[](size_t index) const noexcept;
	field_ref operator[](std::string_view name) const noexcept;

	// --------------------------------------------------------------------

	bool operator==(const row_ref &rhs) const { return m_row == rhs.m_row and m_cols == rhs.m_cols; }
	bool operator!=(const row_ref &rhs) const { return m_row != rhs.m_row or m_cols != rhs.m_cols; }

  private:
    row_handle m_row;
	const std::vector<int> *m_cols;
};

// --------------------------------------------------------------------

class result
{
  public:
	// --------------------------------------------------------------------

	class const_row_iterator
	{
	  public:
		friend class result;

		using iterator_category = std::forward_iterator_tag;
		using value_type = const row_ref;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type *;
		using reference = value_type &;

		// const_row_iterator() = default;

		const_row_iterator(const const_row_iterator &) = default;
		const_row_iterator(const_row_iterator &&) = default;

		// const_row_iterator &operator=(const const_row_iterator &) = default;
		// const_row_iterator &operator=(const_row_iterator &&) = default;

		reference operator*()
		{
			return m_current;
		}

		pointer operator->()
		{
			return &m_current;
		}

		const_row_iterator &operator++()
		{
			++m_index;
			if (m_index < m_result.m_rows.size())
				m_current = row_ref(m_result.m_rows[m_index], m_result.m_columns);
			return *this;
		}

		const_row_iterator operator++(int)
		{
			const_row_iterator result(*this);
			this->operator++();
			return result;
		}

		bool operator==(const const_row_iterator &rhs) const
		{
			return &m_result == &rhs.m_result and m_index == rhs.m_index;
		}

		bool operator!=(const const_row_iterator &rhs) const
		{
			return &m_result != &rhs.m_result or m_index != rhs.m_index;
		}

	  private:
		const_row_iterator(const result &result, size_t index = 0)
			: m_result(result)
			, m_index(index)
			, m_current(index < m_result.m_rows.size() ? m_result.m_rows[index] : row_handle{}, m_result.m_columns)
		{
		}

		const result &m_result;
		size_t m_index = 0;
		row_ref m_current;
	};

	// --------------------------------------------------------------------

	result();
	result(result const &rhs) noexcept = default;
	result(result &&rhs) noexcept = default;
	result &operator=(result const &rhs) noexcept = default;
	result &operator=(result &&rhs) noexcept = default;

	result(std::vector<row_handle> rows, std::vector<int> columns);

	result(std::string query,
		std::vector<row_handle> rows, std::vector<int> columns);

	row_ref one_row() const;
	field_ref one_field() const;

	// --------------------------------------------------------------------

	const_row_iterator begin() const noexcept { return const_row_iterator(*this, 0); }
	const_row_iterator cbegin() const noexcept { return const_row_iterator(*this, 0); }
	
	const_row_iterator end() const noexcept { return const_row_iterator(*this, size()); }
	const_row_iterator cend() const noexcept { return const_row_iterator(*this, size()); }

	row_ref front() const noexcept { return row_ref(m_rows.front(), m_columns); }
	row_ref back() const noexcept { return row_ref(m_rows.back(), m_columns); }

	size_t size() const noexcept { return m_rows.size(); }
	bool empty() const noexcept { return m_rows.empty(); }

	size_t columns() const;

  private:
	friend class transaction;
	friend class SelectStatement;
	friend class const_row_iterator;

	result expect_columns(size_t cols) const
	{
		if (auto actual = columns(); cols != actual)
			throw std::runtime_error("Unexpected number of columns");
		return *this;
	}

	row_ref at(size_t index) const;

	std::vector<row_handle> m_rows;
	std::vector<int> m_columns;
	std::string m_query;
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

} // namespace cql