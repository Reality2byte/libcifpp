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

#include "test-main.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cif++.hpp>
#include <cif++/cql/transaction.hpp>
#include <stdexcept>

// --------------------------------------------------------------------

cif::file operator""_cf(const char *text, std::size_t length)
{
	struct membuf : public std::streambuf
	{
		membuf(char *text, std::size_t length)
		{
			this->setg(text, text, text + length);
		}
	} buffer(const_cast<char *>(text), length);

	std::istream is(&buffer);
	return cif::file(is);
}

// --------------------------------------------------------------------

TEST_CASE("cql-1")
{
	cif::file f(gTestDir / ".." / "examples" / "1cbs.cif.gz");
	auto &db = f.front();

	cif::cql::transaction tx(db);

	// CHECK(tx.exec("SELECT COUNT(*) FROM entry").one_field().as<int>() == 1);
	// CHECK(tx.exec("SELECT COUNT(*) FROM entry WHERE id = '1CBS'").one_field().as<int>() == 1);
	// CHECK(tx.exec("SELECT COUNT(*) FROM entry WHERE id = 'XXXX'").one_field().as<int>() == 0);

	// CHECK(tx.exec("SELECT COUNT(*) FROM citation").one_field().as<int>() == 4);
	// CHECK(tx.exec("SELECT COUNT(page_last) FROM citation").one_field().as<int>() == 1);

	const char *kPrimaryAuthors[] = {
		"Kleywegt, G.J.",
		"Bergfors, T.",
		"Senn, H.",
		"Le Motte, P.",
		"Gsell, B.",
		"Shudo, K.",
		"Jones, T.A."
	};

	auto r = tx.exec("SELECT name, ordinal FROM citation_author WHERE citation_id = 'primary';");
	CHECK(r.size() == 7);

	for (size_t ix = 0; auto row : r)
	{
		REQUIRE(ix < (sizeof(kPrimaryAuthors) / sizeof(char *)));

		CHECK(row[0].as<std::string>() == kPrimaryAuthors[ix++]);
		CHECK(row[1].as<size_t>() == ix);

		// CHECK(row["name"].as<std::string>() == kPrimaryAuthors[ix++]);
		// CHECK(row["ordinal"].as<int>() == ix);
	}

	r = tx.exec("SELECT ordinal, name FROM citation_author WHERE citation_id = 'primary';");
	CHECK(r.size() == 7);

	for (size_t ix = 0; auto row : r)
	{
		REQUIRE(ix < (sizeof(kPrimaryAuthors) / sizeof(char *)));

		CHECK(row[1].as<std::string>() == kPrimaryAuthors[ix++]);
		CHECK(row[0].as<size_t>() == ix);

		// CHECK(row["name"].as<std::string>() == kPrimaryAuthors[ix++]);
		// CHECK(row["ordinal"].as<int>() == ix);
	}

	r = tx.exec("SELECT * FROM citation_author WHERE citation_id = 'primary';");
	CHECK(r.size() == 7);

	for (size_t ix = 0; auto row : r)
	{
		REQUIRE(ix < (sizeof(kPrimaryAuthors) / sizeof(char *)));

		for (auto fld : row)
		{
			switch (fld.num())
			{
				case 0:
					CHECK(fld.name() == "citation_id");
					CHECK(fld.as<std::string>() == "primary");
					break;
				case 1:
					CHECK(fld.name() == "name");
					CHECK(fld.as<std::string>() == kPrimaryAuthors[ix]);
					break;
				case 2:
					CHECK(fld.name() == "ordinal");
					CHECK(fld.as<int>() == ix);
					break;
				default:
					REQUIRE(false);
					break;
			}
		}

		++ix;

		// CHECK(row[0].as<std::string>() == kPrimaryAuthors[ix++]);
		// CHECK(row[1].as<int>() == ix);

		CHECK(row["name"].as<std::string>() == kPrimaryAuthors[ix++]);
		CHECK(row["ordinal"].as<size_t>() == ix);
	}

	// CHECK(tx.query_value<int>("SELECT COUNT(*) FROM citation_author WHERE citation_id = 'primary';") == 7);

	// for (size_t ix = 0; auto row : r)
	// {
	// 	REQUIRE(ix < (sizeof(kPrimaryAuthors) / sizeof(char*)));
	// 	// CHECK(row["name"].as<std::string>() == kPrimaryAuthors[ix++]);
	// 	// CHECK(row["ordinal"].as<int>() == ix);
	// }

	// for (size_t ix = 0; const auto &[name, ordinal] : tx.stream<std::string, int>("SELECT name FROM citation_author WHERE citation_id = 'primary'"))
	// {
	// 	REQUIRE(ix < (sizeof(kPrimaryAuthors) / sizeof(char*)));
	// 	CHECK(name == kPrimaryAuthors[ix++]);
	// 	CHECK(ordinal == ix);
	// }
}