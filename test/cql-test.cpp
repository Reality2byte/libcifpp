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

const char *kAuthors[] = {
	"Kleywegt, G.J.",
	"Bergfors, T.",
	"Senn, H.",
	"Le Motte, P.",
	"Gsell, B.",
	"Shudo, K.",
	"Jones, T.A.",

	"Banaszak, L.",
	"Winter, N.",
	"Xu, Z.",
	"Bernlohr, D.A.",
	"Cowan, S.W.",
	"Jones, T.A.",
	"Bergfors, T.",
	"Kleywegt, G.J.",
	"Jones, T.A.",
	"Cowan, S.W.",
	"Newcomer, M.E.",
	"Jones, T.A.",
	"Jones, T.A.",
	"Bergfors, T.",
	"Sedzik, J.",
	"Unge, T."
};

// Test simple SELECT
TEST_CASE("cql-1")
{
	cif::file f(gTestDir / ".." / "examples" / "1cbs.cif.gz");
	auto &db = f.front();
	db.set_validator(&cif::validator_factory::instance().get("mmcif_pdbx.dic"));

	cif::cql::connection connection(db);
	cif::cql::transaction tx(connection);

	auto r = tx.exec("SELECT name, ordinal FROM citation_author WHERE citation_id = 'primary';");
	CHECK(r.size() == 7);

	for (size_t ix = 0; auto row : r)
	{
		REQUIRE(ix < (sizeof(kAuthors) / sizeof(char *)));

		CHECK(row[0].as<std::string>() == kAuthors[ix]);
		CHECK(row[1].as<size_t>() == ix + 1);

		CHECK(row["name"].as<std::string>() == kAuthors[ix]);
		CHECK(row["ordinal"].as<size_t>() == ix + 1);

		++ix;
	}

	r = tx.exec("SELECT ordinal, name FROM citation_author WHERE citation_id = 'primary';");
	CHECK(r.size() == 7);

	for (size_t ix = 0; auto row : r)
	{
		REQUIRE(ix < (sizeof(kAuthors) / sizeof(char *)));

		CHECK(row[1].as<std::string>() == kAuthors[ix]);
		CHECK(row[0].as<size_t>() == ix + 1);

		CHECK(row["name"].as<std::string>() == kAuthors[ix]);
		CHECK(row["ordinal"].as<size_t>() == ix + 1);

		++ix;
	}

	r = tx.exec("SELECT * FROM citation_author WHERE citation_id = 'primary';");
	CHECK(r.size() == 7);

	for (int ix = 0; auto row : r)
	{
		REQUIRE(static_cast<size_t>(ix) < (sizeof(kAuthors) / sizeof(char *)));

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
					CHECK(fld.as<std::string>() == kAuthors[ix]);
					break;
				case 2:
					CHECK(fld.name() == "ordinal");
					CHECK(fld.as<int>() == ix + 1);
					break;
				default:
					REQUIRE(false);
					break;
			}
		}

		CHECK(row["name"].as<std::string>() == kAuthors[ix]);
		CHECK(row["ordinal"].as<int>() == ix + 1);
		CHECK(row["citation_id"].as<std::string>() == "primary");

		++ix;
	}
}

// Test SELECT AS
TEST_CASE("cql-2")
{
	cif::file f(gTestDir / ".." / "examples" / "1cbs.cif.gz");
	auto &db = f.front();
	db.set_validator(&cif::validator_factory::instance().get("mmcif_pdbx.dic"));

	cif::cql::connection connection(db);
	cif::cql::transaction tx(connection);

	auto r = tx.exec("SELECT name AS v1, ordinal AS v2 FROM citation_author WHERE citation_id = 'primary';");
	CHECK(r.size() == 7);

	for (size_t ix = 0; auto row : r)
	{
		REQUIRE(ix < (sizeof(kAuthors) / sizeof(char *)));

		CHECK(row[0].as<std::string>() == kAuthors[ix]);
		CHECK(row[1].as<size_t>() == ix + 1);

		CHECK(row["v1"].as<std::string>() == kAuthors[ix]);
		CHECK(row["v2"].as<size_t>() == ix + 1);

		++ix;
	}
}

TEST_CASE("cql-3")
{
	cif::file f(gTestDir / ".." / "examples" / "1cbs.cif.gz");
	auto &db = f.front();
	db.set_validator(&cif::validator_factory::instance().get("mmcif_pdbx.dic"));

	cif::cql::connection connection(db);
	cif::cql::transaction tx(connection);

	auto r = tx.exec("SELECT name FROM citation_author WHERE ordinal = 10").one_field();
	CHECK(r.as<std::string>() == kAuthors[9]);
}

TEST_CASE("cql-4")
{
	cif::file f(gTestDir / ".." / "examples" / "1cbs.cif.gz");
	auto &db = f.front();
	db.set_validator(&cif::validator_factory::instance().get("mmcif_pdbx.dic"));

	cif::cql::connection connection(db);
	cif::cql::transaction tx(connection);

	auto r = tx.exec("SELECT name FROM citation_author WHERE ordinal BETWEEN 10 AND 15");
	REQUIRE(r.size() == 6);
}

TEST_CASE("cql-5")
{
	cif::file f(gTestDir / ".." / "examples" / "1cbs.cif.gz");
	auto &db = f.front();
	db.set_validator(&cif::validator_factory::instance().get("mmcif_pdbx.dic"));

	cif::cql::connection connection(db);
	cif::cql::transaction tx(connection);

	auto r = tx.exec("SELECT (SELECT year FROM citation WHERE id = citation_id) AS jaar FROM citation_author WHERE ordinal IS 23").one_field();
	CHECK(r.name() == "jaar");
	CHECK(r.as<int>() == 1988);
}