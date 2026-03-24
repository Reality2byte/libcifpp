/*-
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Copyright (c) 2026 NKI/AVL, Netherlands Cancer Institute
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

#include <cif++/cif++.hpp>

class dummy_parser : public cif::sac_parser
{
  public:
	dummy_parser(std::istream &is)
		: sac_parser(is)
	{
	}

	void produce_datablock(std::string_view name) override
	{
	}

	void produce_category(std::string_view name) override
	{
	}

	void produce_row() override
	{
	}

	void produce_item(std::string_view category, std::string_view item, std::string_view value) override
	{
	}
};


int main()
{
	cif::gzio::ifstream in("/srv/data/pdb/mmCIF/gl/8glv.cif.gz");

	dummy_parser parser(in);
	parser.parse_file();

	// cif::file f("/srv/data/pdb/mmCIF/gl/8glv.cif.gz");

	return 0;
}