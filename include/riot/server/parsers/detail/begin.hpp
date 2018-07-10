/**
 * @author Canberk Sönmez
 * 
 * @date Tue Jul 10 04:49:12 +03 2018
 * 
 */

/**
 * This file tries to decrease compilation times by conditionally including
 * templated code. The code is included only when the translation unit holding
 * the parser code is being generated.
 */

/**
 * No include guards!
 */

#if !defined(RIOT_SERVER_PARSER_SEPARATELY_COMPILED) || \
    defined(RIOT_SERVER_PARSER_SEPARATELY_COMPILED) && \
    defined(RIOT_SERVER_THIS_IS_A_PARSER_TRANSLATION_UNIT)

#define RIOT_SERVER_PARSER_CONTINUE

#endif
