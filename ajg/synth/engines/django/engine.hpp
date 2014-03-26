//  (C) Copyright 2014 Alvaro J. Genial (http://alva.ro)
//  Use, modification and distribution are subject to the Boost Software
//  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#ifndef AJG_SYNTH_ENGINES_DJANGO_ENGINE_HPP_INCLUDED
#define AJG_SYNTH_ENGINES_DJANGO_ENGINE_HPP_INCLUDED

#include <ajg/synth/config.hpp>
#include <ajg/synth/vector.hpp>

#include <map>
#include <string>
#include <vector>
#include <ostream>
#include <numeric>
#include <algorithm>

#include <boost/ref.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/tokenizer.hpp>
#include <boost/noncopyable.hpp>

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>

#include <ajg/synth/templates.hpp>
#include <ajg/synth/engines/detail.hpp>
#include <ajg/synth/engines/exceptions.hpp>
#include <ajg/synth/engines/base_definition.hpp>
#include <ajg/synth/engines/django/value.hpp>
#include <ajg/synth/engines/django/loader.hpp>
#include <ajg/synth/engines/django/library.hpp>
#include <ajg/synth/engines/django/options.hpp>

namespace ajg {
namespace synth {
namespace django {

using detail::operator ==;
using detail::find_mapped_value;
namespace x = boost::xpressive;

template < class Library = django::default_library
         , class Loader  = django::default_loader
         >
struct engine : detail::nonconstructible {

typedef engine engine_type;

template <class BidirectionalIterator>
struct definition : base_definition< BidirectionalIterator
                                   , definition<BidirectionalIterator>
                                   > {
  private:

    template <class Sequence>
    struct define_sequence;

  public:

    typedef definition                          this_type;
    typedef base_definition
        < BidirectionalIterator
        , this_type
        >                                       base_type;
    typedef typename base_type::id_type         id_type;
    typedef typename base_type::size_type       size_type;
    typedef typename base_type::char_type       char_type;
    typedef typename base_type::match_type      match_type;
    typedef typename base_type::regex_type      regex_type;
    typedef typename base_type::frame_type      frame_type;
    typedef typename base_type::string_type     string_type;
    typedef typename base_type::stream_type     stream_type;
    typedef typename base_type::symbols_type    symbols_type;
    typedef typename base_type::iterator_type   iterator_type;
    typedef typename base_type::definition_type definition_type;

    typedef typename base_type::string_regex_type string_regex_type;

    typedef Library                               library_type;
    typedef Loader                                loader_type;
    typedef typename library_type::first          tags_type;
    typedef typename library_type::second         filters_type;
    typedef django::value<char_type>              value_type;
    typedef options<value_type>                   options_type;
    typedef typename value_type::boolean_type     boolean_type;
    typedef typename value_type::datetime_type    datetime_type;
    typedef typename value_type::duration_type    duration_type;
    typedef typename options_type::context_type   context_type;
    typedef typename options_type::names_type     names_type;
    typedef typename options_type::sequence_type  sequence_type;
    typedef typename options_type::arguments_type arguments_type;

    typedef detail::indexable_sequence<this_type, tags_type,    id_type,     detail::create_definitions_extended> tag_sequence_type;
    typedef detail::indexable_sequence<this_type, filters_type, string_type, detail::create_definitions_extended> filter_sequence_type;

  private:

    symbols_type keywords_, names_;

    struct not_in {
        symbols_type const& symbols;
        explicit not_in(symbols_type const& symbols) : symbols(symbols) {}

        bool operator ()(typename match_type::value_type const& match) const {
            return this->symbols.find(match.str()) == this->symbols.end();
        }
    };

  public:

    regex_type name(string_type const& s) {
        namespace x = boost::xpressive;
        names_.insert(s);
        return x::as_xpr(s)/* TODO: >> x::_b*/;
    }

    regex_type keyword(string_type const& s) {
        namespace x = boost::xpressive;
        keywords_.insert(s);
        return x::as_xpr(s)/* TODO: >> x::_b*/;
    }

    definition()
        : newline        (detail::text("\n"))
        , ellipsis       (detail::text("..."))
        , brace_open     (detail::text("{"))
        , brace_close    (detail::text("}"))
        , block_open     (detail::text("{%"))
        , block_close    (detail::text("%}"))
        , comment_open   (detail::text("{#"))
        , comment_close  (detail::text("#}"))
        , variable_open  (detail::text("{{"))
        , variable_close (detail::text("}}")) {
        using namespace xpressive;
//
// common grammar
////////////////////////////////////////////////////////////////////////////////////////////////////

        identifier
            = ((alpha | '_') >> *_w >> _b)[ x::check(not_in(keywords_)) ]
            ;
        unreserved_name
            = identifier[ x::check(not_in(names_)) ]
            ;
        package
            = identifier >> *('.' >> identifier)
            ;
        none_literal
            = as_xpr("None") >> _b
            ;
        true_literal
            = as_xpr("True") >> _b
            ;
        false_literal
            = as_xpr("False") >> _b
            ;
        boolean_literal
            = true_literal
            | false_literal
            ;
        number_literal
            = !(set= '-','+') >> +_d // integral part
                >> !('.' >> +_d)     // floating part
                >> !('e' >> +_d)     // exponent part
            ;
        string_literal
            = '"'  >> *~as_xpr('"')  >> '"'  // >> _b
            | '\'' >> *~as_xpr('\'') >> '\'' // >> _b
            ;
        variable_literal
            = identifier
            ;
        literal
            = none_literal
            | boolean_literal
            | number_literal
            | string_literal
            | variable_literal
            ;
        attribution
            = '.' >> *_s >> identifier
            ;
        subscription
            = '[' >> *_s >> x::ref(expression) >> *_s >> ']'
            ;
        chain
            = literal >> *(*_s >> (attribution | subscription))
            ;
        unary_operator
            = as_xpr("not") >> _b
            ;
        binary_operator
            = as_xpr("==")
            | as_xpr("!=")
            | as_xpr("<")
            | as_xpr(">")
            | as_xpr("<=")
            | as_xpr(">=")
            | as_xpr("and") >> _b
            | as_xpr("or")  >> _b
            | as_xpr("in")  >> _b
            | as_xpr("not") >> +_s >> "in" >> _b
            ;
        binary_expression
            = chain >> *(binary_operator >> x::ref(expression))
            ;
        unary_expression
            = unary_operator >> x::ref(expression)
            ;
        nested_expression
            = '(' >> *_s >> x::ref(expression) >> *_s >> ')'
            ;
        expression
            = unary_expression
            | binary_expression
            | nested_expression
            ;
        arguments
            = *(+_s >> expression)
            ;
        filter
            = identifier >> !(':' >> *_s >> chain)
            ;
        pipe
            = filter >> *(*_s >> '|' >> *_s >> filter)
            ;
        skipper
            = as_xpr(block_open)
            | as_xpr(block_close)
            | as_xpr(comment_open)
            | as_xpr(comment_close)
            | as_xpr(variable_open)
            | as_xpr(variable_close)
            ;
        nothing
            = as_xpr('\0') // Xpressive barfs when default-constructed.
            ;
        html_namechar
            = ~(set = ' ', '\t', '\n', '\v', '\f', '\r', '>')
            ;
        html_whitespace
            = (set = ' ', '\t', '\n', '\v', '\f', '\r')
            ;
        html_tag
            = '<' >> !as_xpr('/')
                   // The tag's name:
                   >> (s1 = -+html_namechar)
                   // Attributes, if any:
                   >> !(+html_whitespace >> -*~as_xpr('>'))
               >> !as_xpr('/')
            >> '>'
            ;

        this->initialize_grammar();
        fusion::for_each(tags_.definition, detail::construct<detail::element_initializer<this_type> >(*this));
        fusion::for_each(filters_.definition, detail::construct<append_filter>(*this));
        detail::index_sequence<this_type, tag_sequence_type, &this_type::tags_, tag_sequence_type::size>(*this);
    }

  public:

    value_type apply_filter( value_type     const& value
                           , string_type    const& name
                           , arguments_type const& arguments
                           , context_type   const& context
                           , options_type   const& options
                           ) const {
        process_filter const processor = { *this, value, name, arguments, context, options };
        // Let library filters override built-in ones:
        if (optional<typename options_type::filter_type const> const& filter
                = find_mapped_value(name, options.loaded_filters)) {
            return (*filter)(options, &context, value, arguments);
        }
        else if (optional<value_type> const& result = detail::may_find_by_index(
                    *this, filters_.definition, filters_.index, name, processor)) {
            return *result;
        }
        else {
            throw_exception(missing_filter(this->template transcode<char>(name)));
        }
    }

    template <char_type Delimiter>
    sequence_type split_argument( value_type   const& argument
                                , context_type const& context
                                , options_type const& options
                                ) const {
        typedef boost::char_separator<char_type>                                separator_type;
        typedef typename value_type::token_type                                 token_type;
        typedef typename token_type::const_iterator                             iterator_type;
        typedef boost::tokenizer<separator_type, iterator_type, token_type>     tokenizer_type;
        typedef definition<iterator_type>                                       definition_type;

        BOOST_ASSERT(argument.is_literal());
        token_type const& source = argument.token();
        static char_type const delimiter[2] = { Delimiter, 0 };
        separator_type const separator(delimiter, 0, keep_empty_tokens);
        tokenizer_type const tokenizer(source.begin(), source.end(), separator);
        static definition_type const tokenizable_definition;
        typename definition_type::match_type match;
        sequence_type sequence;

        BOOST_FOREACH(token_type const& token, tokenizer) {
            if (std::distance(token.begin(), token.end()) == 0) {
                sequence.push_back(value_type());
            }
            else if (xpressive::regex_match(token.begin(), token.end(), match, tokenizable_definition.chain)) {
                try {
                    sequence.push_back(tokenizable_definition.evaluate_chain(match, context, options));
                }
                catch (missing_variable const& e) {
                    string_type const string(token.begin(), token.end());

                    if (this->template transcode<char>(string) != e.name) {
                        throw_exception(e);
                    }

                    // A missing variable means an embedded
                    // argument was meant as a string literal.
                    value_type value = string;
                    value.token(match[0]);
                    sequence.push_back(value);
                }
            }
        }

        return sequence;
    }

    template <class T>
    string_type extract_string(T const& from) const {
        // TODO: Escape sequences, etc.
        // Handles "string" or 'string'.
        string_type const string = from.str();
        return string.substr(1, string.size() - 2);
    }

    void render( stream_type&        stream
               , frame_type   const& frame
               , context_type const& context
               , options_type const& options) const {
        render_block(stream, frame, context, options);
    }

    void render_file( stream_type&        stream
                    , string_type  const& filepath
                    , context_type const& context
                    , options_type const& options
                    ) const {
        typedef file_template<char_type, engine_type> file_template_type;
        std::string const filepath_ = this->template transcode<char>(filepath);
        file_template_type(filepath_, options.directories).render(stream, context, options);
    }

    void render_text( stream_type&        stream
                    , match_type   const& text
                    , context_type const& context
                    , options_type const& options
                    ) const {
        stream << text.str();
    }

    void render_block( stream_type&        stream
                     , match_type   const& block
                     , context_type const& context
                     , options_type const& options
                     ) const {
        BOOST_FOREACH(match_type const& nested, block.nested_results()) {
            render_match(stream, nested, context, options);
        }
    }

    void render_tag( stream_type&        stream
                   , match_type   const& match
                   , context_type const& context
                   , options_type const& options
                   ) const {
        using namespace detail;
        // If there's only _one_ tag, xpressive will not
        // "nest" the match, so we use it directly instead.
        match_type const& tag = tags_type::size::value == 1 ? match : get_nested<1>(match);
        tag_renderer<this_type> const renderer = { *this, stream, tag, context, options };
        must_find_by_index(*this, tags_.definition, tags_.index, tag.regex_id(), renderer);
    }

    void render_match( stream_type&        stream
                     , match_type   const& match
                     , context_type const& context
                     , options_type const& options
                     ) const {
             if (match == text)  render_text(stream, match, context, options);
        else if (match == block) render_block(stream, match, context, options);
        else if (match == tag)   render_tag(stream, match, context, options);
        else throw_exception(std::logic_error("invalid template state"));
    }

    value_type apply_pipe( value_type   const& value
                         , match_type   const& pipe
                         , context_type const& context
                         , options_type const& options
                         ) const {
        value_type result = value;

        BOOST_FOREACH(match_type const& filter, pipe.nested_results()) {
            match_type const& name  = filter(this->identifier);
            match_type const& chain = filter(this->chain);

            arguments_type arguments;
            if (chain) {
                arguments.first.push_back(evaluate_chain(chain, context, options));
            }
            result = apply_filter(result, name.str(), arguments, context, options);
        }

        return result;
    }

    arguments_type evaluate_arguments( match_type    const& args
                                     , context_type  const& context
                                     , options_type  const& options
                                     ) const {
        arguments_type arguments;
        BOOST_FOREACH(match_type const& arg, args.nested_results()) {
            arguments.first.push_back(this->evaluate_expression(arg, context, options));
        }
        return arguments;
    }

    value_type evaluate( match_type     const& match
                       , context_type   const& context
                       , options_type   const& options
              // TODO: , arguments_type const& arguments = arguments_type()
                       ) const {
        return evaluate_expression(match, context, options);
    }

    value_type evaluate_literal( match_type   const& match
                               , context_type const& context
                               , options_type const& options
                               ) const {
        value_type value;
        BOOST_ASSERT(match == this->literal);
        string_type const string = match.str();
        match_type const& literal = detail::get_nested<1>(match);

        if (literal == none_literal) {
            value = value_type();
            value.token(literal[0]);
        }
        else if (literal == boolean_literal) {
            match_type const& boolean = detail::get_nested<1>(literal);

            if (boolean == true_literal) {
                value = boolean_type(true);
                value.token(literal[0]);
            }
            else if (boolean == false_literal) {
                value = boolean_type(false);
                value.token(literal[0]);
            }
            else {
                throw_exception(std::logic_error("invalid boolean literal"));
            }
        }
        else if (literal == number_literal) {
            value = boost::lexical_cast<typename value_type::number_type>(string);
            value.token(literal[0]);
        }
        else if (literal == string_literal) {
            value = extract_string(literal);
            // Adjust the token by trimming the quotes.
            value.token(std::make_pair(literal[0].first + 1, literal[0].second - 1));
        }
        else if (literal == variable_literal) {
            if (optional<value_type const&> const
                    variable = detail::find_value(string, context)) {
                value = *variable;
                value.token(literal[0]);
            }
            else {
                throw_exception(missing_variable(
                    this->template transcode<char>(string)));
            }
        }
        else {
            throw_exception(std::logic_error("invalid literal"));
        }

        return value;
    }

    value_type evaluate_expression( match_type   const& match
                                  , context_type const& context
                                  , options_type const& options
                                  ) const {
        BOOST_ASSERT(match == this->expression);
        match_type const& expr = detail::get_nested<1>(match);

        if (expr == unary_expression) {
            return evaluate_unary(expr, context, options);
        }
        else if (expr == binary_expression) {
            return evaluate_binary(expr, context, options);
        }
        else if (expr == nested_expression) {
            match_type const& nested = detail::get_nested<1>(expr);
            return evaluate_expression(nested, context, options);
        }
        else {
            throw_exception(std::logic_error("invalid expression"));
        }
    }

    value_type evaluate_unary( match_type   const& unary
                             , context_type const& context
                             , options_type const& options
                             ) const {
        BOOST_ASSERT(unary == unary_expression);
        string_type const op = algorithm::trim_copy(detail::get_nested<1>(unary).str());
        match_type const& operand = detail::get_nested<2>(unary);

        if (op == "not") {
            return !evaluate_expression(operand, context, options);
        }
        else {
            throw_exception(std::logic_error("invalid unary operator: " + op));
        }
    }

    value_type evaluate_binary( match_type   const& binary
                              , context_type const& context
                              , options_type const& options
                              ) const {
        BOOST_ASSERT(binary == binary_expression);
        // First, evaluate the first segment, which is
        // always present, and which is always a chain.
        match_type const& chain = detail::get_nested<1>(binary);
        value_type value = evaluate_chain(chain, context, options);
        size_type i = 0;
        string_type op;

        BOOST_FOREACH( match_type const& segment
                     , binary.nested_results()
                     ) {
            if (!i++) continue; // Skip the first segment (the chain.)
            else if (segment == binary_operator) {
                op = algorithm::trim_copy(segment.str());
                continue;
            }
            else if (!(segment == expression)) {
                throw_exception(std::logic_error("invalid binary expression"));
            }

            if (op == "==") {
                value = value == evaluate_expression(segment, context, options);
            }
            else if (op == "!=") {
                value = value != evaluate_expression(segment, context, options);
            }
            else if (op == "<") {
                value = value < evaluate_expression(segment, context, options);
            }
            else if (op == ">") {
                value = value > evaluate_expression(segment, context, options);
            }
            else if (op == "<=") {
                value = value <= evaluate_expression(segment, context, options);
            }
            else if (op == ">=") {
                value = value >= evaluate_expression(segment, context, options);
            }
            else if (op == "and") {
                value = value ? evaluate_expression(segment, context, options) : value;
            }
            else if (op == "or") {
                value = value ? value : evaluate_expression(segment, context, options);
            }
            else if (op == "in") {
                value_type const elements = evaluate_expression(segment, context, options);
                value = elements.contains(value);
            }
            else if (algorithm::starts_with(op, "not") && algorithm::ends_with(op, "in")) {
                value_type const elements = evaluate_expression(segment, context, options);
                value = !elements.contains(value);
            }
            else {
                throw_exception(std::logic_error("invalid binary operator: " + op));
            }
        }

        return value;
    }

    value_type evaluate_chain( match_type   const& chain
                             , context_type const& context
                             , options_type const& options
                             ) const {
        BOOST_ASSERT(chain == this->chain);
        // Handle singly-nested expressions.
        /*if (chain == expression) {
            match_type const& nested = detail::get_nested<1>(chain);
            return evaluate(nested, context, options);
        }*/

        // First, evaluate the first segment, which is
        // always present, and which is always a literal.
        match_type const& literal = detail::get_nested<1>(chain);
        value_type value = evaluate_literal(literal, context, options);
        size_type i = 0;

        BOOST_FOREACH( match_type const& segment
                     , chain.nested_results()
                     ) {
            if (!i++) continue; // Skip the first segment (the literal.)
            value_type attribute;
            match_type const& nested = detail::get_nested<1>(segment);

            if (segment == subscription) { // i.e. value [ attribute ]
                attribute = evaluate(nested, context, options);
            }
            else if (segment == attribution) { // i.e. value.attribute
                attribute = nested.str();
            }
            else {
                throw_exception(std::logic_error("invalid chain"));
            }

            /*
            typename value_type::const_iterator const
                it = value.find_attribute(attribute);

            if (it == value.end()) {
                std::string const name = this->template
                    transcode<char>(attribute.to_string());
                throw_exception(missing_attribute(name));
            }

            value = *it;
            */

            value = value.must_get_attribute(attribute);
        }

        return value;
    }

    string_type format_datetime( options_type  const& options
                               , string_type   const& format
                               , datetime_type const& datetime
                               ) const {
        typedef std::map<char_type, string_type>            transliterations_type;
        typedef typename transliterations_type::value_type  transliteration_type;

        static transliterations_type const transliterations = boost::assign::list_of<transliteration_type>
            (char_type('%'), detail::text("%%"))
            (char_type('a'), detail::text("%P")) // TODO: Periods
            (char_type('A'), detail::text("%p"))
            (char_type('b'), detail::text("%b")) // TODO: Lowercase
            (char_type('B'), detail::text(""))   // "Not implemented" per spec.
            (char_type('c'), detail::text("%Y-%m-%dT%H:%M:%S%z"))
            (char_type('d'), detail::text("%d"))
            (char_type('D'), detail::text("%a"))
            (char_type('e'), detail::text("%z"))    // TODO: Ignored with ptimes
            (char_type('E'), detail::text("%B"))    // TODO: Make locale-aware
            (char_type('f'), detail::text("%l:%M")) // TODO: No leading blank, no zero minutes
            (char_type('F'), detail::text("%B"))
            (char_type('g'), detail::text("%l"))    // TODO: No leading blank
            (char_type('G'), detail::text("%k"))    // TODO: No leading blank
            (char_type('h'), detail::text("%I"))
            (char_type('H'), detail::text("%H"))
            (char_type('i'), detail::text("%M"))
            (char_type('I'), detail::text(""))   // TODO: Implement
            (char_type('j'), detail::text("%e")) // TODO: No leading blank
            (char_type('l'), detail::text("%A"))
            (char_type('L'), detail::text(""))   // TODO: Implement
            (char_type('m'), detail::text("%m"))
            (char_type('M'), detail::text("%b"))
            (char_type('n'), detail::text("%m")) // TODO: No leading zeros
            (char_type('N'), detail::text("%b")) // TODO: Abbreviations/periods
            (char_type('o'), detail::text("%G"))
            (char_type('O'), detail::text(""))   // TODO: Implement
            (char_type('P'), detail::text("%r")) // TODO: Periods, no zero minutes, "midnight"/"noon"
            (char_type('r'), detail::text("%a, %d %b %Y %T %z"))
            (char_type('s'), detail::text("%S"))
            (char_type('S'), detail::text(""))   // TODO: Implement
            (char_type('t'), detail::text(""))   // TODO: Implement
            (char_type('T'), detail::text(""))   // TODO: Implement
            (char_type('u'), detail::text("%f")) // TODO: No leading period
            (char_type('U'), detail::text(""))   // TODO: Implement
            (char_type('w'), detail::text("%w"))
            (char_type('W'), detail::text("%V")) // TODO: No leading zeros
            (char_type('y'), detail::text("%y"))
            (char_type('Y'), detail::text("%Y"))
            (char_type('z'), detail::text("%j")) // TODO: No leading zeros
            (char_type('Z'), detail::text(""))   // TODO: Implement
            ;

        std::basic_ostringstream<char_type> stream;

        // TODO: This might not be UTF8-safe; consider using a utf8_iterator.
        BOOST_FOREACH(char_type const c, find_mapped_value(format, options.formats).get_value_or(format)) {
            stream << find_mapped_value(c, transliterations).get_value_or(string_type(1, c));
        }

        return detail::format_time<string_type>(stream.str(), datetime);
    }

    // TODO: Proper, localizable formatting.
    // TODO: Factor out to detail.
    string_type format_duration( options_type  const& options
                               , duration_type const& duration
                               ) const {
        BOOST_STATIC_CONSTANT(size_type, N = 6);

        static size_type const seconds[N] = { 60 * 60 * 24 * 365
                                            , 60 * 60 * 24 * 30
                                            , 60 * 60 * 24 * 7
                                            , 60 * 60 * 24
                                            , 60 * 60
                                            , 60
                                            };
        static string_type const units[N] = { detail::text("year")
                                            , detail::text("month")
                                            , detail::text("week")
                                            , detail::text("day")
                                            , detail::text("hour")
                                            , detail::text("minute")
                                            };

        if (duration.is_negative()) {
            return pluralize_unit(0, units[N - 1], options);
        }

        string_type result;
        size_type const total = duration.total_seconds();
        size_type count = 0, i = 0;

        for (; i < N; ++i) {
            if ((count = total / seconds[i])) {
                break;
            }
        }

        result += pluralize_unit(count, units[i], options);

        if (i + 1 < N) {
            if ((count = (total - (seconds[i] * count)) / seconds[i + 1])) {
                result += string_type(detail::text(", "))
                       +  pluralize_unit(count, units[i + 1], options);
            }
        }

        return result;
    }

    /*inline static string_type nonbreaking(string_type const& s) {
        return algorithm::replace_all_copy(s, detail::text(" "), detail::text("\xA0")); options.nonbreaking_space
    }*/

    inline static string_type pluralize_unit( size_type    const  n
                                            , string_type  const& s
                                            , options_type const& options
                                            ) {
        return boost::lexical_cast<string_type>(n) + options.nonbreaking_space + s +
            (n == 1 ? string_type() : string_type(detail::text("s")));
    }

    optional<string_type> get_view_url( value_type     const& view
                                      , arguments_type const& arguments
                                      , context_type   const& context
                                      , options_type   const& options
                                      ) const {
        string_type name = view.to_string();

        BOOST_FOREACH(typename options_type::resolver_type const& resolver, options.resolvers) {
            if (optional<string_type> const& url = resolver->reverse(name, arguments, context, options)) {
                return url;
            }
        }
        return none;
    }

    void load_library( context_type&      context
                     , options_type&      options
                     , string_type const& library
                     , names_type  const* names   = 0
                     ) const {
        loader_.template load<this_type>(context, options, library, names);
    }

  private:

    struct process_filter {
        this_type      const& self;
        value_type     const& value_;
        string_type    const& name_;
        arguments_type const& arguments_;
        context_type   const& context_;
        options_type   const& options_;

        typedef value_type result_type;

        template <class Filter>
        value_type operator()(Filter const& filter) const {
            options_type& options = const_cast<options_type&>(options_);
            // TODO: Pass the full arguments_, not just the sequential (.first) ones.
            return filter.process(value_, self, name_, context_, arguments_.first, options);
        }
    };

    struct append_filter {
        this_type& self;

        template <class Filter>
        void operator()(Filter const& filter) const {
            self.filters_.index.push_back(filter.name());
        }
    };

  public:

    string_type const newline;
    string_type const ellipsis;
    string_type const brace_open;
    string_type const brace_close;
    string_type const block_open;
    string_type const block_close;
    string_type const comment_open;
    string_type const comment_close;
    string_type const variable_open;
    string_type const variable_close;

  public:

    regex_type tag, text, block, skipper, nothing;
    regex_type identifier, unreserved_name, package;
    regex_type arguments, filter, pipe, chain, subscription, attribution;
    regex_type unary_operator, binary_operator;
    regex_type unary_expression, binary_expression, nested_expression, expression;
    regex_type none_literal, true_literal, false_literal, boolean_literal;
    regex_type string_literal, number_literal, variable_literal, literal;
    string_regex_type html_namechar, html_whitespace, html_tag;

  private:

    tag_sequence_type    tags_;
    filter_sequence_type filters_;
    loader_type          loader_;

}; // definition

}; // engine

}}} // namespace ajg::synth::django

#endif // AJG_SYNTH_ENGINES_DJANGO_ENGINE_HPP_INCLUDED
