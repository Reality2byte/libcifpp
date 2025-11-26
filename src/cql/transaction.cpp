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
#include "cif++/row.hpp"

#include <stdexcept>
#include <unordered_set>

namespace cif::cql
{

// --------------------------------------------------------------------

std::string_view field_ref::name() const &
{
	return m_row ? m_row.get_category().get_item_name(m_index) : "";
}

// --------------------------------------------------------------------

field_ref row_ref::operator[](size_t ix) const noexcept
{
	return field_ref(m_row, ix < m_cols->size() ? m_cols->at(ix) : 0);
}

field_ref row_ref::operator[](std::string_view name) const noexcept
{
	if (m_row)
	{
		auto &cat = m_row.get_category();

		for (auto col : *m_cols)
		{
			if (cat.get_item_name(col) == name)
				return field_ref(m_row, col);
		}
	}

	return field_ref(m_row, 0);
}

// --------------------------------------------------------------------

result::result(std::string query, std::vector<row_handle> rows, std::vector<int> columns)
	: m_rows(rows)
	, m_columns(columns)
	, m_query(query)
{
}

size_t result::columns() const
{
	return m_columns.size();
}

row_ref result::one_row() const
{
	if (auto sz = size(); sz != 1)
		throw std::runtime_error("Unexpected number of rows");

	return row_ref(m_rows[0], m_columns);
	// return front();
}

field_ref result::one_field() const
{
	expect_columns(1);
	return one_row()[0];
}

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

using cif::iequals;

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

	virtual result Execute() = 0;

  protected:
	Statement() = default;
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

// 	virtual result Execute()
// 	{
// 		for (auto stmt : mStatements)
// 			stmt->Execute();
// 	}

//   private:
// 	std::vector<StatementPtr> mStatements;
// };

// -----------------------------------------------------------------------

class SelectStatement : public Statement
{
  public:
	SelectStatement(cif::category &category, bool distinct, std::vector<std::string> &&items, cif::condition &&where)
		: mCategory(category)
		, mDistinct(distinct)
		, mItems(std::move(items))
		, mWhere(std::move(where))
	{
	}

	virtual result Execute()
	{
		std::vector<std::string> fields(mItems.size());
		std::unordered_set<std::string> seen;

		std::vector<row_handle> rows;

		// TODO: optimise this code please... duh
		for (auto r : mCategory.find(std::move(mWhere)))
		{
			transform(mItems.begin(), mItems.end(), fields.begin(),
				[r](auto item)
				{
					return r[item].template as<std::string>();
				});

			std::string line = cif::join(fields, "\t");
			bool seenLine = seen.count(line);

			if (not mDistinct or not seenLine)
			{
				rows.emplace_back(r);
				seen.insert(line);
			}
		}

		std::vector<int> cols;
		for (auto col : mItems)
			cols.emplace_back(mCategory.get_item_ix(col));

		return result("", std::move(rows), std::move(cols));
	}

  private:
	cif::category &mCategory;
	bool mDistinct;
	std::vector<std::string> mItems;
	cif::condition mWhere;
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
		FROM,
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
	std::vector<std::string> ParseItemList();

	cif::condition ParseWhereClause(cif::category &cat);
	cif::condition ParseNotWhereClause(cif::category &cat);

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
					case '*':
						token = Token::ASTERISK;
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

					if (iequals(mToken, "SELECT"))
						token = Token::SELECT;
					else if (iequals(mToken, "DISTINCT"))
						token = Token::DISTINCT;
					else if (iequals(mToken, "FROM"))
						token = Token::FROM;
					else if (iequals(mToken, "UPDATE"))
						token = Token::UPDATE;
					else if (iequals(mToken, "SET"))
						token = Token::SET;
					else if (iequals(mToken, "WHERE"))
						token = Token::WHERE;
					else if (iequals(mToken, "AND"))
						token = Token::AND;
					else if (iequals(mToken, "OR"))
						token = Token::OR;
					else if (iequals(mToken, "NOT"))
						token = Token::NOT;
					else if (iequals(mToken, "INSERT"))
						token = Token::INSERT;
					else if (iequals(mToken, "DELETE"))
						token = Token::DELETE;
					else if (iequals(mToken, "INTO"))
						token = Token::INTO;
					else if (iequals(mToken, "VALUES"))
						token = Token::VALUES;
					else if (iequals(mToken, "IS"))
						token = Token::IS;
					else if (iequals(mToken, "NULL"))
						token = Token::_NULL;
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

	Match(Token::SEMICOLON);

	return result;
}

// -----------------------------------------------------------------------

StatementPtr Parser::ParseSelect()
{
	bool distinct = false;
	if (mLookahead == Token::DISTINCT)
	{
		distinct = true;
		Match(Token::DISTINCT);
	}

	auto items = ParseItemList();

	Match(Token::FROM);

	std::string cat = mToken;
	Match(Token::IDENT);

	auto category = mDb.get(cat);
	if (category == nullptr)
		throw std::runtime_error("Category " + cat + " is not defined in this file");

	auto cv = category->get_cat_validator();
	if (cv != nullptr)
	{
		std::vector<std::string> nItems;

		for (auto item : items)
		{
			if (item == "*")
				transform(cv->m_item_validators.begin(), cv->m_item_validators.end(), back_inserter(nItems),
					[cat](auto iv)
					{ return iv.m_item_name; });
			else
				nItems.push_back(item);
		}

		swap(items, nItems);

		items.erase(remove_if(items.begin(), items.end(), [category](auto item)
						{ return not category->has_item(item); }),
			items.end());

		for (auto item : items)
		{
			auto iv = cv->get_validator_for_item(item);
			if (iv == nullptr)
				throw std::runtime_error("Item " + item + " is not defined in the PDBx dictionary for category " + cat);
		}
	}

	if (mLookahead == Token::WHERE)
	{
		Match(Token::WHERE);
		return StatementPtr{ new SelectStatement(*category, distinct, std::move(items), ParseNotWhereClause(*category)) };
	}
	else
		return StatementPtr{ new SelectStatement(*category, distinct, std::move(items), cif::all()) };
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

std::vector<std::string> Parser::ParseItemList()
{
	std::vector<std::string> items;

	for (;;)
	{
		if (mLookahead == Token::ASTERISK)
		{
			Match(Token::ASTERISK);
			items.push_back("*");
		}
		else
		{
			items.push_back(mToken);
			Match(Token::IDENT);
		}

		if (mLookahead == Token::COMMA)
		{
			Match(Token::COMMA);
			continue;
		}

		break;
	}

	return items;
}

// -----------------------------------------------------------------------

cif::condition Parser::ParseNotWhereClause(cif::category &cat)
{
	cif::condition result;

	if (mLookahead == Token::NOT)
	{
		Match(Token::NOT);
		result = not ParseNotWhereClause(cat);
	}
	else if (mLookahead == Token::BRACE_OPEN)
	{
		Match(Token::BRACE_OPEN);
		result = ParseNotWhereClause(cat);
		Match(Token::BRACE_CLOSE);
	}
	else
	{
		result = ParseWhereClause(cat);

		for (;;)
		{
			if (mLookahead == Token::AND)
			{
				Match(Token::AND);
				result = std::move(result) and ParseNotWhereClause(cat);
				continue;
			}

			if (mLookahead == Token::OR)
			{
				Match(Token::OR);
				result = std::move(result) or ParseNotWhereClause(cat);
				continue;
			}

			break;
		}
	}
	return result;
}

// -----------------------------------------------------------------------

cif::condition Parser::ParseWhereClause(cif::category &cat)
{
	std::string item = mToken;
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
	else
	{
		if (mLookahead < Token::EQ or mLookahead > Token::NE)
			Match(Token::EQ);

		auto oper = mLookahead;
		Match(mLookahead);

		cif::condition c;
		std::string value = mToken;

		switch (mLookahead)
		{
			case Token::INTEGER:
			case Token::NUMBER:
			case Token::STRING:
				Match(mLookahead);
				break;
			default:
				Match(Token::STRING);
		}

		switch (oper)
		{
			case Token::EQ: return cif::key(item) == value;
			case Token::LT: return cif::key(item) < value;
			case Token::LE: return cif::key(item) <= value;
			case Token::GT: return cif::key(item) > value;
			case Token::GE: return cif::key(item) >= value;
			case Token::NE: return cif::key(item) != value;
			default: throw std::logic_error("should never happen");
		}
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
	return stmt->Execute();
}


} // namespace cif::cql