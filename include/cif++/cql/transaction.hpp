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

#include "cif++/category.hpp"
#include "cif++/datablock.hpp"
#include "cif++/item.hpp"
#include "cif++/row.hpp"
#include "cif++/validate.hpp"

#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>

// --------------------------------------------------------------------

namespace cif::cql
{

class result;
class row;
class transaction;
class connection;

struct result_impl;

// --------------------------------------------------------------------

class field_ref final
{
  public:
	std::string_view name() const &
	{
		return m_row.get_category().get_item_name(m_index);
	}

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

	field_ref(row_handle rh, int col, std::shared_ptr<result_impl> result_impl)
		: m_row(rh)
		, m_index(col)
		, m_result_impl(result_impl)
	{
	}

	field_ref(const field_ref &) = default;
	field_ref(field_ref &&) = default;

	field_ref &operator=(const field_ref &) = default;
	field_ref &operator=(field_ref &&) = default;

  private:
	row_handle m_row;
	int m_index;

	std::shared_ptr<result_impl> m_result_impl;
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
				++m_col;
				m_current = field_ref(m_row, m_col, m_result_impl);
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
			return m_row == rhs.m_row and m_col == rhs.m_col;
		}

		bool operator!=(const const_field_iterator &rhs) const
		{
			return m_row != rhs.m_row or m_col != rhs.m_col;
		}

	  private:
		friend class row_ref;

		const_field_iterator(row_handle row, int column, std::shared_ptr<result_impl> result_impl)
			: m_row(row)
			, m_col(column)
			, m_current(m_row, m_col, result_impl)
			, m_result_impl(result_impl)
		{
		}

		row_handle m_row;
		int m_col;
		field_ref m_current;

		std::shared_ptr<result_impl> m_result_impl;
	};

	// --------------------------------------------------------------------

	row_ref() = default;

	row_ref(row_handle rh, std::shared_ptr<result_impl> result_impl)
		: m_row(rh)
		, m_result_impl(result_impl)
	{
	}

	row_ref(const row_ref &) = default;
	row_ref &operator=(const row_ref &) = default;

	// --------------------------------------------------------------------

	const_field_iterator cbegin() const noexcept { return const_field_iterator(m_row, 0, m_result_impl); }
	const_field_iterator begin() const noexcept { return const_field_iterator(m_row, 0, m_result_impl); }
	const_field_iterator cend() const noexcept { return const_field_iterator(m_row, size(), m_result_impl); }
	const_field_iterator end() const noexcept { return const_field_iterator(m_row, size(), m_result_impl); }

	field_ref front() const noexcept { return field_ref(m_row, 0, m_result_impl); }
	field_ref back() const noexcept { return field_ref(m_row, size() - 1, m_result_impl); }

	size_t size() const noexcept;
	bool empty() const noexcept { return size() == 0; }

	field_ref operator[](size_t index) const noexcept { return field_ref(m_row, index, m_result_impl); }
	field_ref operator[](std::string_view name) const;

	// --------------------------------------------------------------------

	bool operator==(const row_ref &rhs) const { return m_row == rhs.m_row; }
	bool operator!=(const row_ref &rhs) const { return m_row != rhs.m_row; }

  private:
	row_handle m_row;
	std::shared_ptr<result_impl> m_result_impl;
};

// --------------------------------------------------------------------

class result
{
  public:
	// --------------------------------------------------------------------

	class const_row_iterator
	{
	  public:
		friend class view;

		using iterator_category = std::forward_iterator_tag;
		using value_type = const row_ref;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type *;
		using reference = value_type &;

		// const_row_iterator() = default;

		const_row_iterator(std::shared_ptr<result_impl> result_impl, category::iterator cat_iter)
			: m_iter(cat_iter)
			, m_current(*m_iter, result_impl)
			, m_result_impl(result_impl)
		{
		}

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
			++m_iter;
			m_current = { *m_iter, m_result_impl };
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
			return m_result_impl == rhs.m_result_impl and m_iter == rhs.m_iter;
		}

		bool operator!=(const const_row_iterator &rhs) const
		{
			return m_result_impl != rhs.m_result_impl or m_iter != rhs.m_iter;
		}

	  private:
		category::iterator m_iter;
		row_ref m_current;
		std::shared_ptr<result_impl> m_result_impl;
	};

	// --------------------------------------------------------------------

	result() = delete;
	result(result const &rhs) noexcept = default;
	result(result &&rhs) noexcept = default;
	result &operator=(result const &rhs) noexcept = default;
	result &operator=(result &&rhs) noexcept = default;

	result(category &&data, const std::string &query = "");

	row_ref one_row() const
	{
		if (size() != 1)
			throw std::runtime_error("Expected one row");
		return front();
	}

	field_ref one_field() const
	{
		if (size() != 1)
			throw std::runtime_error("Expected one row");

		expect_columns(1);

		return one_row().front();
	}

	// --------------------------------------------------------------------

	const_row_iterator begin() const noexcept;
	const_row_iterator cbegin() const noexcept;

	const_row_iterator end() const noexcept;
	const_row_iterator cend() const noexcept;

	row_ref front() const;
	row_ref back() const;

	size_t size() const noexcept;
	bool empty() const noexcept { return size() == 0; }

	size_t column_count() const;

  private:
	friend class transaction;
	friend class SelectStatement;

	result expect_columns(size_t cols) const
	{
		if (auto actual = column_count(); cols != actual)
			throw std::runtime_error("Unexpected number of columns");
		return *this;
	}

	std::shared_ptr<result_impl> m_impl;
};

// --------------------------------------------------------------------

class transaction final
{
  public:
	transaction(connection &conn);
	~transaction();

	transaction(const transaction &) = delete;
	transaction &operator=(const transaction &) = delete;

	result exec(const std::string &query);

	void commit();
	void rollback();

  private:
	connection &m_conn;
};

// --------------------------------------------------------------------

class connection final
{
  public:
	connection(datablock &db);
	~connection();

	friend class transaction;

  private:
	struct connection_impl *m_impl;
};

} // namespace cif::cql