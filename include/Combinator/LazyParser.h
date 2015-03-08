#ifndef PCOMB_LAZY_PARSER_H
#define PCOMB_LAZY_PARSER_H

#include "InputStream/PositionedInputStream.h"
#include "InputStream/StringInputStream.h"

#include "Type/InputStreamConcept.h"
#include "Type/ParserConcept.h"
#include "Type/ParseResult.h"

#include <memory>

namespace pcomb
{

// LazyParser allows the user to declare a parser first before giving its full definitions. This is useful for recursive grammar definitions.
// It is essentially a pointer to another parser. The challenge here is that it is not possible to tell, at compile time, what the type of that pointer might be because that type could appear AFTER the definition of this LazyParser. Therefore, the only solution I came up with is to use type erasure to allow that pointer points to any parser type at runtime.
// The downside of this solution is, however, that you must specify the input and output type of the inner parser when you declare a LazyParser
template <typename A>
class LazyParser
{
private:

	class PhantomParser
	{
	private:
		struct ParserConcept
		{
			virtual ParseResult<StringInputStream, A> parse(const StringInputStream&) const = 0;
			virtual ParseResult<PositionedInputStream, A> parse(const PositionedInputStream&) const = 0;
			virtual ParserConcept* clone() const = 0;
			virtual ~ParserConcept() {}
		};

		template <typename T>
		class ParserModel: public ParserConcept
		{
		private:
			const T& value;

			template <typename InputStream>
			ParseResult<InputStream, A> doParse(const InputStream& input) const
			{
				static_assert(IsParser<T, InputStream>::value, "LazyParser only accepts parser");
				return value.parse(input);
			}
		public:
			static_assert(std::is_convertible<ParserAttrType<T>, A>::value, "LazyParser parser type mismatch");

			ParserModel(const T& v): value(v) {}
			ParserModel* clone() const override
			{
				return new ParserModel<T>(value);
			}
			ParseResult<StringInputStream, A> parse(const StringInputStream& input) const override
			{
				return doParse(input);
			}
			ParseResult<PositionedInputStream, A> parse(const PositionedInputStream& input) const override
			{
				return doParse(input);
			}
		};

		std::unique_ptr<ParserConcept> impl;
	public:
		PhantomParser(): impl(nullptr) {}
		template <typename Parser>
		PhantomParser(const Parser& p): impl(std::make_unique<ParserModel<Parser>>(p)) {}
		PhantomParser(const PhantomParser& rhs)
		{
			if (rhs.impl.get() == nullptr)
				impl = nullptr;
			else
				impl = rhs->clone();
		}
		PhantomParser(PhantomParser&& rhs) = default;
		PhantomParser& operator=(const PhantomParser& rhs)
		{
			if (rhs.impl.get() == nullptr)
				impl = nullptr;
			else
				impl = rhs->clone();
		}
		PhantomParser& operator=(PhantomParser&& rhs) = default;

		template <typename InputType>
		ParseResult<InputType, A> parse(const InputType& input) const
		{
			assert(impl != nullptr);
			return impl->parse(input);
		}
	};

	std::shared_ptr<PhantomParser> parser;
public:
	using AttrType = A;

	LazyParser(): parser(std::make_shared<PhantomParser>()) {}

	template <typename Parser>
	LazyParser<A>& setParser(const Parser& p)
	{
		*parser = p;
		return *this;
	}

	template <typename InputType>
	ParseResult<InputType, AttrType> parse(const InputType& input) const
	{
		static_assert(IsInputStream<InputType>::value, "Parser's input type must be an InputStream!");

		assert(parser.get() != nullptr);

		return parser->parse(input);
	}
};

}

#endif