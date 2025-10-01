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

// --------------------------------------------------------------------

namespace cif::cql
{

class result;
class row;
class transaction;

// --------------------------------------------------------------------

class field
{
  public:

	/** Return the contents of this item as type @tparam T */
	template <typename T = std::string>
	auto as() const -> T
	{
		return m_item.as<T>();
	}

	/** Return the contents of this item as type @tparam T or, if not
	 * set, use @a dv as the default value.
	 */
	template <typename T>
	auto value_or(const T &dv) const
	{
		return m_item.value_or(dv);
	}    

  private:
    cif::item_handle m_item;
};

// --------------------------------------------------------------------

class row
{
  public:



};

// --------------------------------------------------------------------

class result
{
  public:

    row &one_row();


  private:
    friend class transaction;

    result();

};


// --------------------------------------------------------------------

class transaction
{
  public:
    transaction(const datablock &db);

    result exec(std::string_view query);
};

}