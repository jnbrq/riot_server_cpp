/**
 * @author Canberk Sönmez
 * 
 * @date Wed Jun 20 03:21:06 +03 2018
 * 
 */

/**
 *  Notes to other people willing to edit this file:
 *          DO NOT EDIT THIS to keep Gods calm.
 * 
 *  However, if you really have to, then review each and every example of
 *  Boost.Spirit x3. Also, never ever forget that it generates Recursive
 *  Descent Parsers, and they have known limitations, such as generating
 *  only right-associative binary trees. Beware of you theoretical limitations!
 * 
 *  So, learn about parsing theory.
 * 
 *  I also strongly suggest you to read a good book about Modern C++, such
 *  as "Effective Modern C++: 42 Specific Ways to Improve Your Use of C++11
 *  and C++14" by Scott Meyers and you should have a deep understanding of
 *  TMP (template meta-programming) [many books are available, also google
 *  things like:
 *      c++ how std bind is implemented
 *      c++ reference collapse rules
 *      c++ how std function is implemented
 *      c++ how std variant is implemented
 *      c++ boost variant vs std variant
 *      c++ ....
 *  ]
 * 
 *  Good luck!
 */

/**
 *  Notes to myself:
 *          do not forget to "inline" functions free in the namespace scope, to
 *      avoid problems due to ODR (one definition rule)
 * 
 *          the boost spirit attributes are somehow inconsistent. so, strictly
 *      follow the guidelines given in the quick reference section of
 *      Boost.Spirit x3
 * 
 *          if your grammar contains recursive expressions, then always write
 *      the grammar first, then deduct what ast should be!
 */

/**
 * since the error handling doesn't play well with syntaxes like: '' '', I have
 * decided not to do any error reporting.
 * 
 * also, the error messages are multiline, which are not favorable in the
 * network protocol.
 */

#ifndef RIOT_SERVER_SFE_PARSER_INCLUDED
#define RIOT_SERVER_SFE_PARSER_INCLUDED

#include <string>
#include <list>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <utility>

#include <cassert>

#include <boost/variant/static_visitor.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>

// ASTs
namespace riot::server {
    // sfe = simple_filter_expression
    namespace sfe_parser {
        namespace ast {
            namespace x3 = boost::spirit::x3;
            
            struct expression;
            struct regex_op;
            struct unary_op;
            struct matcher_op;
            struct binary_op_group;
            
            struct nil {  };
            
            enum op_type: char {
                op_xor = '^',
                op_and = '&',
                op_or = '|',
                op_neg = '~'
            };
            
            enum matcher_type: char {
                matcher0 = '%',
                matcher1 = '$',
                matcher2 = '#'
            };
            
            struct regex_op {
                std::string str;    // regex in str form
                std::regex rgx;     // regex object
            };
            
            struct expression: x3::variant<
                nil,                // nil by default!
                regex_op,
                x3::forward_ast<unary_op>,
                x3::forward_ast<matcher_op>,
                x3::forward_ast<binary_op_group>> {
                using base_type::base_type;
                using base_type::operator=;
            };
            
            struct unary_op {
                op_type op;
                expression expr;
            };
            
            struct matcher_op {
                matcher_type matcher;
                expression expr;
            };
            
            struct binary_op_group {
                expression first;
                std::list<unary_op> rest;
            };
        }
    }
}

// Boost.Fusion adaption
BOOST_FUSION_ADAPT_STRUCT(
    riot::server::sfe_parser::ast::unary_op,
    op,
    expr
);

BOOST_FUSION_ADAPT_STRUCT(
    riot::server::sfe_parser::ast::matcher_op,
    matcher,
    expr
);

BOOST_FUSION_ADAPT_STRUCT(
    riot::server::sfe_parser::ast::binary_op_group,
    first,
    rest
);

// grammars
namespace riot::server {
    namespace sfe_parser {
        namespace x3 = boost::spirit::x3;
        namespace grammars {
            using x3::alpha;
            using x3::char_;
            using x3::lexeme;
            using x3::symbols;
            using x3::lit;
            using x3::eps;
            using x3::attr;
            using x3::_val;
            using x3::_attr;
            
            // operator parser
            struct op_: x3::symbols<ast::op_type> {
                ast::op_type op;
                
                op_(ast::op_type o, const std::string &name = ""):
                    x3::symbols<ast::op_type>(name), op(o) {  }
                
                op_ &operator()(const char *str) {
                    this->add(str, op);
                    return *this;
                }
            };
            
            // simple_regex_predicate
            namespace srp {
                /*  the following expressions are valid in this grammar:
                    *      'regex'
                    *      'regex1' | 'regex2'
                    *      'regex1' & ~'regex2'
                    *      'regex1' | 'regex2' & 'regex3' & ~'regex4'
                    *      ('regex1' | 'regex2') & ('regex3' | 'regex4')
                    *  etc.
                    * 
                    *  The operator precedence is as follows (excluding
                    *  parantheses):
                    * 
                    *  ~ > ^ > & > |
                    * 
                    *  ~ -> not; ^ -> xor; & -> and; | -> or
                    * 
                    *  This grammar extends the vast capabilities of regular
                    *  expressions hugely. However, 
                    * 
                    *  If the regex expression doesn't contain the special
                    *  symbols '~', '^', '&', '|', '(' or ')', then it doesn't
                    *  need to be quoted.
                    * 
                    *  There is no way to escape single-quote ''', so, do not
                    *  use it.
                    */
                
                struct expression_class;
                struct or_group_class;
                struct and_group_class;
                struct xor_group_class;
                struct primary_class;
                struct regex_class;
                
                // symbols
                static auto or_sym     = op_(ast::op_or, "|")  ("|");
                static auto and_sym    = op_(ast::op_and, "&") ("&");
                static auto xor_sym    = op_(ast::op_xor, "^") ("^");
                static auto neg_sym    = op_(ast::op_neg, "~") ("~");
                
                static x3::rule<expression_class,
                    ast::expression> const expression("srp_expression");
                static x3::rule<or_group_class,
                    ast::binary_op_group> const or_group("or_group");
                static x3::rule<and_group_class,
                    ast::binary_op_group> const and_group("and_group");
                static x3::rule<xor_group_class,
                    ast::binary_op_group> const xor_group("xor_group");
                static x3::rule<primary_class,
                    ast::expression> const primary("primary");
                static x3::rule<regex_class,
                    ast::regex_op> const regex("regex");
                
                static auto const expression_def = or_group;
                
                static auto const or_group_def =
                    and_group >> *(or_sym >> and_group);
                
                static auto const and_group_def =
                    xor_group >> *(and_sym >> xor_group);
                
                static auto const xor_group_def =
                    primary >> *(xor_sym >> primary);
                
                static auto const primary_def =
                    ('(' >> expression >> ')') |
                    (neg_sym >> primary) |
                    (regex);
                
                static auto const regex_action = [](auto &ctx) {
                    auto const &attr = _attr(ctx);
                    auto &val = _val(ctx);
                    val.str = attr;
                    val.rgx = std::regex{val.str};
                };
                
                /**
                    * Note to myself: this is where you can change the regex
                    * syntax. you can easily make quotes mandatory here
                    */
                static auto const regex_def =
                    lexeme[('\'' >> *(~char_("\'")) >> '\'')
                        [regex_action]] |
                    lexeme[ (+(~(char_("~^&|()!\'$# ")) - x3::space))
                        [regex_action] ];
                
                BOOST_SPIRIT_DEFINE(
                    expression,
                    or_group,
                    and_group,
                    xor_group,
                    primary,
                    regex
                )
            }
            
            // compund_expression
            namespace ce {
                /*  say A1, A2, ... are valid in srp grammar. Then, the
                    *  followings are valid in ce grammar:
                    * 
                    *      A1 $ A2             read as A1 from A2
                    *      A1 # A2             read as A1 from those having A2
                    *      A1 $ A2 # A3        read as A1 from A2 having A3
                    *      A1                  read as event A1
                    *      $ A1                read as from A1
                    *      A1                  read as at A1
                    *      A1 # A3             read as at A1 having A3
                    *
                    *  Those expressions can be used in logicals as well:
                    * 
                    *  
                    * 
                    */
                
                static auto const srp_expression = srp::expression;
                
                struct expression_class;
                struct or_group_class;
                struct and_group_class;
                struct list_and_class;
                struct xor_group_class;
                struct primary_class;
                struct prefixed_expression_class;
                
                struct and_sym_class;
                struct prefix_sym_class;
                
                // symbols
                static auto or_sym      = op_(ast::op_or, "||") ("||");
                static auto and_sym     = op_(ast::op_and, "&&")("&&");
                static auto xor_sym     = op_(ast::op_xor, "^^")("^^");
                static auto neg_sym     = op_(ast::op_neg, "!") ("!");
                
                static x3::rule<expression_class,
                    ast::expression> const expression("ce_expression");
                static x3::rule<or_group_class,
                    ast::binary_op_group> const or_group("OR_GROUP");
                static x3::rule<and_group_class,
                    ast::binary_op_group> const and_group("AND_GROUP");
                static x3::rule<xor_group_class,
                    ast::binary_op_group> const xor_group("XOR_GROUP");
                static x3::rule<primary_class,
                    ast::expression> const primary("PRIMARY");
                static x3::rule<prefixed_expression_class,
                    ast::matcher_op> const
                    prefixed_expression("prefixed_expression");
                
                static x3::rule<prefix_sym_class, ast::matcher_type> const
                    prefix_sym("$ or # or none");
                
                static auto const expression_def = or_group;
                
                static auto const or_group_def =
                    and_group >> *(or_sym >> and_group);
                
                static auto const and_group_def =
                    xor_group >>
                    *(
                        (and_sym >> xor_group) |
                        (attr(ast::op_and) >> xor_group)
                    );
                
                static auto const xor_group_def =
                    primary >> *(xor_sym >> primary);
                
                static auto const primary_def =
                    ('(' >> expression >> ')') |
                    (neg_sym >> primary) |
                    (prefixed_expression);
                
                static auto const prefixed_expression_def =
                    prefix_sym >> srp_expression;
                
                static auto const prefix_sym_def =
                    ("$" >> attr(ast::matcher1)) |
                    ("#" >> attr(ast::matcher2)) |
                    (eps >> attr(ast::matcher0));
                
                BOOST_SPIRIT_DEFINE(
                    expression,
                    or_group,
                    and_group,
                    xor_group,
                    primary,
                    prefixed_expression,
                    prefix_sym)
            }
        }
    }
}

// utilities
namespace riot::server {
    namespace sfe_parser {
        template <typename ConstIt>
        inline auto parse(
            ConstIt const begin, ConstIt const end) {
            auto it = begin;
            ast::expression expr;
            auto const parser = grammars::ce::expression;
            bool r = false;
            try {
                r = x3::phrase_parse(
                    it,
                    end,
                    parser,
                    x3::space,
                    expr);
            }
            catch (std::regex_error const &re) {
                std::ostringstream oss;
                oss << "[parse_sfep] regex error at " << (it-begin)
                    << " error message: " << re.what();
                throw std::runtime_error(oss.str());
            }
            if (!r)
                throw std::runtime_error(
                    "[parse_sfep] parser error at " +
                    std::to_string(it-begin));
            if (it != end)
                throw std::runtime_error(
                    "[parse_sfep] not consumed at " +
                    std::to_string(it-begin));
            return expr;
        };
        
        template <typename String>
        inline auto parse(const String &str) {
            return parse(std::cbegin(str), std::cend(str));
        }
        
        namespace detail {
            using namespace ast;
            
            struct printer: boost::static_visitor<void> {
                bool matcher_padding {true};
                
                printer(std::ostream &out_):
                    out{out_} {
                }
                
                void operator()(const ast::regex_op &ro) {
                    out << "'" << ro.str << "'";
                }
                
                void operator()(const matcher_op &mo) {
                    out << (matcher_padding ? " " : "")
                        << (char)mo.matcher;
                    inside_matcher = true;
                    boost::apply_visitor(*this, mo.expr);
                    inside_matcher = false;
                    out << (matcher_padding ? " " : "");
                }
                
                void operator()(const unary_op &uo) {
                    out << get_symbol(uo.op);
                    boost::apply_visitor(*this, uo.expr);
                }
                
                void operator()(const binary_op_group &bog) {
                    if (bog.rest.size() > 0) {
                        out << (inside_matcher ? '(' : '[');
                        boost::apply_visitor(*this, bog.first);
                        for (auto const &expr: bog.rest)
                            (*this)(expr);
                        out << (inside_matcher ? ')' : ']');
                    }
                    else {
                        boost::apply_visitor(*this, bog.first);
                    }
                }
                
                void operator()(const nil &) {
                    out << "<NIL>";
                }
                
            private:
                std::ostream &out;
                bool inside_matcher{false};
                
                std::string get_symbol(ast::op_type op) {
                    if (!inside_matcher) {
                        switch (op) {
                            case op_and:
                                return "&&";
                            case op_or:
                                return "||";
                            case op_xor:
                                return "^^";
                            case op_neg:
                                return "!";
                            default:
                                return "??";
                        }
                    }
                    else {
                        return std::string(1, (char)op);
                    }
                }
            };
            
            template <
                typename ConstListIt,
                bool NoSecondMatcher = false>
            struct evaluator: boost::static_visitor<bool> {
                template <typename String1, typename String2>
                evaluator(
                    String1 &&str1,
                    String2 &&str2,
                    ConstListIt strs_begin,
                    ConstListIt strs_end):
                    str1_{std::forward<String1>(str1)},
                    str2_{std::forward<String2>(str2)},
                    strs_begin_{strs_begin},
                    strs_end_{strs_end} {
                }
                
                template <typename String1>
                evaluator(
                    String1 &&str1,
                    ConstListIt strs_begin,
                    ConstListIt strs_end):
                    str1_{std::forward<String1>(str1)},
                    strs_begin_{strs_begin},
                    strs_end_{strs_end} {
                }
                
                bool operator()(const ast::regex_op &ro) {
                    switch (current_matcher_) {
                        case ast::matcher0 /* first */: {
                            // always there
                            return std::regex_match(str1_, ro.rgx);
                            break;
                        }
                        case ast::matcher1 /* second */: {
                            // this one is not always necessary
                            if constexpr (NoSecondMatcher) {
                                return true;    // always return true
                            }
                            else {
                                return std::regex_match(str2_, ro.rgx);
                            }
                        }
                        case ast::matcher2 /* third */: {
                            /**
                                * this one tries to match a bunch of a strings
                                * and returns true if any string matches
                                */
                            return std::find_if(
                                strs_begin_,
                                strs_end_,
                                [&ro](const auto &str) {
                                    return std::regex_match(str, ro.rgx);
                                }) != strs_end_;
                        }
                        default: return false;
                    }
                }
                
                bool operator()(const matcher_op &mo) {
                    current_matcher_ = mo.matcher;
                    return boost::apply_visitor(*this, mo.expr);
                }
                
                bool operator()(const unary_op &uo) {
                    // this can be only negation!
                    return ! boost::apply_visitor(*this, uo.expr);
                }
                
                bool operator()(const binary_op_group &bog) {
                    bool result = boost::apply_visitor(*this, bog.first);
                    for (auto &unary: bog.rest) {
                        bool next = boost::apply_visitor(*this, unary.expr);
                        switch (unary.op) {
                            case ast::op_and: {
                                result = (result && next);
                                break;
                            }
                            case ast::op_or: {
                                result = (result || next);
                                break;
                            }
                            case ast::op_xor: {
                                result = (result != next);
                            }
                            default: {
                                break;
                            }
                        }
                    }
                    return result;
                }
                
                bool operator()(const nil &) {
                    return true;
                }
                
            private:
                std::string str1_;
                
                // I <3 templates <3 <3 <3 <3 <3 <3 <3 
                std::conditional_t<
                    !NoSecondMatcher,
                    std::string, void *> str2_;
                
                ConstListIt strs_begin_;
                ConstListIt strs_end_;
                
                ast::matcher_type current_matcher_{ast::matcher0};
            };
        }
        
        using detail::printer;
        using detail::evaluator;
        
        template <
            typename String1,
            typename String2,
            typename ConstListIt>
        inline bool evaluate(
                const ast::expression &expr,
                String1 &&str1,
                String2 &&str2,
                ConstListIt strs_begin,
                ConstListIt strs_end
            ) {
            evaluator<ConstListIt, false> eval{
                std::forward<String1>(str1),
                std::forward<String2>(str2),
                strs_begin,
                strs_end};
            return boost::apply_visitor(eval, expr);
        }
        
        template <
            typename String1,
            typename ConstListIt>
        inline bool evaluate(
            const ast::expression &expr,
            String1 &&str1,
            ConstListIt strs_begin,
            ConstListIt strs_end
            ) {
            evaluator<ConstListIt, true> eval{
                std::forward<String1>(str1),
                strs_begin,
                strs_end};
            return boost::apply_visitor(eval, expr);
        }
    }
}

namespace riot::server {
    namespace sfe_parser {
        /**
            * sfep expression AST
            */
        using expression = ast::expression;
        
        /**
            * sfep expression parser
            */
        static auto const expression_ = grammars::ce::expression;
    }
}

inline std::ostream &operator<<(
    std::ostream &os,
    riot::server::sfe_parser::ast::expression const &expression) {
    using namespace riot::server::sfe_parser;
    printer p(os);
#ifdef RIOT_SERVER_SFEP_NO_MATCHER_PADDING
    p.matcher_padding = false;
#else
    p.matcher_padding = true;
#endif
    boost::apply_visitor(p, expression);
    return os;
}

#endif // RIOT_SERVER_SFE_PARSER_INCLUDED
