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

#include "cif++/matrix.hpp"
#include "test-main.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cif++.hpp>

TEST_CASE("m1")
{
	cif::matrix3x3<int> m = cif::identity_matrix<int>(3);

	CHECK(cif::determinant(m) == 1);
}

TEST_CASE("m2")
{
	cif::matrix4x4<int> m = cif::identity_matrix<int>(4);

	cif::sub_matrix<cif::matrix4x4<int>> ms(m, 1, 1);
	CHECK(ms == cif::identity_matrix<int>(3));
}

TEST_CASE("m3")
{
	cif::matrix4x4<int> m{
		{ 1, 2, 3, 4,      //
			5, 6, 7, 8,    //
			9, 10, 11, 12, //
			13, 14, 15, 16 }
	};
	cif::sub_matrix<cif::matrix4x4<int>> ms(m, 1, 1);

	cif::matrix3x3<int> t{
		{ 1, 3, 4, 9, 11, 12, 13, 15, 16 }
	};

	CHECK(ms == t);
}

TEST_CASE("m4")
{
	cif::matrix4x4<int> m{
		{
			-2,
			3,
			1,
			0,
			4,
			1,
			-3,
			2,
			0,
			-1,
			2,
			5,
			3,
			2,
			0,
			-4,
		}
	};

	// std::cout << m << "\n\n";

	// std::cout << cif::matrix3x3<int>(cif::sub_matrix<decltype(m)>(m, 0, 0)) << "\n\n";
	// std::cout << cif::matrix3x3<int>(cif::sub_matrix<decltype(m)>(m, 0, 1)) << "\n\n";
	// std::cout << cif::matrix3x3<int>(cif::sub_matrix<decltype(m)>(m, 0, 2)) << "\n\n";
	// std::cout << cif::matrix3x3<int>(cif::sub_matrix<decltype(m)>(m, 0, 3)) << "\n\n";

	// std::cout << cif::determinant(cif::matrix3x3<int>(cif::sub_matrix<decltype(m)>(m, 0, 0))) << "\n\n";
	// std::cout << cif::determinant(cif::matrix3x3<int>(cif::sub_matrix<decltype(m)>(m, 0, 1))) << "\n\n";
	// std::cout << cif::determinant(cif::matrix3x3<int>(cif::sub_matrix<decltype(m)>(m, 0, 2))) << "\n\n";
	// std::cout << cif::determinant(cif::matrix3x3<int>(cif::sub_matrix<decltype(m)>(m, 0, 3))) << "\n\n";

	CHECK(cif::determinant(m) == 332);
}


