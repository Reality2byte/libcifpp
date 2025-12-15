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
#include <vector>

// --------------------------------------------------------------------

namespace cif::cql
{

class result;
class row;
class transaction;
class view;
class connection;

// --------------------------------------------------------------------

struct column
{
	std::string name;
	size_t index;
};

using column_list = std::vector<column>;

// --------------------------------------------------------------------

class field_ref final
{
  public:
	std::string_view name() const &
	{
		return m_col->name;
	}

	constexpr size_t num() const noexcept
	{
		return m_col->index;
	}

	std::string_view text() const &
	{
		return m_row[m_col->index].text();
	}

	/** Return the contents of this item as type @tparam T */
	template <typename T = std::string>
	auto as() const -> T
	{
		return m_row[m_col->index].as<T>();
	}

	/** Return the contents of this item as type @tparam T or, if not
	 * set, use @a dv as the default value.
	 */
	template <typename T>
	auto value_or(const T &dv) const
	{
		return m_row[m_col->index].value_or(dv);
	}

	field_ref(row_handle rh, column_list::const_iterator col)
		: m_row(rh)
		, m_col(col)
	{
	}

	field_ref(const field_ref &) = default;
	field_ref(field_ref &&) = default;

	field_ref &operator=(const field_ref &) = default;
	field_ref &operator=(field_ref &&) = default;

  private:
	row_handle m_row;
	column_list::const_iterator m_col;
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
				m_current = field_ref(m_row, m_col);
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

		const_field_iterator(const row_handle &row, column_list::const_iterator col)
			: m_row(row)
			, m_col(col)
			, m_current(m_row, m_col)
		{
		}

		row_handle m_row;
		column_list::const_iterator m_col;
		field_ref m_current;
	};

	// --------------------------------------------------------------------

	row_ref() = default;

	row_ref(row_handle rh, const column_list &cols)
		: m_row(rh)
		, m_cols(&cols)
	{
	}

	row_ref(row_ref r, const column_list &cols)
		: m_row(r.m_row)
		, m_cols(&cols)
	{
	}

	row_ref(const row_ref &) = default;
	row_ref &operator=(const row_ref &) = default;

	// --------------------------------------------------------------------

	const_field_iterator cbegin() const noexcept { return const_field_iterator(m_row, m_cols->cbegin()); }
	const_field_iterator begin() const noexcept { return const_field_iterator(m_row, m_cols->cbegin()); }
	const_field_iterator cend() const noexcept { return const_field_iterator(m_row, m_cols->cend()); }
	const_field_iterator end() const noexcept { return const_field_iterator(m_row, m_cols->cend()); }

	field_ref front() const noexcept { return field_ref(m_row, m_cols->cbegin()); }
	field_ref back() const noexcept { return field_ref(m_row, m_cols->cend()); }

	size_t size() const noexcept { return m_cols->size(); }
	bool empty() const noexcept { return m_cols->empty(); }

	field_ref operator[](size_t index) const noexcept;
	field_ref operator[](std::string_view name) const noexcept;

	// --------------------------------------------------------------------

	bool operator==(const row_ref &rhs) const { return m_row == rhs.m_row and m_cols == rhs.m_cols; }
	bool operator!=(const row_ref &rhs) const { return m_row != rhs.m_row or m_cols != rhs.m_cols; }

  private:
	row_handle m_row;
	const column_list *m_cols = nullptr;
};

// // --------------------------------------------------------------------

// class view : public std::enable_shared_from_this<view>
// {
//   public:
// 	virtual ~view() = default;


// 	// --------------------------------------------------------------------

// 	const_row_iterator begin() const noexcept { return const_row_iterator(*this, 0, at(0)); }
// 	const_row_iterator cbegin() const noexcept { return const_row_iterator(*this, 0, at(0)); }

// 	const_row_iterator end() const noexcept { return const_row_iterator(*this, size(), row_ref{}); }
// 	const_row_iterator cend() const noexcept { return const_row_iterator(*this, size(), row_ref{}); }

// 	virtual row_ref front() const noexcept = 0;
// 	virtual row_ref back() const noexcept = 0;

// 	virtual size_t size() const noexcept = 0;
// 	bool empty() const noexcept { return size() == 0; }

// 	virtual row_ref at(size_t index) const = 0;

// 	// --------------------------------------------------------------------

// 	std::vector<std::string> columns() const
// 	{
// 		std::vector<std::string> result;
// 		for (const auto &[name, ignore] : m_columns)
// 			result.emplace_back(name);
// 		return result;
// 	}

//   protected:
// 	friend class const_row_iterator;

// 	view(const column_list &cols)
// 		: m_columns(cols)
// 	{
// 	}

// 	view(column_list &&cols)
// 		: m_columns(std::forward<column_list>(cols))
// 	{
// 	}

// 	column_list m_columns;
// };

// // --------------------------------------------------------------------

// class simple_view : public view
// {
//   public:
// 	simple_view(const category &cat)
// 		: view(get_column_list_for_category(cat))
// 		, m_cat(cat)
// 	{
// 	}

// 	simple_view(const simple_view &) = default;
// 	simple_view(simple_view &&) = default;

// 	virtual size_t size() const noexcept override { return m_cat.size(); }

// 	virtual row_ref front() const noexcept override;
// 	virtual row_ref back() const noexcept override;

// 	virtual row_ref at(size_t index) const override;

//   protected:

// 	static column_list get_column_list_for_category(const category &cat);

//   const category &m_cat;
// };

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
			if (m_index < m_data.size())
				m_current = m_data.at(m_index);
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
			return &m_data == &rhs.m_data and m_index == rhs.m_index;
		}

		bool operator!=(const const_row_iterator &rhs) const
		{
			return &m_data != &rhs.m_data or m_index != rhs.m_index;
		}

	  private:
		const_row_iterator(const view &result, size_t index, row_ref current);

		// const view &m_data;
		// size_t m_index = 0;
		// row_ref m_current;
	};

	// --------------------------------------------------------------------

	result() = delete;
	result(result const &rhs) noexcept = default;
	result(result &&rhs) noexcept = default;
	result &operator=(result const &rhs) noexcept = default;
	result &operator=(result &&rhs) noexcept = default;

	result(view &vw, const std::string &query = "");

	row_ref one_row() const;
	field_ref one_field() const;

	// --------------------------------------------------------------------

	const_row_iterator begin() const noexcept;
	const_row_iterator cbegin() const noexcept;

	const_row_iterator end() const noexcept;
	const_row_iterator cend() const noexcept;

	row_ref front() const noexcept;
	row_ref back() const noexcept;

	size_t size() const noexcept;
	bool empty() const noexcept;

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

	row_ref at(size_t index) const;

	std::string m_query;
	std::shared_ptr<view> m_view;
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
	struct transaction_impl *m_impl;
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