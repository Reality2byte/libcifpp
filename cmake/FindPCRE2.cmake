# SPDX-License-Identifier: BSD-2-Clause
# 
# Copyright (c) 2026 NKI/AVL, Netherlands Cancer Institute
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# The problem is, find_package(PCRE2) does not work
# and using pkg-config results in linking to a shared library
# causing all kinds of trouble later on

find_path(PCRE2_INCLUDEDIR NAMES pcre2.h HINTS "C:/Program Files (x86)/PCRE2/include" REQUIRED)
find_library(PCRE2_LIBRARY NAMES pcre2-8-static libpcre2-8.a HINTS "C:/Program Files (x86)/PCRE2/lib" REQUIRED)

add_library(pcre2-8 IMPORTED STATIC)
target_include_directories(pcre2-8 INTERFACE ${PCRE2_INCLUDEDIR})
target_compile_definitions(pcre2-8 INTERFACE PCRE2_STATIC)
set_target_properties(pcre2-8 PROPERTIES IMPORTED_LOCATION ${PCRE2_LIBRARY})
set_target_properties(pcre2-8 PROPERTIES IMPORTED_IMPLIB ${PCRE2_LIBRARY})
