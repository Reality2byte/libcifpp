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

#include "cif++/cql/transaction.hpp"

#include "../sqlite3.h"
#include "cif++/category.hpp"
#include "cif++/condition.hpp"
#include "cif++/datablock.hpp"
#include "cif++/iterator.hpp"
#include "cif++/row.hpp"
#include "cif++/text.hpp"
#include "cif++/validate.hpp"

#include <cstdint>
#include <exception>
#include <iomanip>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace cif::cql
{

struct result_impl
{
	category m_cat;
	std::string m_query;
};

// --------------------------------------------------------------------

size_t row_ref::size() const noexcept
{
	return m_result_impl->m_cat.get_item_count();
}

field_ref row_ref::operator[](std::string_view name) const
{
	for (int ix = 0; auto &item : m_result_impl->m_cat.get_items())
	{
		if (iequals(item, name))
			return { m_row, ix, m_result_impl };
		++ix;
	}
	throw std::runtime_error("Column not defined in query result");
}

// --------------------------------------------------------------------

result::result(category &&cat, const std::string &query)
	: m_impl(new result_impl{ std::forward<category>(cat), query })
{
}

size_t result::size() const noexcept
{
	return m_impl->m_cat.size();
}

size_t result::column_count() const
{
	return m_impl->m_cat.get_item_count();
}

result::iterator result::begin() const noexcept
{
	return { m_impl, m_impl->m_cat.begin() };
}

result::iterator result::cbegin() const noexcept
{
	return { m_impl, m_impl->m_cat.cbegin() };
}

result::iterator result::end() const noexcept
{
	return { m_impl, m_impl->m_cat.end() };
}

result::iterator result::cend() const noexcept
{
	return { m_impl, m_impl->m_cat.cend() };
}

row_ref result::front() const
{
	return { m_impl->m_cat.front(), m_impl };
}

row_ref result::back() const
{
	return { m_impl->m_cat.back(), m_impl };
}

// // --------------------------------------------------------------------

// inline std::string to_hex(uint32_t i)
// {
// 	char s[sizeof(i) * 2 + 3];
// 	char *p = s + sizeof(s);
// 	*--p = 0;

// 	const char kHexChars[] = "0123456789abcdef";

// 	while (i)
// 	{
// 		*--p = kHexChars[i & 0x0F];
// 		i >>= 4;
// 	}

// 	*--p = 'x';
// 	*--p = '0';

// 	return p;
// }

// // --------------------------------------------------------------------

// class Statement;
// using StatementPtr = std::shared_ptr<Statement>;

// // -----------------------------------------------------------------------

// class Statement
// {
//   public:
// 	Statement(const Statement &) = delete;
// 	Statement &operator=(const Statement &) = delete;

// 	virtual ~Statement() = default;

// 	virtual std::shared_ptr<view> Execute() = 0;

//   protected:
// 	Statement() = default;
// };

// // --------------------------------------------------------------------

// class ValueExpression : public std::enable_shared_from_this<ValueExpression>
// {
//   public:
// 	ValueExpression(const ValueExpression &) = delete;
// 	ValueExpression &operator=(const ValueExpression &) = delete;

// 	virtual ~ValueExpression() = default;

// 	virtual item Execute(const std::vector<std::pair<std::string, row_handle>> &data) = 0;

//   protected:
// 	ValueExpression() = default;
// };

// // --------------------------------------------------------------------

// class DerivedColumn
// {
//   public:
// 	DerivedColumn(ValueExpression *expr, const std::string &name)
// 		: mExpr(expr->shared_from_this())
// 		, mName(name)
// 	{
// 	}

// 	item Execute(const std::vector<std::pair<std::string, row_handle>> &data)
// 	{
// 		return mExpr->Execute(data);
// 	}

// 	std::string Name() const
// 	{
// 		return mName;
// 	}

//   private:
// 	std::shared_ptr<ValueExpression> mExpr;
// 	std::string mName;
// };

// // --------------------------------------------------------------------

// class SimpleColumnValueExpression : public ValueExpression
// {
//   public:
// 	SimpleColumnValueExpression(const std::string &column)
// 		: mColumn(column)
// 	{
// 	}

// 	item Execute(const std::vector<std::pair<std::string, row_handle>> &data) override;

//   private:
// 	std::string mColumn;
// };

// // // -----------------------------------------------------------------------

// // class StatementList : public Statement
// // {
// //   public:
// // 	StatementList() {}

// // 	void Add(StatementPtr stmt)
// // 	{
// // 		mStatements.emplace_back(stmt);
// // 	}

// // 	virtual std::shared_ptr<view> Execute() override
// // 	{
// // 		for (auto stmt : mStatements)
// // 			stmt->Execute();
// // 	}

// //   private:
// // 	std::vector<StatementPtr> mStatements;
// // };

// // --------------------------------------------------------------------

// class FromStatement : public Statement
// {
//   public:
// 	FromStatement(cif::category &cat)
// 		: m_category(cat)
// 	{
// 	}

// 	virtual std::shared_ptr<view> Execute() override
// 	{
// 		return std::make_shared<simple_view>(m_category);
// 	}

//   private:
// 	cif::category &m_category;
// };

// // -----------------------------------------------------------------------

// class SelectStatement : public Statement
// {
//   public:
// 	SelectStatement(std::shared_ptr<view> view, const column_list &cols)
// 		: m_view(view)
// 		, m_columns(cols)
// 	{
// 	}

// 	SelectStatement(std::shared_ptr<view> view, column_list &&cols)
// 		: m_view(view)
// 		, m_columns(std::forward<column_list>(cols))
// 	{
// 	}

// 	virtual std::shared_ptr<view> Execute() override
// 	{
// 		return std::make_shared<select_view>(m_view, m_columns);
// 	}

// 	std::shared_ptr<view> m_view;
// 	column_list m_columns;
// };

// // // -----------------------------------------------------------------------

// // class DeleteStatement : public Statement
// // {
// //   public:
// // 	DeleteStatement(cif::category &category, cif::condition &&where)
// // 		: mCategory(category)
// // 		, mWhere(std::move(where))
// // 	{
// // 	}

// // 	virtual void Execute()
// // 	{
// // 		std::vector<cif::row_handle> remove;

// // 		mWhere.prepare(mCategory);

// // 		for (auto r : mCategory)
// // 		{
// // 			if (mWhere(r))
// // 				remove.insert(remove.end(), r);
// // 		}

// // 		for (auto r : remove)
// // 			mCategory.erase(r);

// // 		std::cout << "Number of removed rows " << remove.size() << '\n';
// // 	}

// //   private:
// // 	cif::category &mCategory;
// // 	cif::condition mWhere;
// // };

// // // -----------------------------------------------------------------------

// // class UpdateStatement : public Statement
// // {
// //   public:
// // 	UpdateStatement(cif::category &category, std::vector<std::pair<std::string, std::string>> &&itemValuePairs, cif::condition &&where)
// // 		: mCategory(category)
// // 		, mItemValuePairs(std::move(itemValuePairs))
// // 		, mWhere(std::move(where))
// // 	{
// // 	}

// // 	virtual void Execute()
// // 	{
// // 		size_t updated = 0;

// // 		mWhere.prepare(mCategory);

// // 		for (auto r : mCategory)
// // 		{
// // 			if (mWhere(r))
// // 			{
// // 				for (auto iv : mItemValuePairs)
// // 					r[iv.first] = iv.second;

// // 				++updated;
// // 			}
// // 		}

// // 		std::cout << "Number of updated rows: " << updated << '\n';
// // 	}

// //   private:
// // 	cif::category &mCategory;
// // 	std::vector<std::pair<std::string, std::string>> mItemValuePairs;
// // 	cif::condition mWhere;
// // };

// class Parser
// {
//   public:
// 	Parser(cif::datablock &db)
// 		: mDb(db)
// 	{
// 		if (mDb.empty())
// 			throw std::runtime_error("Empty datablock");
// 	}

// 	StatementPtr Parse(std::streambuf *is);

//   private:
// 	enum class Token
// 	{
// 		EOLN,

// 		UNDEF,

// 		BRACE_OPEN,
// 		BRACE_CLOSE,

// 		DOT,
// 		COMMA,
// 		COLON,
// 		SEMICOLON,
// 		ASTERISK,

// 		EQ,
// 		LT,
// 		LE,
// 		GT,
// 		GE,
// 		NE,

// 		STRING,
// 		INTEGER,
// 		NUMBER,

// 		IDENT,

// 		SELECT,
// 		DISTINCT,
// 		ALL,
// 		FROM,
// 		AS,
// 		BETWEEN,
// 		UPDATE,
// 		SET,
// 		WHERE,
// 		AND,
// 		OR,
// 		NOT,
// 		INSERT,
// 		DELETE,
// 		INTO,
// 		VALUES,
// 		IS,
// 		_NULL
// 	};

// 	std::map<std::string, Token, iless> kKeyWords{
// 		{ "SELECT", Token::SELECT },
// 		{ "DISTINCT", Token::DISTINCT },
// 		{ "ALL", Token::ALL },
// 		{ "FROM", Token::FROM },
// 		{ "AS", Token::AS },
// 		{ "BETWEEN", Token::BETWEEN },
// 		{ "UPDATE", Token::UPDATE },
// 		{ "SET", Token::SET },
// 		{ "WHERE", Token::WHERE },
// 		{ "AND", Token::AND },
// 		{ "OR", Token::OR },
// 		{ "NOT", Token::NOT },
// 		{ "INSERT", Token::INSERT },
// 		{ "DELETE", Token::DELETE },
// 		{ "INTO", Token::INTO },
// 		{ "VALUES", Token::VALUES },
// 		{ "IS", Token::IS },
// 		{ "NULL", Token::_NULL }
// 	};

// 	std::string Describe(Token token)
// 	{
// 		switch (token)
// 		{
// 			case Token::EOLN: return "<EOLN>";
// 			case Token::UNDEF: return "<UNDEFINED>";
// 			case Token::BRACE_OPEN: return "'('";
// 			case Token::BRACE_CLOSE: return "')'";

// 			case Token::DOT: return "'.'";
// 			case Token::COMMA: return "','";
// 			case Token::COLON: return "':'";
// 			case Token::SEMICOLON: return "';'";
// 			case Token::ASTERISK: return "'*'";

// 			case Token::EQ: return "'='";
// 			case Token::LT: return "'<'";
// 			case Token::LE: return "'<='";
// 			case Token::GT: return "'>'";
// 			case Token::GE: return "'>='";
// 			case Token::NE: return "'<>'";

// 			case Token::STRING: return "string";
// 			case Token::INTEGER: return "integer";
// 			case Token::NUMBER: return "number";
// 			case Token::IDENT: return "identifier";

// 			case Token::SELECT: return "SELECT";
// 			case Token::DISTINCT: return "DISTINCT";
// 			case Token::FROM: return "FROM";
// 			case Token::AS: return "AS";
// 			case Token::UPDATE: return "UPDATE";
// 			case Token::SET: return "SET";
// 			case Token::WHERE: return "WHERE";
// 			case Token::AND: return "AND";
// 			case Token::OR: return "OR";
// 			case Token::NOT: return "NOT";
// 			case Token::INSERT: return "INSERT";
// 			case Token::DELETE: return "DELETE";
// 			case Token::INTO: return "INTO";
// 			case Token::VALUES: return "VALUES";
// 			case Token::IS: return "IS";
// 			case Token::_NULL: return "NULL";

// 			default: assert(false); return "unknown token";
// 		}
// 	}

// 	char GetNextChar();
// 	Token GetNextToken();

// 	void Retract();
// 	void Match(Token token);

// 	// parser rules
// 	StatementPtr ParseStatement();
// 	StatementPtr ParseSelect();
// 	StatementPtr ParseDelete();
// 	StatementPtr ParseUpdate();
// 	std::vector<std::pair<std::string, std::string>> ParseItemList();

// 	cif::condition ParseWhereClause(cif::category &cat, const column_list &columns);
// 	cif::condition ParseNotWhereClause(cif::category &cat, const column_list &columns);

// 	cif::datablock &mDb;
// 	std::streambuf *mIs;
// 	Token mLookahead;
// 	std::stack<char> mBuffer;
// 	std::string mToken;
// 	double mTokenFloat;
// 	int64_t mTokenInteger;
// };

// // -----------------------------------------------------------------------

// char Parser::GetNextChar()
// {
// 	char result = 0;

// 	if (not mBuffer.empty()) // if buffer is not empty we already did all the validity checks
// 	{
// 		result = mBuffer.top();
// 		mBuffer.pop();
// 	}
// 	else
// 	{
// 		int c = mIs->sbumpc();
// 		result = c == std::streambuf::traits_type::eof() ? 0 : static_cast<char>(c);
// 	}

// 	mToken += result;

// 	return result;
// }

// void Parser::Retract()
// {
// 	assert(not mToken.empty());
// 	mBuffer.push(mToken.back());
// 	mToken.pop_back();
// }

// constexpr bool is_name_start_char(char ch)
// {
// 	return (ch >= 'A' and ch <= 'Z') or
// 	       ch == '_' or
// 	       (ch >= 'a' and ch <= 'z');
// }

// constexpr bool is_name_char(char ch)
// {
// 	return (ch >= '0' and ch <= '9') or is_name_start_char(ch);
// }

// Parser::Token Parser::GetNextToken()
// {
// 	enum class State
// 	{
// 		Start,
// 		Negative,
// 		Zero,
// 		NegativeZero,
// 		Number,
// 		NumberFraction,
// 		NumberExpSign,
// 		NumberExpDigit1,
// 		NumberExpDigit2,
// 		Literal,
// 		String,
// 		Escape,
// 		EscapeHex1,
// 		EscapeHex2,
// 		// EscapeHex3,
// 		// EscapeHex4,

// 		Less,
// 		Greater
// 	} state = State::Start;

// 	Token token = Token::UNDEF;
// 	double fraction = 1.0, exponent = 1;
// 	bool negative = false, negativeExp = false;

// 	char hx = 0;

// 	mToken.clear();

// 	while (token == Token::UNDEF)
// 	{
// 		auto ch = GetNextChar();

// 		switch (state)
// 		{
// 			case State::Start:
// 				switch (ch)
// 				{
// 					case 0:
// 						token = Token::EOLN;
// 						break;
// 					case '(':
// 						token = Token::BRACE_OPEN;
// 						break;
// 					case ')':
// 						token = Token::BRACE_CLOSE;
// 						break;
// 					// case '[':
// 					// 	token = Token::LeftBracket;
// 					// 	break;
// 					// case ']':
// 					// 	token = Token::RightBracket;
// 					// 	break;
// 					case '.':
// 						token = Token::DOT;
// 						break;
// 					case ',':
// 						token = Token::COMMA;
// 						break;
// 					case ':':
// 						token = Token::COLON;
// 						break;
// 					case ';':
// 						token = Token::SEMICOLON;
// 						break;
// 					case '*':
// 						token = Token::ASTERISK;
// 						break;
// 					case '=':
// 						token = Token::EQ;
// 						break;
// 					case '<':
// 						state = State::Less;
// 						break;
// 					case '>':
// 						state = State::Greater;
// 						break;
// 					case ' ':
// 					case '\n':
// 					case '\r':
// 					case '\t':
// 						mToken.clear();
// 						break;
// 					case '\'':
// 						mToken.pop_back();
// 						state = State::String;
// 						break;
// 					case '-':
// 						state = State::Negative;
// 						break;
// 					default:
// 						if (ch == '0')
// 						{
// 							state = State::Zero;
// 							mTokenInteger = 0;
// 						}
// 						else if (ch >= '1' and ch <= '9')
// 						{
// 							mTokenInteger = ch - '0';
// 							state = State::Number;
// 						}
// 						else if (is_name_start_char(ch))
// 							state = State::Literal;
// 						else
// 							throw std::runtime_error("invalid character (" + to_hex(ch) + "/'" + (isprint(ch) ? static_cast<char>(ch) : '.') + "') in command");
// 				}
// 				break;

// 			case State::Less:
// 				if (ch == '=')
// 					token = Token::LE;
// 				else if (ch == '>')
// 					token = Token::NE;
// 				else
// 				{
// 					Retract();
// 					token = Token::LT;
// 				}
// 				break;

// 			case State::Greater:
// 				if (ch == '=')
// 					token = Token::GE;
// 				else
// 				{
// 					Retract();
// 					token = Token::GT;
// 				}
// 				break;

// 			case State::Negative:
// 				if (ch == '0')
// 					state = State::NegativeZero;
// 				else if (ch >= '1' and ch <= '9')
// 				{
// 					state = State::Number;
// 					mTokenInteger = ch - '0';
// 					negative = true;
// 				}
// 				else
// 					throw std::runtime_error("invalid character '-' in command");
// 				break;

// 			case State::NegativeZero:
// 				if (ch >= '0' or ch <= '9')
// 					throw std::runtime_error("invalid number in command, should not start with zero");
// 				token = Token::NUMBER;
// 				break;

// 			case State::Zero:
// 				if (ch >= '0' or ch <= '9')
// 					throw std::runtime_error("invalid number in command, should not start with zero");
// 				token = Token::NUMBER;
// 				break;

// 			case State::Number:
// 				if (ch >= '0' and ch <= '9')
// 					mTokenInteger = 10 * mTokenInteger + (ch - '0');
// 				else if (ch == '.')
// 				{
// 					mTokenFloat = mTokenInteger;
// 					fraction = 0.1;
// 					state = State::NumberFraction;
// 				}
// 				else
// 				{
// 					Retract();
// 					token = Token::INTEGER;
// 				}
// 				break;

// 			case State::NumberFraction:
// 				if (ch >= '0' and ch <= '9')
// 				{
// 					mTokenFloat += fraction * (ch - '0');
// 					fraction /= 10;
// 				}
// 				else if (ch == 'e' or ch == 'E')
// 					state = State::NumberExpSign;
// 				else
// 				{
// 					Retract();
// 					token = Token::NUMBER;
// 				}
// 				break;

// 			case State::NumberExpSign:
// 				if (ch == '+')
// 					state = State::NumberExpDigit1;
// 				else if (ch == '-')
// 				{
// 					negativeExp = true;
// 					state = State::NumberExpDigit1;
// 				}
// 				else if (ch >= '0' and ch <= '9')
// 				{
// 					exponent = (ch - '0');
// 					state = State::NumberExpDigit2;
// 				}
// 				break;

// 			case State::NumberExpDigit1:
// 				if (ch >= '0' and ch <= '9')
// 				{
// 					exponent = (ch - '0');
// 					state = State::NumberExpDigit2;
// 				}
// 				else
// 					throw std::runtime_error("invalid floating point format in command");
// 				break;

// 			case State::NumberExpDigit2:
// 				if (ch >= '0' and ch <= '9')
// 					exponent = 10 * exponent + (ch - '0');
// 				else
// 				{
// 					Retract();
// 					mTokenFloat *= pow(10, (negativeExp ? -1 : 1) * exponent);
// 					if (negative)
// 						mTokenFloat = -mTokenFloat;
// 					token = Token::NUMBER;
// 				}
// 				break;

// 			case State::Literal:
// 				if (not is_name_char(ch))
// 				{
// 					Retract();

// 					if (auto i = kKeyWords.find(mToken); i != kKeyWords.end())
// 						token = i->second;
// 					else
// 						token = Token::IDENT;
// 				}
// 				break;

// 			case State::String:
// 				if (ch == '\'')
// 				{
// 					token = Token::STRING;
// 					mToken.pop_back();
// 				}
// 				else if (ch == 0)
// 					throw std::runtime_error("Invalid unterminated std::string in command");
// 				else if (ch == '\\')
// 				{
// 					state = State::Escape;
// 					mToken.pop_back();
// 				}
// 				break;

// 			case State::Escape:
// 				switch (ch)
// 				{
// 					case '\'':
// 					case '\\':
// 					case '/':
// 						break;

// 					case 'n': mToken.back() = '\n'; break;
// 					case 't': mToken.back() = '\t'; break;
// 					case 'r': mToken.back() = '\r'; break;
// 					case 'f': mToken.back() = '\f'; break;
// 					case 'b': mToken.back() = '\b'; break;

// 					case 'u':
// 						state = State::EscapeHex1;
// 						mToken.pop_back();
// 						break;

// 					default:
// 						throw std::runtime_error("Invalid escape sequence in command (\\" + std::string{ static_cast<char>(ch) } + ')');
// 				}
// 				if (state == State::Escape)
// 					state = State::String;
// 				break;

// 			case State::EscapeHex1:
// 				if (/*ch >= 0 and*/ ch <= '9')
// 					hx = ch - '0';
// 				else if (ch >= 'a' and ch <= 'f')
// 					hx = 10 + ch - 'a';
// 				else if (ch >= 'A' and ch <= 'F')
// 					hx = 10 + ch - 'A';
// 				else
// 					throw std::runtime_error("Invalid hex sequence in command");
// 				mToken.pop_back();
// 				state = State::EscapeHex2;
// 				break;

// 			case State::EscapeHex2:
// 				if (/*ch >= 0 and*/ ch <= '9')
// 					hx = 16 * hx + ch - '0';
// 				else if (ch >= 'a' and ch <= 'f')
// 					hx = 16 * hx + 10 + ch - 'a';
// 				else if (ch >= 'A' and ch <= 'F')
// 					hx = 16 * hx + 10 + ch - 'A';
// 				else
// 					throw std::runtime_error("Invalid hex sequence in command");
// 				mToken.pop_back();
// 				mToken += hx;
// 				state = State::String;
// 				break;
// 		}
// 	}

// 	return token;
// }

// void Parser::Match(Token expected)
// {
// 	if (mLookahead != expected)
// 		throw std::runtime_error("Syntax error in command, expected " + Describe(expected) + " but found " + Describe(mLookahead) + " (" + mToken + ")");

// 	mLookahead = GetNextToken();
// }

// StatementPtr Parser::Parse(std::streambuf *is)
// {
// 	mIs = is;

// 	mLookahead = GetNextToken();
// 	return ParseStatement();
// 	// std::shared_ptr<StatementList> result(new StatementList());

// 	// while (mLookahead != Token::EOLN)
// 	// {
// 	// 	auto stmt = ParseStatement();
// 	// 	result->Add(stmt);
// 	// }

// 	// return result;
// }

// // -----------------------------------------------------------------------

// StatementPtr Parser::ParseStatement()
// {
// 	StatementPtr result;

// 	switch (mLookahead)
// 	{
// 		case Token::SELECT:
// 			Match(Token::SELECT);
// 			result = ParseSelect();
// 			break;

// 			// case Token::DELETE:
// 			// 	Match(Token::DELETE);
// 			// 	result = ParseDelete();
// 			// 	break;

// 			// case Token::UPDATE:
// 			// 	Match(Token::UPDATE);
// 			// 	result = ParseUpdate();
// 			// 	break;

// 		default:
// 			// force error
// 			Match(Token::SELECT);
// 	}

// 	if (mLookahead != Token::EOLN)
// 		Match(Token::SEMICOLON);

// 	return result;
// }

// // -----------------------------------------------------------------------

// StatementPtr Parser::ParseSelect()
// {
// 	bool distinct = false, all = false;
// 	if (mLookahead == Token::DISTINCT)
// 	{
// 		distinct = true;
// 		Match(Token::DISTINCT);
// 	}
// 	else if (mLookahead == Token::ALL)
// 	{
// 		all = true;
// 		Match(Token::ALL);
// 	}

// 	auto items = ParseItemList();

// 	Match(Token::FROM);

// 	std::string cat = mToken;
// 	Match(Token::IDENT);

// 	auto category = mDb.get(cat);
// 	if (category == nullptr)
// 		throw std::runtime_error("Category " + cat + " is not defined in this datablock");

// 	std::vector<std::string> cat_items;

// 	auto cv = category->get_cat_validator();
// 	if (cv != nullptr)
// 	{
// 		transform(cv->m_item_validators.begin(), cv->m_item_validators.end(), back_inserter(cat_items),
// 			[cat](auto iv)
// 			{ return iv.m_item_name; });
// 	}
// 	else
// 	{
// 		for (auto &item : category->get_items())
// 			cat_items.emplace_back(item);
// 	}

// 	std::vector<std::pair<std::string, std::string>> nItems;

// 	for (auto item : items)
// 	{
// 		if (item.second == "*")
// 			std::transform(cat_items.begin(), cat_items.end(), std::back_inserter(nItems),
// 				[](const std::string &ci)
// 				{ return std::make_pair(ci, ci); });
// 		else
// 			nItems.push_back(item);
// 	}

// 	swap(items, nItems);

// 	items.erase(remove_if(items.begin(), items.end(), [category](auto item)
// 					{ return not category->has_item(item.second); }),
// 		items.end());

// 	if (cv)
// 	{
// 		for (auto item : items)
// 		{
// 			auto iv = cv->get_validator_for_item(item.second);
// 			if (iv == nullptr)
// 				throw std::runtime_error("Item " + item.second + " is not defined in the dictionary for category " + cat);
// 		}
// 	}

// 	column_list columns;

// 	for (auto &item : items)
// 		columns.emplace_back(item.first, category->get_item_ix(item.second));

// 	std::shared_ptr<view> view;

// 	if (mLookahead == Token::WHERE)
// 	{
// 		Match(Token::WHERE);

// 		std::vector<row_handle> rows;
// 		for (auto rh : category->find(ParseNotWhereClause(*category, columns)))
// 			rows.emplace_back(rh);

// 		view.reset(new row_handle_view(std::move(rows)));
// 	}
// 	else
// 		view.reset(new simple_view(*category));

// 	return std::make_shared<SelectStatement>(view, std::move(columns));
// }

// // // -----------------------------------------------------------------------

// // StatementPtr Parser::ParseDelete()
// // {
// // 	Match(Token::FROM);

// // 	std::string cat = mToken;
// // 	Match(Token::IDENT);

// // 	auto category = mDb.get(cat);
// // 	if (category == nullptr)
// // 		throw std::runtime_error("Category " + cat + " is not defined in this file");

// // 	if (mLookahead == Token::WHERE)
// // 	{
// // 		Match(Token::WHERE);
// // 		return StatementPtr{ new DeleteStatement(*category, ParseNotWhereClause(*category)) };
// // 	}
// // 	else
// // 		return StatementPtr{ new DeleteStatement(*category, cif::all()) };
// // }

// // // -----------------------------------------------------------------------

// // StatementPtr Parser::ParseUpdate()
// // {
// // 	std::string cat = mToken;
// // 	Match(Token::IDENT);

// // 	auto category = mDb.get(cat);
// // 	if (category == nullptr)
// // 		throw std::runtime_error("Category " + cat + " is not defined in this file");

// // 	auto cv = category->get_cat_validator();

// // 	Match(Token::SET);

// // 	std::vector<std::pair<std::string, std::string>> itemValuePairs;
// // 	for (;;)
// // 	{
// // 		std::string item = mToken;
// // 		Match(Token::IDENT);

// // 		auto iv = cv ? cv->get_validator_for_item(item) : nullptr;
// // 		if (cv and iv == nullptr)
// // 			throw std::runtime_error("Invalid item '" + item + "' for category '" + cat + '\'');

// // 		Match(Token::EQ);

// // 		std::string value = mToken;
// // 		switch (mLookahead)
// // 		{
// // 			case Token::INTEGER:
// // 			case Token::NUMBER:
// // 			case Token::STRING:
// // 				Match(mLookahead);
// // 				break;
// // 			default:
// // 				Match(Token::STRING);
// // 		}

// // 		if (iv)
// // 			iv->operator()(value);

// // 		itemValuePairs.emplace_back(item, value);

// // 		if (mLookahead == Token::COMMA)
// // 		{
// // 			Match(Token::COMMA);
// // 			continue;
// // 		}

// // 		break;
// // 	}

// // 	if (mLookahead == Token::WHERE)
// // 	{
// // 		Match(Token::WHERE);
// // 		return StatementPtr{ new UpdateStatement(*category, std::move(itemValuePairs), ParseNotWhereClause(*category)) };
// // 	}
// // 	else
// // 		return StatementPtr{ new UpdateStatement(*category, std::move(itemValuePairs), cif::all()) };
// // }

// // -----------------------------------------------------------------------

// std::vector<std::pair<std::string, std::string>> Parser::ParseItemList()
// {
// 	std::vector<std::pair<std::string, std::string>> items;

// 	if (mLookahead == Token::ASTERISK)
// 	{
// 		Match(Token::ASTERISK);
// 		items.emplace_back("", "*");
// 	}
// 	else
// 	{
// 		for (;;)
// 		{
// 			if (mLookahead == Token::BRACE_OPEN)
// 			{
// 				Match(Token::BRACE_OPEN);

// 				Match(Token::BRACE_CLOSE);
// 			}
// 			else
// 			{
// 				items.emplace_back(mToken, mToken);
// 				Match(Token::IDENT);
// 			}

// 			if (mLookahead == Token::AS)
// 			{
// 				Match(Token::AS);
// 				items.back().first = mToken;
// 				Match(Token::IDENT);
// 			}

// 			if (mLookahead == Token::COMMA)
// 			{
// 				Match(Token::COMMA);
// 				continue;
// 			}

// 			break;
// 		}
// 	}

// 	return items;
// }

// // -----------------------------------------------------------------------

// cif::condition Parser::ParseNotWhereClause(cif::category &cat, const column_list &columns)
// {
// 	cif::condition result;

// 	if (mLookahead == Token::NOT)
// 	{
// 		Match(Token::NOT);
// 		result = not ParseNotWhereClause(cat, columns);
// 	}
// 	else if (mLookahead == Token::BRACE_OPEN)
// 	{
// 		Match(Token::BRACE_OPEN);
// 		result = ParseNotWhereClause(cat, columns);
// 		Match(Token::BRACE_CLOSE);
// 	}
// 	else
// 	{
// 		result = ParseWhereClause(cat, columns);

// 		for (;;)
// 		{
// 			if (mLookahead == Token::AND)
// 			{
// 				Match(Token::AND);
// 				result = std::move(result) and ParseNotWhereClause(cat, columns);
// 				continue;
// 			}

// 			if (mLookahead == Token::OR)
// 			{
// 				Match(Token::OR);
// 				result = std::move(result) or ParseNotWhereClause(cat, columns);
// 				continue;
// 			}

// 			break;
// 		}
// 	}
// 	return result;
// }

// // -----------------------------------------------------------------------

// cif::condition Parser::ParseWhereClause(cif::category &cat, const column_list &columns)
// {
// 	std::string item = mToken;

// 	if (auto i = std::find_if(columns.begin(), columns.end(), [&item](const column &col)
// 			{ return col.name == item; });
// 		i != columns.end())
// 		item = cat.get_item_name(i->index);

// 	Match(Token::IDENT);

// 	auto cv = cat.get_cat_validator();
// 	if (cv != nullptr and cv->get_validator_for_item(item) == nullptr)
// 	{
// 		throw std::runtime_error("Invalid item '" + item + "' for category '" + cat.name() + "' in where clause");
// 	}

// 	if (mLookahead == Token::IS)
// 	{
// 		Match(mLookahead);

// 		if (mLookahead == Token::NOT)
// 		{
// 			Match(mLookahead);
// 			Match(Token::_NULL);
// 			return cif::key(item) != cif::null;
// 		}
// 		else
// 		{
// 			Match(Token::_NULL);
// 			return cif::key(item) == cif::null;
// 		}
// 	}
// 	else if (mLookahead == Token::BETWEEN)
// 	{
// 		Match(Token::BETWEEN);

// 		cif::condition c;

// 		switch (mLookahead)
// 		{
// 			case Token::INTEGER:
// 			{
// 				auto v1 = mTokenInteger;
// 				Match(Token::INTEGER);
// 				Match(Token::AND);
// 				if (mLookahead == Token::NUMBER)
// 				{
// 					c = cif::key(item) >= v1 and cif::key(item) <= mTokenFloat;
// 					Match(Token::NUMBER);
// 				}
// 				else
// 				{
// 					c = cif::key(item) >= v1 and cif::key(item) <= mTokenInteger;
// 					Match(Token::INTEGER);
// 				}
// 				break;
// 			}

// 			case Token::NUMBER:
// 			{
// 				auto v1 = mTokenFloat;
// 				Match(Token::NUMBER);
// 				Match(Token::AND);
// 				if (mLookahead == Token::NUMBER)
// 				{
// 					c = cif::key(item) >= v1 and cif::key(item) <= mTokenFloat;
// 					Match(Token::NUMBER);
// 				}
// 				else
// 				{
// 					c = cif::key(item) >= v1 and cif::key(item) <= mTokenInteger;
// 					Match(Token::INTEGER);
// 				}
// 				break;
// 			}

// 			default:
// 			{
// 				auto v1 = mToken;
// 				Match(Token::STRING);
// 				Match(Token::AND);
// 				c = cif::key(item) >= v1 and cif::key(item) <= mToken;
// 				Match(Token::STRING);
// 				break;
// 			}
// 		}

// 		return c;
// 	}
// 	else
// 	{
// 		if (mLookahead < Token::EQ or mLookahead > Token::NE)
// 			Match(Token::EQ);

// 		auto oper = mLookahead;
// 		Match(mLookahead);

// 		cif::condition c;

// 		switch (mLookahead)
// 		{
// 			case Token::INTEGER:
// 				switch (oper)
// 				{
// 					case Token::EQ:
// 						c = cif::key(item) == mTokenInteger;
// 						break;
// 					case Token::LT:
// 						c = cif::key(item) < mTokenInteger;
// 						break;
// 					case Token::LE:
// 						c = cif::key(item) <= mTokenInteger;
// 						break;
// 					case Token::GT:
// 						c = cif::key(item) > mTokenInteger;
// 						break;
// 					case Token::GE:
// 						c = cif::key(item) >= mTokenInteger;
// 						break;
// 					case Token::NE:
// 						c = cif::key(item) != mTokenInteger;
// 						break;
// 					default: throw std::logic_error("should never happen");
// 				}

// 				Match(Token::INTEGER);
// 				break;

// 			case Token::NUMBER:
// 				switch (oper)
// 				{
// 					case Token::EQ:
// 						c = cif::key(item) == mTokenFloat;
// 						break;
// 					case Token::LT:
// 						c = cif::key(item) < mTokenFloat;
// 						break;
// 					case Token::LE:
// 						c = cif::key(item) <= mTokenFloat;
// 						break;
// 					case Token::GT:
// 						c = cif::key(item) > mTokenFloat;
// 						break;
// 					case Token::GE:
// 						c = cif::key(item) >= mTokenFloat;
// 						break;
// 					case Token::NE:
// 						c = cif::key(item) != mTokenFloat;
// 						break;
// 					default: throw std::logic_error("should never happen");
// 				}

// 				Match(Token::NUMBER);
// 				break;

// 			default:
// 				switch (oper)
// 				{
// 					case Token::EQ:
// 						c = cif::key(item) == mToken;
// 						break;
// 					case Token::LT:
// 						c = cif::key(item) < mToken;
// 						break;
// 					case Token::LE:
// 						c = cif::key(item) <= mToken;
// 						break;
// 					case Token::GT:
// 						c = cif::key(item) > mToken;
// 						break;
// 					case Token::GE:
// 						c = cif::key(item) >= mToken;
// 						break;
// 					case Token::NE:
// 						c = cif::key(item) != mToken;
// 						break;
// 					default: throw std::logic_error("should never happen");
// 				}

// 				Match(Token::STRING);
// 				break;
// 		}

// 		return c;
// 	}
// }

// --------------------------------------------------------------------

struct connection_impl
{
	datablock &m_db;
	sqlite3 *m_sqlite_db = nullptr;

	connection_impl(datablock &db);

	~connection_impl()
	{
		sqlite3_close(m_sqlite_db);
	}

	int Connect(sqlite3 *db, int argc, const char *const *argv, sqlite3_vtab **ppVtab, char **pzErr);

	// The module interface

	static int Connect(sqlite3 *db, void *pAux, int argc, const char *const *argv, sqlite3_vtab **ppVtab, char **pzErr);
	static int Disconnect(sqlite3_vtab *pVtab);
	static int Open(sqlite3_vtab *p, sqlite3_vtab_cursor **ppCursor);
	static int Close(sqlite3_vtab_cursor *cur);
	static int Next(sqlite3_vtab_cursor *cur);
	static int Column(
		sqlite3_vtab_cursor *cur, /* The cursor */
		sqlite3_context *ctx,     /* First argument to sqlite3_result_...() */
		int i);                   /* Which column to return */
	static int Rowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid);
	static int Eof(sqlite3_vtab_cursor *cur);
	static int Filter(sqlite3_vtab_cursor *pVtabCursor, int idxNum, const char *idxStr, int argc, sqlite3_value **argv);
	static int BestIndex(sqlite3_vtab *tab, sqlite3_index_info *pIdxInfo);

	static sqlite3_module s_module;
};

struct virtual_table
{
	sqlite3_vtab base;
	category &m_cat;
	std::unique_ptr<condition> m_condition;
};

struct virtual_cursor
{
	sqlite3_vtab_cursor base;
	category &m_cat;

	std::unique_ptr<conditional_iterator_proxy<category>> m_result;
	conditional_iterator_proxy<category>::iterator m_cur;
};

sqlite3_module connection_impl::s_module{
	/* iVersion    */ 0,
	/* xCreate     */ Connect,
	/* xConnect    */ Connect,
	/* xBestIndex  */ BestIndex,
	/* xDisconnect */ Disconnect,
	/* xDestroy    */ Disconnect,
	/* xOpen       */ Open,
	/* xClose      */ Close,
	/* xFilter     */ Filter,
	/* xNext       */ Next,
	/* xEof        */ Eof,
	/* xColumn     */ Column,
	/* xRowid      */ Rowid,
	/* xUpdate     */ 0,
	/* xBegin      */ 0,
	/* xSync       */ 0,
	/* xCommit     */ 0,
	/* xRollback   */ 0,
	/* xFindFunction */ 0,
	/* xRename     */ 0,
	/* xSavepoint  */ 0,
	/* xRelease    */ 0,
	/* xRollbackTo */ 0,
	/* xShadowName */ 0,
	/* xIntegrity  */ 0
};

/*
** The templatevtabConnect() method is invoked to create a new
** template virtual table.
**
** Think of this routine as the constructor for connection_impl objects.
**
** All this routine needs to do is:
**
**    (1) Allocate the connection_impl object and initialize all fields.
**
**    (2) Tell SQLite (via the sqlite3_declare_vtab() interface) what the
**        result set of queries against the virtual table will look like.
*/

int connection_impl::Connect(sqlite3 *db, void *pAux, int argc, const char *const *argv, sqlite3_vtab **ppVtab, char **pzErr)
{
	connection_impl *impl = reinterpret_cast<connection_impl *>(pAux);
	try
	{
		return impl->Connect(db, argc, argv, ppVtab, pzErr);
	}
	catch (const std::exception &ex)
	{
		*pzErr = sqlite3_mprintf("%s", ex.what());
		return SQLITE_ERROR;
	}
}

// --------------------------------------------------------------------

int connection_impl::Connect(sqlite3 *db, int argc, const char *const *argv, sqlite3_vtab **ppVtab, char **pzErr)
{
	if (argc < 3)
		throw std::runtime_error("Insufficient arguments to module connect");

	auto cat = m_db.get(argv[2]);
	if (cat == nullptr)
		throw std::runtime_error(std::format("Category {} is not known in this databank", argv[2]));

	std::vector<std::string> items;

	if (auto cv = cat->get_cat_validator(); cv != nullptr)
	{
		for (std::string item : cat->get_items())
		{
			auto iv = cv->get_validator_for_item(item);

			if (iv != nullptr)
			{
				if (iv->m_type->m_primitive_type == DDL_PrimitiveType::Numb)
				{
					if (iequals(iv->m_type->m_name, "int"))
					{
						items.emplace_back(std::format("'{}' {}", item, " INTEGER"));
						continue;
					}

					if (iequals(iv->m_type->m_name, "float"))
					{
						items.emplace_back(std::format("'{}' {}", item, " REAL"));
						continue;
					}
				}
			}

			items.emplace_back(std::format("'{}' {}", item, " TEXT"));
		}
	}
	else
	{
		for (auto item : cat->get_items())
			items.emplace_back(std::format("'{}'", item));
	}

	auto vtab = std::make_unique<virtual_table>(sqlite3_vtab{}, *cat);

	auto createStmt = std::format("CREATE TABLE {} ({})", cat->name(), join(items, ", "));

	int rc = sqlite3_declare_vtab(db, createStmt.c_str());
	if (rc == SQLITE_OK)
		*ppVtab = reinterpret_cast<sqlite3_vtab *>(vtab.release());

	return rc;
}

/*
** This method is the destructor for connection_impl objects.
*/
int connection_impl::Disconnect(sqlite3_vtab *pVtab)
{
	virtual_table *p = reinterpret_cast<virtual_table *>(pVtab);
	delete p;
	return SQLITE_OK;
}

/*
** Constructor for a new templatevtab_cursor object.
*/
int connection_impl::Open(sqlite3_vtab *pVtab, sqlite3_vtab_cursor **ppCursor)
{
	virtual_table *p = reinterpret_cast<virtual_table *>(pVtab);

	auto cursor = std::make_unique<virtual_cursor>(sqlite3_vtab_cursor{}, p->m_cat);
	*ppCursor = reinterpret_cast<sqlite3_vtab_cursor *>(cursor.release());
	return SQLITE_OK;
}

/*
** Destructor for a templatevtab_cursor.
*/
int connection_impl::Close(sqlite3_vtab_cursor *cur)
{
	auto pCur = reinterpret_cast<virtual_cursor *>(cur);
	delete pCur;
	return SQLITE_OK;
}

/*
** Advance a templatevtab_cursor to its next row of output.
*/
int connection_impl::Next(sqlite3_vtab_cursor *cur)
{
	auto pCur = reinterpret_cast<virtual_cursor *>(cur);
	++pCur->m_cur;
	return SQLITE_OK;
}

/*
** Return values of columns for the row at which the templatevtab_cursor
** is currently pointing.
*/
int connection_impl::Column(
	sqlite3_vtab_cursor *cur, /* The cursor */
	sqlite3_context *ctx,     /* First argument to sqlite3_result_...() */
	int i                     /* Which column to return */
)
{
	auto pCur = reinterpret_cast<virtual_cursor *>(cur);
	auto rh = *pCur->m_cur;
	auto item = rh[i];

	if (item.is_null())
		sqlite3_result_null(ctx);
	else if (auto cv = pCur->m_cat.get_cat_validator(); cv != nullptr)
	{
		if (auto iv = cv->get_validator_for_item(pCur->m_cat.get_item_name(i));
			iv != nullptr and iv->m_type->m_primitive_type == DDL_PrimitiveType::Numb)
		{
			if (iequals(iv->m_type->m_name, "int"))
			{
				auto v = item.as<int64_t>();
				sqlite3_result_int64(ctx, v);
			}
			else if (iequals(iv->m_type->m_name, "float"))
			{
				auto v = item.as<double>();
				sqlite3_result_double(ctx, v);
			}
			else
				sqlite3_result_text(ctx, item.text().data(), item.text().size(), SQLITE_STATIC);
		}
		else
			sqlite3_result_text(ctx, item.text().data(), item.text().size(), SQLITE_STATIC);
	}
	else
		sqlite3_result_text(ctx, item.text().data(), item.text().size(), SQLITE_STATIC);

	return SQLITE_OK;
}

/*
** Return the rowid for the current row.  In this implementation, the
** rowid is the same as the output value.
*/
int connection_impl::Rowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid)
{
	auto pCur = reinterpret_cast<virtual_cursor *>(cur);
	row_handle rh = *pCur->m_cur;
	*pRowid = rh.row_id();
	return SQLITE_OK;
}

/*
** Return TRUE if the cursor has been moved off of the last
** row of output.
*/
int connection_impl::Eof(sqlite3_vtab_cursor *cur)
{
	auto pCur = reinterpret_cast<virtual_cursor *>(cur);
	return pCur->m_cur == pCur->m_cat.end();
}

/*
** This method is called to "rewind" the templatevtab_cursor object back
** to the first row of output.  This method is always called at least
** once prior to any call to templatevtabColumn() or templatevtabRowid() or
** templatevtabEof().
*/
int connection_impl::Filter(sqlite3_vtab_cursor *pVtabCursor, int idxNum, const char *idxStr, int argc, sqlite3_value **argv)
{
	auto pCur = reinterpret_cast<virtual_cursor *>(pVtabCursor);

	pCur->m_result.reset();

	try
	{
		if (idxStr != nullptr)
		{
			struct membuf : public std::streambuf
			{
				membuf(char *text, std::size_t length)
				{
					this->setg(text, text, text + length);
				}
			} buffer(const_cast<char *>(idxStr), strlen(idxStr));

			std::istream is(&buffer);

			std::regex rx("^(.+?)( IS NULL| IS NOT NULL|(?: < | <= | == | >= | > ))(.+)?$");

			condition cond;
			std::string line;
			while (std::getline(is, line))
			{
				std::smatch m;
				if (not std::regex_match(line, m, rx))
					throw std::runtime_error("Internal error in cql, no match");

				if (m[2] == " IS NULL")
					cond = std::move(cond) and cif::key(m[1]) == cif::null;
				else if (m[2] == " IS NOT NULL")
					cond = std::move(cond) and cif::key(m[1]) != cif::null;
				else if (m[3].str().starts_with("\""))
				{
					std::istringstream isv(m[3]);
					std::string value;
					isv >> std::quoted(value);

					if (m[2] == " < ")
						cond = std::move(cond) and cif::key(m[1]) < value;
					else if (m[2] == " <- ")
						cond = std::move(cond) and cif::key(m[1]) <= value;
					else if (m[2] == " == ")
						cond = std::move(cond) and cif::key(m[1]) == value;
					else if (m[2] == " >= ")
						cond = std::move(cond) and cif::key(m[1]) >= value;
					else if (m[2] == " > ")
						cond = std::move(cond) and cif::key(m[1]) > value;
				}
				else
				{
					double value;
					const auto &[ptr, ec] = std::from_chars(m[3].str().data(), m[3].str().data() + m[3].str().length(), value);
					if (ec != std::errc{})
						throw std::system_error(std::make_error_code(ec));

					if (m[2] == " < ")
						cond = std::move(cond) and cif::key(m[1]) < value;
					else if (m[2] == " <- ")
						cond = std::move(cond) and cif::key(m[1]) <= value;
					else if (m[2] == " == ")
						cond = std::move(cond) and cif::key(m[1]) == value;
					else if (m[2] == " >= ")
						cond = std::move(cond) and cif::key(m[1]) >= value;
					else if (m[2] == " > ")
						cond = std::move(cond) and cif::key(m[1]) > value;
				}
			}

			pCur->m_result = std::make_unique<conditional_iterator_proxy<category>>(pCur->m_cat.find(std::move(cond)));
			pCur->m_cur = pCur->m_result->begin();
		}
	}
	catch (const std::exception &ex)
	{
		std::cerr << "Internal error: " << ex.what() << "\n";
	}

	if (not pCur->m_result)
	{
		condition cond = all();
		pCur->m_result = std::make_unique<conditional_iterator_proxy<category>>(pCur->m_cat.find(std::move(cond)));
		pCur->m_cur = pCur->m_result->begin();
	}

	return SQLITE_OK;
}

/*
** SQLite will invoke this method one or more times while planning a query
** that uses the virtual table.  This routine needs to create
** a query plan for each invocation and compute an estimated cost for that
** plan.
*/
int connection_impl::BestIndex(sqlite3_vtab *pVtab, sqlite3_index_info *pIdxInfo)
{
	virtual_table *p = reinterpret_cast<virtual_table *>(pVtab);

	try
	{
		std::ostringstream os;
		bool ok = true;

		if (pIdxInfo->nConstraint > 0)
		{
			auto constraint = [&os](std::string_view item, sqlite3_value *val, unsigned char op)
			{
				bool result = true;
				switch (op)
				{
					case SQLITE_INDEX_CONSTRAINT_EQ:
						os << item << " == ";
						break;
					case SQLITE_INDEX_CONSTRAINT_GT:
						os << item << " > ";
						break;
					case SQLITE_INDEX_CONSTRAINT_LE:
						os << item << " <= ";
						break;
					case SQLITE_INDEX_CONSTRAINT_LT:
						os << item << " < ";
						break;
					case SQLITE_INDEX_CONSTRAINT_GE:
						os << item << " >= ";
						break;
					default:
						result = false;
						break;
				}

				if (result)
				{
					switch (sqlite3_value_type(val))
					{
						case SQLITE_INTEGER:
							os << sqlite3_value_int64(val) << "\n";
							break;
						case SQLITE_FLOAT:
							os << sqlite3_value_double(val) << "\n";
							break;
						default:
						{
							std::string s = (const char *)sqlite3_value_text(val);
							if (s.find("\n") == std::string::npos)
								os << std::quoted(s) << "\n";
							else
								result = false;
							break;
						}
					}
				}

				return result;
			};

			for (int i = 0; ok and i < pIdxInfo->nConstraint; ++i)
			{
				auto &info = pIdxInfo->aConstraint[i];
				auto item = p->m_cat.get_item_name(info.iColumn);

				sqlite3_value *pVal;

				switch (info.op)
				{
					case SQLITE_INDEX_CONSTRAINT_EQ:
					case SQLITE_INDEX_CONSTRAINT_GT:
					case SQLITE_INDEX_CONSTRAINT_LE:
					case SQLITE_INDEX_CONSTRAINT_LT:
					case SQLITE_INDEX_CONSTRAINT_GE:
						if (sqlite3_vtab_rhs_value(pIdxInfo, i, &pVal) == SQLITE_OK and constraint(item, pVal, info.op))
						{
							pIdxInfo->aConstraintUsage[i].omit = 1;
							if (i < 63 and sqlite3_libversion_number() >= 3010000)
								pIdxInfo->colUsed |= 1 << i;
						}
						else
							ok = false;
						break;
					// case SQLITE_INDEX_CONSTRAINT_MATCH:
					// 	break;
					// case SQLITE_INDEX_CONSTRAINT_LIKE:
					// 	break;
					// case SQLITE_INDEX_CONSTRAINT_GLOB:
					// 	break;
					// case SQLITE_INDEX_CONSTRAINT_REGEXP:
					// 	break;
					case SQLITE_INDEX_CONSTRAINT_NE:
						break;
					// case SQLITE_INDEX_CONSTRAINT_ISNOT:
					// 	break;
					case SQLITE_INDEX_CONSTRAINT_ISNOTNULL:
						os << item << " IS NOT NULL\n";
						pIdxInfo->aConstraintUsage[i].omit = 1;
						if (i < 63 and sqlite3_libversion_number() >= 3010000)
							pIdxInfo->colUsed |= 1 << i;
						break;
					case SQLITE_INDEX_CONSTRAINT_ISNULL:
						os << item << " IS NULL\n";
						pIdxInfo->aConstraintUsage[i].omit = 1;
						if (i < 63 and sqlite3_libversion_number() >= 3010000)
							pIdxInfo->colUsed |= 1 << i;
						break;
						// case SQLITE_INDEX_CONSTRAINT_IS:
						// 	break;
						// case SQLITE_INDEX_CONSTRAINT_LIMIT:
						// 	break;
						// case SQLITE_INDEX_CONSTRAINT_OFFSET:
						// 	break;
						// case SQLITE_INDEX_CONSTRAINT_FUNCTION				:
						// 	break;

					default:
						ok = false;
						break;
				}
			}
		}

		if (auto cs = os.str(); ok and not cs.empty())
		{
			pIdxInfo->idxStr = sqlite3_mprintf("%s", cs.c_str());
			pIdxInfo->needToFreeIdxStr = 1;
		}
	}
	catch (const std::exception &ex)
	{
		std::cerr << ex.what() << "\n";
		if (sqlite3_libversion_number() >= 3010000)
			pIdxInfo->colUsed = 0;
	}

	pIdxInfo->estimatedCost = p->m_cat.size();
	pIdxInfo->estimatedRows = p->m_cat.size();
	return SQLITE_OK;
}

// --------------------------------------------------------------------

connection_impl::connection_impl(datablock &db)
	: m_db(db)
{
	auto rc = sqlite3_open(":memory:", &m_sqlite_db);

	if (rc)
		throw std::runtime_error(std::format("Cannot open databank: {}", sqlite3_errmsg(m_sqlite_db)));

	rc = sqlite3_create_module_v2(m_sqlite_db, "CIFPP", &connection_impl::s_module, this, nullptr);

	if (rc)
		throw std::runtime_error(std::format("Cannot create module: {}", sqlite3_errmsg(m_sqlite_db)));

	// Now, create a table for all known categories in the datablock

	for (auto &cat : db)
	{
		char *errmsg;
		rc = sqlite3_exec(m_sqlite_db,
			("CREATE VIRTUAL TABLE " + cat.name() + " USING CIFPP;").c_str(),
			nullptr, nullptr, &errmsg);
		if (rc != SQLITE_OK)
		{
			if (errmsg != nullptr)
			{
				std::string err = errmsg;
				sqlite3_free(errmsg);

				throw std::runtime_error("Error creating virtual tables for the categories: " + err);
			}

			throw std::runtime_error("Error creating virtual tables for the categories");
		}
	}
}

connection::connection(datablock &db)
	: m_impl(new connection_impl(db))
{
}

connection::~connection()
{
	delete m_impl;
}

// --------------------------------------------------------------------

transaction::transaction(connection &conn)
	: m_conn(conn)
{
}

transaction::~transaction()
{
}

result transaction::exec(const std::string &query)
{
	category cat;

	sqlite3_stmt *stmt = nullptr;

	try
	{
		int rc = sqlite3_prepare_v2(m_conn.m_impl->m_sqlite_db, query.data(), query.size(),
			&stmt, nullptr);

		if (rc != SQLITE_OK)
			throw std::runtime_error("Error preparing statement");

		for (int i = 0; i < sqlite3_column_count(stmt); ++i)
			cat.add_item(sqlite3_column_name(stmt, i));

		for (;;)
		{
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW)
			{
				row_initializer data;

				for (int i = 0; i < sqlite3_column_count(stmt); ++i)
				{
					switch (sqlite3_column_type(stmt, i))
					{
						case SQLITE_INTEGER:
							data.emplace_back(sqlite3_column_name(stmt, i), sqlite3_column_int64(stmt, i));
							break;
						case SQLITE_FLOAT:
							data.emplace_back(sqlite3_column_name(stmt, i), sqlite3_column_double(stmt, i));
							break;
						case SQLITE_TEXT:
							data.emplace_back(sqlite3_column_name(stmt, i), (const char *)sqlite3_column_text(stmt, i));
							break;
						case SQLITE_BLOB:
							// data.emplace_back(sqlite3_column_name(stmt, i), sqlite3_column_int64(stmt, i));
							throw std::runtime_error("Unexpected: blob in result");
							break;
						case SQLITE_NULL:
							data.emplace_back(sqlite3_column_name(stmt, i), ".");
							break;
					}
				}

				cat.emplace(std::move(data));
				continue;
			}

			if (rc == SQLITE_BUSY)
				throw std::runtime_error("Oops, busy?");
			if (rc == SQLITE_DONE)
				break;
			if (rc == SQLITE_ERROR)
				throw std::runtime_error(std::format("Error in sqlite: {}", sqlite3_errmsg(m_conn.m_impl->m_sqlite_db)));

			throw std::runtime_error("Unknown result from step");
		}

		sqlite3_finalize(stmt);

		return result(std::move(cat), query);
	}
	catch (const std::exception &ex)
	{
		if (stmt)
			sqlite3_finalize(stmt);
		throw;
	}
}

} // namespace cif::cql