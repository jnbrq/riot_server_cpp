/**
 * @author Canberk Sönmez
 * 
 * @date Tue Jul 10 05:10:50 +03 2018
 * 
 */

#ifdef RIOT_SERVER_PARSER_SEPARATELY_COMPILED

#define RIOT_SERVER_THIS_IS_A_PARSER_TRANSLATION_UNIT

#include <riot/server/parsers/command.hpp>
#include <riot/server/parsers/header.hpp>
#include <riot/server/parsers/sfe.hpp>

// what if any macro names below are already used? so unlikely.

#define GENERIC_IMPLEMENTATION(return_type, argument_type) \
    return_type parse( \
        argument_type ::const_iterator a, \
        argument_type ::const_iterator b) \
    { return templated::parse(a, b); } \
     \
    return_type parse(const argument_type &a) \
    { return templated::parse(a); }

#define STD_VECTOR_CHAR_IMPLEMENTATION(return_type) \
    GENERIC_IMPLEMENTATION(return_type, std::vector<char>)

#define STD_STRING_IMPLEMENTATION(return_type) \
    GENERIC_IMPLEMENTATION(return_type, std::string)

#define IMPLEMENT_PARSER(parser, return_type) \
    namespace riot::server { namespace parser { \
        STD_VECTOR_CHAR_IMPLEMENTATION(return_type) \
        STD_STRING_IMPLEMENTATION(return_type) \
    }}

IMPLEMENT_PARSER(parsers::sfe, ast::expression)
IMPLEMENT_PARSER(parsers::command, command)
IMPLEMENT_PARSER(parsers::header, entry)

#endif
