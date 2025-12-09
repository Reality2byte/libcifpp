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

#include "cif++/category.hpp"
#include "cif++/row.hpp"
#include "cif++/text.hpp"
#include "cif++/validate.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <stdexcept>

namespace cif::cql
{

// --------------------------------------------------------------------

field_ref row_ref::operator[](size_t ix) const noexcept
{
	return field_ref(m_row, m_cols->begin() + ix);
}

field_ref row_ref::operator[](std::string_view name) const noexcept
{
	if (m_row)
	{
		for (auto col = m_cols->begin(); col != m_cols->end(); ++col)
		{
			if (col->name == name)
				return field_ref(m_row, col);
		}
	}

	return field_ref(m_row, m_cols->end());
}

// --------------------------------------------------------------------

result::result(view &vw, const std::string &query)
	: m_query(query)
	, m_view(vw.shared_from_this())
{
}

size_t result::column_count() const
{
	return m_view->columns().size();
}

row_ref result::one_row() const
{
	if (auto sz = size(); sz != 1)
		throw std::runtime_error("Unexpected number of rows");

	return front();
}

field_ref result::one_field() const
{
	expect_columns(1);
	return one_row()[0];
}

view::const_row_iterator result::begin() const noexcept
{
	return m_view->begin();
}

view::const_row_iterator result::cbegin() const noexcept
{
	return m_view->cbegin();
}

view::const_row_iterator result::end() const noexcept
{
	return m_view->end();
}

view::const_row_iterator result::cend() const noexcept
{
	return m_view->cend();
}

row_ref result::front() const noexcept
{
	return m_view->front();
}

row_ref result::back() const noexcept
{
	return m_view->back();
}

size_t result::size() const noexcept
{
	return m_view->size();
}

bool result::empty() const noexcept
{
	return m_view->empty();
}

row_ref result::at(size_t index) const
{
	return m_view->at(index);
}

// --------------------------------------------------------------------

row_ref simple_view::front() const noexcept
{
	return row_ref{ m_cat.front(), m_columns };
}

row_ref simple_view::back() const noexcept
{
	return row_ref{ m_cat.back(), m_columns };
}

column_list simple_view::get_column_list_for_category(const category &cat)
{
	column_list result;
	for (int ix = 0; auto &item : cat.get_items())
		result.emplace_back(item, ix++);
	return result;
}

row_ref simple_view::at(size_t index) const
{
	// auto i = std::advance(m_cat.begin(), index);
	auto i = m_cat.begin();
	while (index--)
		++i;
	return { *i, m_columns };
}

// --------------------------------------------------------------------

class row_handle_view : public view
{
  public:
	row_handle_view(std::vector<row_handle> &&rows)
		: view(get_column_list_for_rows(rows))
		, m_rows(std::forward<std::vector<row_handle>>(rows))
	{
	}

	row_handle_view(const row_handle_view &) = default;
	row_handle_view(row_handle_view &&) = default;

	virtual size_t size() const noexcept override { return m_rows.size(); }

	virtual row_ref front() const noexcept override
	{
		return row_ref{ m_rows.front(), m_columns };
	}

	virtual row_ref back() const noexcept override
	{
		return row_ref{ m_rows.back(), m_columns };
	}

	virtual row_ref at(size_t index) const override
	{
		return { m_rows.at(index), m_columns };
	}

	static column_list get_column_list_for_rows(const std::vector<row_handle> &rows)
	{
		column_list result;
		if (not rows.empty())
		{
			for (int ix = 0; auto &item : rows.front().get_category().get_items())
				result.emplace_back(item, ix++);
		}
		return result;
	}

  protected:
	std::vector<row_handle> m_rows;
};

// --------------------------------------------------------------------

class select_view : public view
{
  public:
	select_view(std::shared_ptr<view> vw, const column_list &cols)
		: view(cols)
		, m_subview(vw)
	{
	}

	select_view(std::shared_ptr<view> vw, column_list &&cols)
		: view(std::forward<column_list>(cols))
		, m_subview(vw)
	{
	}

	select_view(const select_view &) = default;
	select_view(select_view &&) = default;

	virtual size_t size() const noexcept override { return m_subview->size(); }

	virtual row_ref front() const noexcept override
	{
		return row_ref{ m_subview->front(), m_columns };
	}

	virtual row_ref back() const noexcept override
	{
		return row_ref{ m_subview->back(), m_columns };
	}

	virtual row_ref at(size_t index) const override
	{
		return row_ref{ m_subview->at(index), m_columns };
	}

  protected:
	std::shared_ptr<view> m_subview;
};

// --------------------------------------------------------------------

inline std::string to_hex(uint32_t i)
{
	char s[sizeof(i) * 2 + 3];
	char *p = s + sizeof(s);
	*--p = 0;

	const char kHexChars[] = "0123456789abcdef";

	while (i)
	{
		*--p = kHexChars[i & 0x0F];
		i >>= 4;
	}

	*--p = 'x';
	*--p = '0';

	return p;
}

// --------------------------------------------------------------------

class Statement;
using StatementPtr = std::shared_ptr<Statement>;

// -----------------------------------------------------------------------

class Statement
{
  public:
	Statement(const Statement &) = delete;
	Statement &operator=(const Statement &) = delete;

	virtual ~Statement() = default;

	virtual std::shared_ptr<view> Execute() = 0;

  protected:
	Statement() = default;
};

// --------------------------------------------------------------------

class ValueExpression : public std::enable_shared_from_this<ValueExpression>
{
  public:
	ValueExpression(const ValueExpression &) = delete;
	ValueExpression &operator=(const ValueExpression &) = delete;

	virtual ~ValueExpression() = default;

	virtual item Execute(const std::vector<std::pair<std::string, row_handle>> &data) = 0;

  protected:
	ValueExpression() = default;
};

// --------------------------------------------------------------------

class DerivedColumn
{
  public:
	DerivedColumn(ValueExpression *expr, const std::string &name)
		: mExpr(expr->shared_from_this())
		, mName(name)
	{
	}

	item Execute(const std::vector<std::pair<std::string, row_handle>> &data)
	{
		return mExpr->Execute(data);
	}

	std::string Name() const
	{
		return mName;
	}

  private:
	std::shared_ptr<ValueExpression> mExpr;
	std::string mName;
};

// --------------------------------------------------------------------

class SimpleColumnValueExpression : public ValueExpression
{
  public:
	SimpleColumnValueExpression(const std::string &column)
		: mColumn(column)
	{
	}

	item Execute(const std::vector<std::pair<std::string, row_handle>> &data) override;

  private:
	std::string mColumn;
};


// // -----------------------------------------------------------------------

// class StatementList : public Statement
// {
//   public:
// 	StatementList() {}

// 	void Add(StatementPtr stmt)
// 	{
// 		mStatements.emplace_back(stmt);
// 	}

// 	virtual std::shared_ptr<view> Execute() override
// 	{
// 		for (auto stmt : mStatements)
// 			stmt->Execute();
// 	}

//   private:
// 	std::vector<StatementPtr> mStatements;
// };

// --------------------------------------------------------------------

class FromStatement : public Statement
{
  public:
	FromStatement(cif::category &cat)
		: m_category(cat)
	{
	}

	virtual std::shared_ptr<view> Execute() override
	{
		return std::make_shared<simple_view>(m_category);
	}

  private:
	cif::category &m_category;
};

// -----------------------------------------------------------------------

class SelectStatement : public Statement
{
  public:
	SelectStatement(std::shared_ptr<view> view, const column_list &cols)
		: m_view(view)
		, m_columns(cols)
	{
	}

	SelectStatement(std::shared_ptr<view> view, column_list &&cols)
		: m_view(view)
		, m_columns(std::forward<column_list>(cols))
	{
	}

	virtual std::shared_ptr<view> Execute() override
	{
		return std::make_shared<select_view>(m_view, m_columns);
	}

	std::shared_ptr<view> m_view;
	column_list m_columns;
};

// // -----------------------------------------------------------------------

// class DeleteStatement : public Statement
// {
//   public:
// 	DeleteStatement(cif::category &category, cif::condition &&where)
// 		: mCategory(category)
// 		, mWhere(std::move(where))
// 	{
// 	}

// 	virtual void Execute()
// 	{
// 		std::vector<cif::row_handle> remove;

// 		mWhere.prepare(mCategory);

// 		for (auto r : mCategory)
// 		{
// 			if (mWhere(r))
// 				remove.insert(remove.end(), r);
// 		}

// 		for (auto r : remove)
// 			mCategory.erase(r);

// 		std::cout << "Number of removed rows " << remove.size() << '\n';
// 	}

//   private:
// 	cif::category &mCategory;
// 	cif::condition mWhere;
// };

// // -----------------------------------------------------------------------

// class UpdateStatement : public Statement
// {
//   public:
// 	UpdateStatement(cif::category &category, std::vector<std::pair<std::string, std::string>> &&itemValuePairs, cif::condition &&where)
// 		: mCategory(category)
// 		, mItemValuePairs(std::move(itemValuePairs))
// 		, mWhere(std::move(where))
// 	{
// 	}

// 	virtual void Execute()
// 	{
// 		size_t updated = 0;

// 		mWhere.prepare(mCategory);

// 		for (auto r : mCategory)
// 		{
// 			if (mWhere(r))
// 			{
// 				for (auto iv : mItemValuePairs)
// 					r[iv.first] = iv.second;

// 				++updated;
// 			}
// 		}

// 		std::cout << "Number of updated rows: " << updated << '\n';
// 	}

//   private:
// 	cif::category &mCategory;
// 	std::vector<std::pair<std::string, std::string>> mItemValuePairs;
// 	cif::condition mWhere;
// };

class Parser
{
  public:
	Parser(cif::datablock &db)
		: mDb(db)
	{
		if (mDb.empty())
			throw std::runtime_error("Empty datablock");
	}

	StatementPtr Parse(std::streambuf *is);

  private:
	enum class Token
	{
		EOLN,

		UNDEF,

		BRACE_OPEN,
		BRACE_CLOSE,

		DOT,
		COMMA,
		COLON,
		SEMICOLON,
		ASTERISK,

		EQ,
		LT,
		LE,
		GT,
		GE,
		NE,

		STRING,
		INTEGER,
		NUMBER,

		IDENT,

		SELECT,
		DISTINCT,
		ALL,
		FROM,
		AS,
		BETWEEN,
		UPDATE,
		SET,
		WHERE,
		AND,
		OR,
		NOT,
		INSERT,
		DELETE,
		INTO,
		VALUES,
		IS,
		_NULL
	};

	std::map<std::string, Token, iless> kKeyWords{
		{ "SELECT", Token::SELECT },
		{ "DISTINCT", Token::DISTINCT },
		{ "ALL", Token::ALL },
		{ "FROM", Token::FROM },
		{ "AS", Token::AS },
		{ "BETWEEN", Token::BETWEEN },
		{ "UPDATE", Token::UPDATE },
		{ "SET", Token::SET },
		{ "WHERE", Token::WHERE },
		{ "AND", Token::AND },
		{ "OR", Token::OR },
		{ "NOT", Token::NOT },
		{ "INSERT", Token::INSERT },
		{ "DELETE", Token::DELETE },
		{ "INTO", Token::INTO },
		{ "VALUES", Token::VALUES },
		{ "IS", Token::IS },
		{ "NULL", Token::_NULL }
	};

	std::string Describe(Token token)
	{
		switch (token)
		{
			case Token::EOLN: return "<EOLN>";
			case Token::UNDEF: return "<UNDEFINED>";
			case Token::BRACE_OPEN: return "'('";
			case Token::BRACE_CLOSE: return "')'";

			case Token::DOT: return "'.'";
			case Token::COMMA: return "','";
			case Token::COLON: return "':'";
			case Token::SEMICOLON: return "';'";
			case Token::ASTERISK: return "'*'";

			case Token::EQ: return "'='";
			case Token::LT: return "'<'";
			case Token::LE: return "'<='";
			case Token::GT: return "'>'";
			case Token::GE: return "'>='";
			case Token::NE: return "'<>'";

			case Token::STRING: return "string";
			case Token::INTEGER: return "integer";
			case Token::NUMBER: return "number";
			case Token::IDENT: return "identifier";

			case Token::SELECT: return "SELECT";
			case Token::DISTINCT: return "DISTINCT";
			case Token::FROM: return "FROM";
			case Token::AS: return "AS";
			case Token::UPDATE: return "UPDATE";
			case Token::SET: return "SET";
			case Token::WHERE: return "WHERE";
			case Token::AND: return "AND";
			case Token::OR: return "OR";
			case Token::NOT: return "NOT";
			case Token::INSERT: return "INSERT";
			case Token::DELETE: return "DELETE";
			case Token::INTO: return "INTO";
			case Token::VALUES: return "VALUES";
			case Token::IS: return "IS";
			case Token::_NULL: return "NULL";

			default: assert(false); return "unknown token";
		}
	}

	char GetNextChar();
	Token GetNextToken();

	void Retract();
	void Match(Token token);

	// parser rules
	StatementPtr ParseStatement();
	StatementPtr ParseSelect();
	StatementPtr ParseDelete();
	StatementPtr ParseUpdate();
	std::vector<std::pair<std::string, std::string>> ParseItemList();

	cif::condition ParseWhereClause(cif::category &cat, const column_list &columns);
	cif::condition ParseNotWhereClause(cif::category &cat, const column_list &columns);

	cif::datablock &mDb;
	std::streambuf *mIs;
	Token mLookahead;
	std::stack<char> mBuffer;
	std::string mToken;
	double mTokenFloat;
	int64_t mTokenInteger;
};

// -----------------------------------------------------------------------

char Parser::GetNextChar()
{
	char result = 0;

	if (not mBuffer.empty()) // if buffer is not empty we already did all the validity checks
	{
		result = mBuffer.top();
		mBuffer.pop();
	}
	else
	{
		int c = mIs->sbumpc();
		result = c == std::streambuf::traits_type::eof() ? 0 : static_cast<char>(c);
	}

	mToken += result;

	return result;
}

void Parser::Retract()
{
	assert(not mToken.empty());
	mBuffer.push(mToken.back());
	mToken.pop_back();
}

constexpr bool is_name_start_char(char ch)
{
	return (ch >= 'A' and ch <= 'Z') or
	       ch == '_' or
	       (ch >= 'a' and ch <= 'z');
}

constexpr bool is_name_char(char ch)
{
	return (ch >= '0' and ch <= '9') or is_name_start_char(ch);
}

Parser::Token Parser::GetNextToken()
{
	enum class State
	{
		Start,
		Negative,
		Zero,
		NegativeZero,
		Number,
		NumberFraction,
		NumberExpSign,
		NumberExpDigit1,
		NumberExpDigit2,
		Literal,
		String,
		Escape,
		EscapeHex1,
		EscapeHex2,
		// EscapeHex3,
		// EscapeHex4,

		Less,
		Greater
	} state = State::Start;

	Token token = Token::UNDEF;
	double fraction = 1.0, exponent = 1;
	bool negative = false, negativeExp = false;

	char hx = 0;

	mToken.clear();

	while (token == Token::UNDEF)
	{
		auto ch = GetNextChar();

		switch (state)
		{
			case State::Start:
				switch (ch)
				{
					case 0:
						token = Token::EOLN;
						break;
					case '(':
						token = Token::BRACE_OPEN;
						break;
					case ')':
						token = Token::BRACE_CLOSE;
						break;
					// case '[':
					// 	token = Token::LeftBracket;
					// 	break;
					// case ']':
					// 	token = Token::RightBracket;
					// 	break;
					case '.':
						token = Token::DOT;
						break;
					case ',':
						token = Token::COMMA;
						break;
					case ':':
						token = Token::COLON;
						break;
					case ';':
						token = Token::SEMICOLON;
						break;
					case '*':
						token = Token::ASTERISK;
						break;
					case '=':
						token = Token::EQ;
						break;
					case '<':
						state = State::Less;
						break;
					case '>':
						state = State::Greater;
						break;
					case ' ':
					case '\n':
					case '\r':
					case '\t':
						mToken.clear();
						break;
					case '\'':
						mToken.pop_back();
						state = State::String;
						break;
					case '-':
						state = State::Negative;
						break;
					default:
						if (ch == '0')
						{
							state = State::Zero;
							mTokenInteger = 0;
						}
						else if (ch >= '1' and ch <= '9')
						{
							mTokenInteger = ch - '0';
							state = State::Number;
						}
						else if (is_name_start_char(ch))
							state = State::Literal;
						else
							throw std::runtime_error("invalid character (" + to_hex(ch) + "/'" + (isprint(ch) ? static_cast<char>(ch) : '.') + "') in command");
				}
				break;

			case State::Less:
				if (ch == '=')
					token = Token::LE;
				else if (ch == '>')
					token = Token::NE;
				else
				{
					Retract();
					token = Token::LT;
				}
				break;

			case State::Greater:
				if (ch == '=')
					token = Token::GE;
				else
				{
					Retract();
					token = Token::GT;
				}
				break;

			case State::Negative:
				if (ch == '0')
					state = State::NegativeZero;
				else if (ch >= '1' and ch <= '9')
				{
					state = State::Number;
					mTokenInteger = ch - '0';
					negative = true;
				}
				else
					throw std::runtime_error("invalid character '-' in command");
				break;

			case State::NegativeZero:
				if (ch >= '0' or ch <= '9')
					throw std::runtime_error("invalid number in command, should not start with zero");
				token = Token::NUMBER;
				break;

			case State::Zero:
				if (ch >= '0' or ch <= '9')
					throw std::runtime_error("invalid number in command, should not start with zero");
				token = Token::NUMBER;
				break;

			case State::Number:
				if (ch >= '0' and ch <= '9')
					mTokenInteger = 10 * mTokenInteger + (ch - '0');
				else if (ch == '.')
				{
					mTokenFloat = mTokenInteger;
					fraction = 0.1;
					state = State::NumberFraction;
				}
				else
				{
					Retract();
					token = Token::INTEGER;
				}
				break;

			case State::NumberFraction:
				if (ch >= '0' and ch <= '9')
				{
					mTokenFloat += fraction * (ch - '0');
					fraction /= 10;
				}
				else if (ch == 'e' or ch == 'E')
					state = State::NumberExpSign;
				else
				{
					Retract();
					token = Token::NUMBER;
				}
				break;

			case State::NumberExpSign:
				if (ch == '+')
					state = State::NumberExpDigit1;
				else if (ch == '-')
				{
					negativeExp = true;
					state = State::NumberExpDigit1;
				}
				else if (ch >= '0' and ch <= '9')
				{
					exponent = (ch - '0');
					state = State::NumberExpDigit2;
				}
				break;

			case State::NumberExpDigit1:
				if (ch >= '0' and ch <= '9')
				{
					exponent = (ch - '0');
					state = State::NumberExpDigit2;
				}
				else
					throw std::runtime_error("invalid floating point format in command");
				break;

			case State::NumberExpDigit2:
				if (ch >= '0' and ch <= '9')
					exponent = 10 * exponent + (ch - '0');
				else
				{
					Retract();
					mTokenFloat *= pow(10, (negativeExp ? -1 : 1) * exponent);
					if (negative)
						mTokenFloat = -mTokenFloat;
					token = Token::NUMBER;
				}
				break;

			case State::Literal:
				if (not is_name_char(ch))
				{
					Retract();

					if (auto i = kKeyWords.find(mToken); i != kKeyWords.end())
						token = i->second;
					else
						token = Token::IDENT;
				}
				break;

			case State::String:
				if (ch == '\'')
				{
					token = Token::STRING;
					mToken.pop_back();
				}
				else if (ch == 0)
					throw std::runtime_error("Invalid unterminated std::string in command");
				else if (ch == '\\')
				{
					state = State::Escape;
					mToken.pop_back();
				}
				break;

			case State::Escape:
				switch (ch)
				{
					case '\'':
					case '\\':
					case '/':
						break;

					case 'n': mToken.back() = '\n'; break;
					case 't': mToken.back() = '\t'; break;
					case 'r': mToken.back() = '\r'; break;
					case 'f': mToken.back() = '\f'; break;
					case 'b': mToken.back() = '\b'; break;

					case 'u':
						state = State::EscapeHex1;
						mToken.pop_back();
						break;

					default:
						throw std::runtime_error("Invalid escape sequence in command (\\" + std::string{ static_cast<char>(ch) } + ')');
				}
				if (state == State::Escape)
					state = State::String;
				break;

			case State::EscapeHex1:
				if (/*ch >= 0 and*/ ch <= '9')
					hx = ch - '0';
				else if (ch >= 'a' and ch <= 'f')
					hx = 10 + ch - 'a';
				else if (ch >= 'A' and ch <= 'F')
					hx = 10 + ch - 'A';
				else
					throw std::runtime_error("Invalid hex sequence in command");
				mToken.pop_back();
				state = State::EscapeHex2;
				break;

			case State::EscapeHex2:
				if (/*ch >= 0 and*/ ch <= '9')
					hx = 16 * hx + ch - '0';
				else if (ch >= 'a' and ch <= 'f')
					hx = 16 * hx + 10 + ch - 'a';
				else if (ch >= 'A' and ch <= 'F')
					hx = 16 * hx + 10 + ch - 'A';
				else
					throw std::runtime_error("Invalid hex sequence in command");
				mToken.pop_back();
				mToken += hx;
				state = State::String;
				break;
		}
	}

	return token;
}

void Parser::Match(Token expected)
{
	if (mLookahead != expected)
		throw std::runtime_error("Syntax error in command, expected " + Describe(expected) + " but found " + Describe(mLookahead) + " (" + mToken + ")");

	mLookahead = GetNextToken();
}

StatementPtr Parser::Parse(std::streambuf *is)
{
	mIs = is;

	mLookahead = GetNextToken();
	return ParseStatement();
	// std::shared_ptr<StatementList> result(new StatementList());

	// while (mLookahead != Token::EOLN)
	// {
	// 	auto stmt = ParseStatement();
	// 	result->Add(stmt);
	// }

	// return result;
}

// -----------------------------------------------------------------------

StatementPtr Parser::ParseStatement()
{
	StatementPtr result;

	switch (mLookahead)
	{
		case Token::SELECT:
			Match(Token::SELECT);
			result = ParseSelect();
			break;

			// case Token::DELETE:
			// 	Match(Token::DELETE);
			// 	result = ParseDelete();
			// 	break;

			// case Token::UPDATE:
			// 	Match(Token::UPDATE);
			// 	result = ParseUpdate();
			// 	break;

		default:
			// force error
			Match(Token::SELECT);
	}

	if (mLookahead != Token::EOLN)
		Match(Token::SEMICOLON);

	return result;
}

// -----------------------------------------------------------------------

StatementPtr Parser::ParseSelect()
{
	bool distinct = false, all = false;
	if (mLookahead == Token::DISTINCT)
	{
		distinct = true;
		Match(Token::DISTINCT);
	}
	else if (mLookahead == Token::ALL)
	{
		all = true;
		Match(Token::ALL);
	}

	auto items = ParseItemList();

	Match(Token::FROM);

	std::string cat = mToken;
	Match(Token::IDENT);

	auto category = mDb.get(cat);
	if (category == nullptr)
		throw std::runtime_error("Category " + cat + " is not defined in this datablock");

	std::vector<std::string> cat_items;

	auto cv = category->get_cat_validator();
	if (cv != nullptr)
	{
		transform(cv->m_item_validators.begin(), cv->m_item_validators.end(), back_inserter(cat_items),
			[cat](auto iv)
			{ return iv.m_item_name; });
	}
	else
	{
		for (auto &item : category->get_items())
			cat_items.emplace_back(item);
	}

	std::vector<std::pair<std::string, std::string>> nItems;

	for (auto item : items)
	{
		if (item.second == "*")
			std::transform(cat_items.begin(), cat_items.end(), std::back_inserter(nItems),
				[](const std::string &ci)
				{ return std::make_pair(ci, ci); });
		else
			nItems.push_back(item);
	}

	swap(items, nItems);

	items.erase(remove_if(items.begin(), items.end(), [category](auto item)
					{ return not category->has_item(item.second); }),
		items.end());

	if (cv)
	{
		for (auto item : items)
		{
			auto iv = cv->get_validator_for_item(item.second);
			if (iv == nullptr)
				throw std::runtime_error("Item " + item.second + " is not defined in the dictionary for category " + cat);
		}
	}

	column_list columns;

	for (auto &item : items)
		columns.emplace_back(item.first, category->get_item_ix(item.second));

	std::shared_ptr<view> view;

	if (mLookahead == Token::WHERE)
	{
		Match(Token::WHERE);

		std::vector<row_handle> rows;
		for (auto rh : category->find(ParseNotWhereClause(*category, columns)))
			rows.emplace_back(rh);

		view.reset(new row_handle_view(std::move(rows)));
	}
	else
		view.reset(new simple_view(*category));

	return std::make_shared<SelectStatement>(view, std::move(columns));
}

// // -----------------------------------------------------------------------

// StatementPtr Parser::ParseDelete()
// {
// 	Match(Token::FROM);

// 	std::string cat = mToken;
// 	Match(Token::IDENT);

// 	auto category = mDb.get(cat);
// 	if (category == nullptr)
// 		throw std::runtime_error("Category " + cat + " is not defined in this file");

// 	if (mLookahead == Token::WHERE)
// 	{
// 		Match(Token::WHERE);
// 		return StatementPtr{ new DeleteStatement(*category, ParseNotWhereClause(*category)) };
// 	}
// 	else
// 		return StatementPtr{ new DeleteStatement(*category, cif::all()) };
// }

// // -----------------------------------------------------------------------

// StatementPtr Parser::ParseUpdate()
// {
// 	std::string cat = mToken;
// 	Match(Token::IDENT);

// 	auto category = mDb.get(cat);
// 	if (category == nullptr)
// 		throw std::runtime_error("Category " + cat + " is not defined in this file");

// 	auto cv = category->get_cat_validator();

// 	Match(Token::SET);

// 	std::vector<std::pair<std::string, std::string>> itemValuePairs;
// 	for (;;)
// 	{
// 		std::string item = mToken;
// 		Match(Token::IDENT);

// 		auto iv = cv ? cv->get_validator_for_item(item) : nullptr;
// 		if (cv and iv == nullptr)
// 			throw std::runtime_error("Invalid item '" + item + "' for category '" + cat + '\'');

// 		Match(Token::EQ);

// 		std::string value = mToken;
// 		switch (mLookahead)
// 		{
// 			case Token::INTEGER:
// 			case Token::NUMBER:
// 			case Token::STRING:
// 				Match(mLookahead);
// 				break;
// 			default:
// 				Match(Token::STRING);
// 		}

// 		if (iv)
// 			iv->operator()(value);

// 		itemValuePairs.emplace_back(item, value);

// 		if (mLookahead == Token::COMMA)
// 		{
// 			Match(Token::COMMA);
// 			continue;
// 		}

// 		break;
// 	}

// 	if (mLookahead == Token::WHERE)
// 	{
// 		Match(Token::WHERE);
// 		return StatementPtr{ new UpdateStatement(*category, std::move(itemValuePairs), ParseNotWhereClause(*category)) };
// 	}
// 	else
// 		return StatementPtr{ new UpdateStatement(*category, std::move(itemValuePairs), cif::all()) };
// }

// -----------------------------------------------------------------------

std::vector<std::pair<std::string, std::string>> Parser::ParseItemList()
{
	std::vector<std::pair<std::string, std::string>> items;

	if (mLookahead == Token::ASTERISK)
	{
		Match(Token::ASTERISK);
		items.emplace_back("", "*");
	}
	else
	{
		for (;;)
		{
			if (mLookahead == Token::BRACE_OPEN)
			{
				Match(Token::BRACE_OPEN);



				Match(Token::BRACE_CLOSE);
			}
			else
			{
				items.emplace_back(mToken, mToken);
				Match(Token::IDENT);
			}

			if (mLookahead == Token::AS)
			{
				Match(Token::AS);
				items.back().first = mToken;
				Match(Token::IDENT);
			}

			if (mLookahead == Token::COMMA)
			{
				Match(Token::COMMA);
				continue;
			}

			break;
		}
	}

	return items;
}

// -----------------------------------------------------------------------

cif::condition Parser::ParseNotWhereClause(cif::category &cat, const column_list &columns)
{
	cif::condition result;

	if (mLookahead == Token::NOT)
	{
		Match(Token::NOT);
		result = not ParseNotWhereClause(cat, columns);
	}
	else if (mLookahead == Token::BRACE_OPEN)
	{
		Match(Token::BRACE_OPEN);
		result = ParseNotWhereClause(cat, columns);
		Match(Token::BRACE_CLOSE);
	}
	else
	{
		result = ParseWhereClause(cat, columns);

		for (;;)
		{
			if (mLookahead == Token::AND)
			{
				Match(Token::AND);
				result = std::move(result) and ParseNotWhereClause(cat, columns);
				continue;
			}

			if (mLookahead == Token::OR)
			{
				Match(Token::OR);
				result = std::move(result) or ParseNotWhereClause(cat, columns);
				continue;
			}

			break;
		}
	}
	return result;
}

// -----------------------------------------------------------------------

cif::condition Parser::ParseWhereClause(cif::category &cat, const column_list &columns)
{
	std::string item = mToken;

	if (auto i = std::find_if(columns.begin(), columns.end(), [&item](const column &col)
			{ return col.name == item; });
		i != columns.end())
		item = cat.get_item_name(i->index);

	Match(Token::IDENT);

	auto cv = cat.get_cat_validator();
	if (cv != nullptr and cv->get_validator_for_item(item) == nullptr)
	{
		throw std::runtime_error("Invalid item '" + item + "' for category '" + cat.name() + "' in where clause");
	}

	if (mLookahead == Token::IS)
	{
		Match(mLookahead);

		if (mLookahead == Token::NOT)
		{
			Match(mLookahead);
			Match(Token::_NULL);
			return cif::key(item) != cif::null;
		}
		else
		{
			Match(Token::_NULL);
			return cif::key(item) == cif::null;
		}
	}
	else if (mLookahead == Token::BETWEEN)
	{
		Match(Token::BETWEEN);

		cif::condition c;

		switch (mLookahead)
		{
			case Token::INTEGER:
			{
				auto v1 = mTokenInteger;
				Match(Token::INTEGER);
				Match(Token::AND);
				if (mLookahead == Token::NUMBER)
				{
					c = cif::key(item) >= v1 and cif::key(item) <= mTokenFloat;
					Match(Token::NUMBER);
				}
				else
				{
					c = cif::key(item) >= v1 and cif::key(item) <= mTokenInteger;
					Match(Token::INTEGER);
				}
				break;
			}

			case Token::NUMBER:
			{
				auto v1 = mTokenFloat;
				Match(Token::NUMBER);
				Match(Token::AND);
				if (mLookahead == Token::NUMBER)
				{
					c = cif::key(item) >= v1 and cif::key(item) <= mTokenFloat;
					Match(Token::NUMBER);
				}
				else
				{
					c = cif::key(item) >= v1 and cif::key(item) <= mTokenInteger;
					Match(Token::INTEGER);
				}
				break;
			}

			default:
			{
				auto v1 = mToken;
				Match(Token::STRING);
				Match(Token::AND);
				c = cif::key(item) >= v1 and cif::key(item) <= mToken;
				Match(Token::STRING);
				break;
			}
		}

		return c;
	}
	else
	{
		if (mLookahead < Token::EQ or mLookahead > Token::NE)
			Match(Token::EQ);

		auto oper = mLookahead;
		Match(mLookahead);

		cif::condition c;

		switch (mLookahead)
		{
			case Token::INTEGER:
				switch (oper)
				{
					case Token::EQ:
						c = cif::key(item) == mTokenInteger;
						break;
					case Token::LT:
						c = cif::key(item) < mTokenInteger;
						break;
					case Token::LE:
						c = cif::key(item) <= mTokenInteger;
						break;
					case Token::GT:
						c = cif::key(item) > mTokenInteger;
						break;
					case Token::GE:
						c = cif::key(item) >= mTokenInteger;
						break;
					case Token::NE:
						c = cif::key(item) != mTokenInteger;
						break;
					default: throw std::logic_error("should never happen");
				}

				Match(Token::INTEGER);
				break;

			case Token::NUMBER:
				switch (oper)
				{
					case Token::EQ:
						c = cif::key(item) == mTokenFloat;
						break;
					case Token::LT:
						c = cif::key(item) < mTokenFloat;
						break;
					case Token::LE:
						c = cif::key(item) <= mTokenFloat;
						break;
					case Token::GT:
						c = cif::key(item) > mTokenFloat;
						break;
					case Token::GE:
						c = cif::key(item) >= mTokenFloat;
						break;
					case Token::NE:
						c = cif::key(item) != mTokenFloat;
						break;
					default: throw std::logic_error("should never happen");
				}

				Match(Token::NUMBER);
				break;

			default:
				switch (oper)
				{
					case Token::EQ:
						c = cif::key(item) == mToken;
						break;
					case Token::LT:
						c = cif::key(item) < mToken;
						break;
					case Token::LE:
						c = cif::key(item) <= mToken;
						break;
					case Token::GT:
						c = cif::key(item) > mToken;
						break;
					case Token::GE:
						c = cif::key(item) >= mToken;
						break;
					case Token::NE:
						c = cif::key(item) != mToken;
						break;
					default: throw std::logic_error("should never happen");
				}

				Match(Token::STRING);
				break;
		}

		return c;
	}
}

// --------------------------------------------------------------------

result transaction::exec(std::string_view query)
{
	struct membuf : public std::streambuf
	{
		membuf(char *text, std::size_t length)
		{
			this->setg(text, text, text + length);
		}
	} buffer(const_cast<char *>(query.data()), query.size());

	Parser p(m_db);
	auto stmt = p.Parse(&buffer);
	return { *stmt->Execute() };
}

} // namespace cif::cql