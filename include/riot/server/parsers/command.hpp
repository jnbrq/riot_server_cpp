/**
 * @author Canberk Sönmez
 * 
 * @date Sun Jun 24 14:11:55 +03 2018
 * 
 */

#ifndef RIOT_SERVER_COMMAND_PARSER_INCLUDED
#define RIOT_SERVER_COMMAND_PARSER_INCLUDED

#include <vector>
#include <string>
#include <iostream>

#include <boost/variant.hpp>
#include <boost/optional.hpp>

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>

#include <boost/fusion/include/adapt_struct.hpp>
#include <riot/server/parsers/sfe.hpp>

/**
 * Error Handling? I do not want error handling. However, I might implement it
 * in future. Also, consider HTTP. When you make a bad request, it only says
 * you made a bad request. It doesn't mention any details. So, for an IoT
 * protocol why should I do that?
 */

namespace riot::server {
    namespace parsers::command {
        namespace x3 = boost::spirit::x3;
        namespace cmd {
            struct nil {  };
            
            struct subscribe {
                parsers::sfe::expression expr;
            };
            
            struct unsubscribe {
                std::size_t n;
            };
            
            struct trigger {
                std::string evt;
                boost::optional<parsers::sfe::expression> expr;
            };
            
            struct trigger_binary {
                std::size_t size;
                std::string evt;
                boost::optional<parsers::sfe::expression> expr;
            };
            
            struct trigger_empty {
                std::string evt;
                boost::optional<parsers::sfe::expression> expr;
            };
            
            struct trigger_cached {
                std::string evt;
                std::size_t expr_id;
            };
            
            struct trigger_cached_binary {
                std::size_t size;
                std::string evt;
                std::size_t expr_id;
            };
            
            struct trigger_cached_empty {
                std::string evt;
                std::size_t expr_id;
            };
            
            struct trigger_cached_cached_data {
                std::string evt;
                std::size_t expr_id;
                std::size_t data_id;
            };
            
            struct pause {
                
            };
            
            struct resume {
                
            };
            
            struct alive {
                
            };
            
            struct kill_me {
                
            };
            
            struct echo {
                boost::optional<bool> state;
            };
            
            struct execute {
                std::string line;
            };
            
            struct execute_script {
                std::size_t size;
            };
            
            struct execute_cached {
                std::size_t id;
            };
            
            struct store {
                std::vector<char> line;
            };
            
            struct store_binary {
                std::size_t size;
            };
            
            struct release {
                std::size_t id;
            };
        }
        
        using command = x3::variant<
            // cmd::nil, // TODO compiler error here, find out?
            cmd::subscribe,
            cmd::unsubscribe,
            cmd::trigger,
            cmd::trigger_binary,
            cmd::trigger_empty,
            cmd::trigger_cached,
            cmd::trigger_cached_binary,
            cmd::trigger_cached_empty,
            cmd::trigger_cached_cached_data,
            cmd::pause,
            cmd::resume,
            cmd::alive,
            cmd::kill_me,
            cmd::echo,
            cmd::execute,
            cmd::execute_script,
            cmd::execute_cached,
            cmd::store,
            cmd::store_binary,
            cmd::release>;
    }
}

BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::nil
);

BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::subscribe,
    expr
);

BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::unsubscribe,
    n
);

BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::trigger,
    evt,
    expr
);

BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::trigger_binary,
    size,
    evt,
    expr
);

BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::trigger_empty,
    evt,
    expr
);

BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::trigger_cached,
    evt,
    expr_id
);


BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::trigger_cached_binary,
    size,
    evt,
    expr_id
);


BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::trigger_cached_empty,
    evt,
    expr_id
);


BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::trigger_cached_cached_data,
    evt,
    expr_id,
    data_id
);


BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::pause
);


BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::resume
);


BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::alive
);


BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::kill_me
);


BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::echo,
    state
);


BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::execute,
    line
);


BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::execute_script,
    size
);


BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::execute_cached,
    id
);


BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::store,
    line
);


BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::store_binary,
    size
);


BOOST_FUSION_ADAPT_STRUCT(
    riot::server::parsers::command::cmd::release,
    id
);

#include <riot/server/parsers/detail/begin.hpp>
#ifdef RIOT_SERVER_PARSER_CONTINUE
namespace riot::server {
    namespace parsers::command {
        namespace grammar {
            namespace x3 = boost::spirit::x3;
            
            using x3::char_;
            using x3::bool_;
            using x3::lexeme;
            using x3::lit;
            using x3::attr;
            
            using x3::uint_parser;
            // size_t parser
            using size_t_parser_type = uint_parser<std::size_t>;
            size_t_parser_type size_t_; // for sizes and numerical ids
            
            struct command_class;
            struct cmd_subscribe_class;
            struct cmd_unsubscribe_class;
            struct cmd_trigger_class;
            struct cmd_trigger_binary_class;
            struct cmd_trigger_empty_class;
            struct cmd_trigger_cached_class;
            struct cmd_trigger_cached_binary_class;
            struct cmd_trigger_cached_empty_class;
            struct cmd_trigger_cached_cached_data_class;
            struct cmd_pause_class;
            struct cmd_resume_class;
            struct cmd_alive_class;
            struct cmd_kill_me_class;
            struct cmd_echo_class;
            struct cmd_execute_class;
            struct cmd_execute_script_class;
            struct cmd_execute_cached_class;
            struct cmd_store_class;
            struct cmd_store_binary_class;
            struct cmd_release_class;
            
            struct identifier_class;
            
            static x3::rule<command_class, command>
                const
                command("command");
            
            static x3::rule<cmd_subscribe_class, cmd::subscribe>
                const
                cmd_subscribe("cmd_subscribe");
            
            static x3::rule<cmd_unsubscribe_class, cmd::unsubscribe>
                const
                cmd_unsubscribe("cmd_unsubscribe");
            
            static x3::rule<cmd_trigger_class, cmd::trigger>
                const
                cmd_trigger("cmd_trigger");
            
            static x3::rule<cmd_trigger_binary_class, cmd::trigger_binary>
                const
                cmd_trigger_binary("cmd_trigger_binary");
            
            static x3::rule<cmd_trigger_empty_class, cmd::trigger_empty>
                const
                cmd_trigger_empty("cmd_trigger_empty");
            
            static x3::rule<cmd_trigger_cached_class, cmd::trigger_cached>
                const
                cmd_trigger_cached("cmd_trigger_cached");
            
            static x3::rule<
                cmd_trigger_cached_binary_class,
                cmd::trigger_cached_binary>
                const
                cmd_trigger_cached_binary("cmd_trigger_cached_binary");
            
            static x3::rule<
                cmd_trigger_cached_empty_class,
                cmd::trigger_cached_empty>
                const
                cmd_trigger_cached_empty("cmd_trigger_cached_empty");
            
            static x3::rule<
                cmd_trigger_cached_cached_data_class,
                cmd::trigger_cached_cached_data>
                const
                cmd_trigger_cached_cached_data(
                    "cmd_trigger_cached_cached_data");
            
            static x3::rule<cmd_pause_class, cmd::pause>
                const
                cmd_pause("cmd_pause");
            
            static x3::rule<cmd_resume_class, cmd::resume>
                const
                cmd_resume("cmd_resume");
            
            static x3::rule<cmd_alive_class, cmd::alive>
                const
                cmd_alive("cmd_alive");
            
            static x3::rule<cmd_kill_me_class, cmd::kill_me>
                const
                cmd_kill_me("cmd_kill_me");
            
            static x3::rule<cmd_echo_class, cmd::echo>
                const
                cmd_echo("cmd_echo");
            
            static x3::rule<cmd_execute_class, cmd::execute>
                const
                cmd_execute("cmd_execute");
            
            static x3::rule<cmd_execute_script_class, cmd::execute_script>
                const
                cmd_execute_script("cmd_execute_script");
            
            static x3::rule<cmd_execute_cached_class, cmd::execute_cached>
                const
                cmd_execute_cached("cmd_execute_cached");
            
            static x3::rule<cmd_store_class, cmd::store>
                const
                cmd_store("cmd_store");
            
            static x3::rule<cmd_store_binary_class, cmd::store_binary>
                const
                cmd_store_binary("cmd_store_binary");
            
            static x3::rule<cmd_release_class, cmd::release>
                const
                cmd_release("cmd_release");
            
            // a simple string identifier
            static x3::rule<identifier_class, std::string>
                const identifier("identifier");
            
            
            // if you examine the rules for the expressions below, the
            // keywords of the commands requiring arguments end with a 
            // space. this is very important! whereas the single keyword
            // commands do not exhibit that. put the commands-with-
            // arguments higher in the following list of |s !
            // also, echo should be last as exception
            // 
            // [why? since, suppose, "release", if you do not obey this
            // rule, parser recognises "r" only which is resume and
            // ultimately fails!]
            static auto const command_def =
                cmd_subscribe |
                cmd_unsubscribe |
                cmd_trigger |
                cmd_trigger_binary |
                cmd_trigger_empty |
                cmd_trigger_cached |
                cmd_trigger_cached_binary |
                cmd_trigger_cached_empty |
                cmd_trigger_cached_cached_data |
                cmd_execute |
                cmd_execute_script |
                cmd_execute_cached |
                cmd_store |
                cmd_store_binary |
                cmd_echo |
                cmd_release |
                cmd_pause |
                cmd_resume |
                cmd_alive |
                cmd_kill_me;
            
            static auto const cmd_subscribe_def =
                (
                    lit("subscribe ") |
                    lit("subs ") |
                    lit("s10n") |
                    lit("s ")
                ) >> parsers::sfe::expression_;
            
            static auto const cmd_unsubscribe_def =
                (
                    lit("unsubscribe ") |
                    lit("unsubs ") |
                    lit("usubs ") |
                    lit("us10n ") |
                    lit("us ")
                ) >> size_t_;
            
            static auto const cmd_trigger_def =
                (
                    lit("trigger ") |
                    lit("trig ") |
                    lit("t ")
                ) >> identifier >> -parsers::sfe::expression_;
            
            static auto const cmd_trigger_binary_def =
                (
                    lit("triggerb ") |
                    lit("trigb ") |
                    lit("tb ")
                ) >> size_t_ >> identifier >> -parsers::sfe::expression_;
            
            static auto const cmd_trigger_empty_def =
                (
                    lit("triggere ") |
                    lit("trige ") |
                    lit("te ") |
                    lit("notify ") |
                    lit("notif ") |
                    lit("n ")
                ) >> identifier >> -parsers::sfe::expression_;
            
            static auto const cmd_trigger_cached_def =
                (
                    lit("triggerc ") |
                    lit("trigc ") |
                    lit("tc ")
                ) >> identifier >> size_t_;
            
            static auto const cmd_trigger_cached_binary_def =
                (
                    lit("triggercb ") |
                    lit("trigcb ") |
                    lit("tcb ")
                ) >> size_t_ >> identifier >> size_t_;
            
            static auto const cmd_trigger_cached_empty_def =
                (
                    lit("triggerce ") |
                    lit("trigce ") |
                    lit("tce ")
                ) >> identifier >> size_t_;
            
            static auto const cmd_trigger_cached_cached_data_def =
                (
                    lit("triggerccd ") |
                    lit("trigccd ") |
                    lit("tccd ")
                ) >> identifier >> size_t_ >> size_t_;
            
            static auto const cmd_pause_def =
                (
                    lit("pause") |
                    lit("p")
                );
            
            static auto const cmd_resume_def =
                (
                    lit("resume") |
                    lit("r")
                );
            
            static auto const cmd_alive_def =
                (
                    lit("alive") |
                    lit("idle") |
                    lit("a") |
                    lit("i")
                );
            
            static auto const cmd_kill_me_def =
                (
                    lit("kill-me") |
                    lit("k")
                );
            
            static auto const cmd_echo_def =
                (
                    lit("echo") |
                    lit("e")
                ) >> -bool_;
            
            static auto const cmd_execute_def =
                (
                    lit("execute ") |
                    lit("exec ") |
                    lit("x ")
                ) >> lexeme[+char_] /* we want pre-skipping, so lexeme */;
            
            static auto const cmd_execute_script_def =
                (
                    lit("script ") |
                    lit("sc ")
                ) >> size_t_;
            
            static auto const cmd_execute_cached_def =
                (
                    lit("executec ") |
                    lit("execc") |
                    lit("xc ")
                ) >> size_t_;
            
            static auto const cmd_store_def =
                (
                    lit("store ") |
                    lit("st ")
                ) >> lexeme[+char_];
            
            static auto const cmd_store_binary_def =
                (
                    lit("storeb ") |
                    lit("stb ")
                ) >> size_t_;
            
            static auto const cmd_release_def =
                (
                    lit("release ") |
                    lit("rl ")
                ) >> size_t_;
            
            static auto const identifier_def =
                lexeme[+char_("a-zA-Z0-9_")];
            
            BOOST_SPIRIT_DEFINE(
                command,
                cmd_subscribe,
                cmd_unsubscribe,
                cmd_trigger,
                cmd_trigger_binary,
                cmd_trigger_empty,
                cmd_trigger_cached,
                cmd_trigger_cached_binary,
                cmd_trigger_cached_empty,
                cmd_trigger_cached_cached_data,
                cmd_pause,
                cmd_resume,
                cmd_alive,
                cmd_kill_me,
                cmd_echo,
                cmd_execute,
                cmd_execute_script,
                cmd_execute_cached,
                cmd_store,
                cmd_store_binary,
                cmd_release,
                identifier
            );
        };
    };
}

namespace riot::server {
    namespace parsers::command {
        static auto const command_ = grammar::command;
    }
}

namespace riot::server {
    namespace parsers::command {
        namespace templated {
            template <typename ConstIt>
            inline auto parse(
                ConstIt const begin, ConstIt const end) {
                auto it = begin;
                command cmd;
                auto const parser = command_;
                bool r = false;
                try {
                    r = x3::phrase_parse(
                        it,
                        end,
                        parser,
                        x3::space,
                        cmd);
                }
                catch (std::exception const &ex) {
                    std::ostringstream oss;
                    oss << "[parse_command] unexpected throw at " << (it-begin)
                        << " error message: " << ex.what();
                    throw std::runtime_error(oss.str());
                }
                if (!r)
                    throw std::runtime_error(
                        "[parse_command] parser error at " +
                        std::to_string(it-begin));
                if (it != end)
                    throw std::runtime_error(
                        "[parse_command] not consumed at " +
                        std::to_string(it-begin));
                return cmd;
            };
            
            template <typename String>
            inline auto parse(const String &str) {
                return parse(std::cbegin(str), std::cend(str));
            }
        }
        
        using templated::parse;
    }
}
#endif
#include <riot/server/parsers/detail/end.hpp>

#ifdef RIOT_SERVER_PARSER_SEPARATELY_COMPILED
namespace riot::server {
    namespace parsers::command {
        command parse(
            std::string::const_iterator,
            std::string::const_iterator);
        command parse(const std::string &);
        
        command parse(
            std::vector<char>::const_iterator,
            std::vector<char>::const_iterator);
        command parse(const std::vector<char> &);
    }
}
#endif

namespace riot::server {
    namespace parsers::command {
        struct printer: boost::static_visitor<void> {
            std::ostream &out;
            
            printer(std::ostream &out_): out{out_} {
                
            }
            
            void operator()(const cmd::subscribe &c) {
                out << "subscribe " << c.expr;
            }
            
            void operator()(const cmd::unsubscribe &c) {
                out << "unsubscribe " << c.n;
            }
            
            void operator()(const cmd::trigger &c) {
                out << "trigger " << c.evt << " "
                    << c.expr.value_or(true_expr);
            }
            
            void operator()(const cmd::trigger_binary &c) {
                out << "triggerb " << c.size << " " << c.evt << " "
                    << c.expr.value_or(true_expr);
            }
            
            void operator()(const cmd::trigger_empty &c) {
                out << "triggere " << c.evt << " "
                    << c.expr.value_or(true_expr);
            }
            
            void operator()(const cmd::trigger_cached &c) {
                out << "triggerc " << c.evt << " " << c.expr_id;
            }
            
            void operator()(const cmd::trigger_cached_binary &c) {
                out << "triggercb " << c.size << " " << c.evt << " "
                    << c.expr_id;
            }
            
            void operator()(const cmd::trigger_cached_empty &c) {
                out << "triggerce " << c.evt << " " << c.expr_id;
                
            }
            
            void operator()(const cmd::trigger_cached_cached_data &c) {
                out << "triggerccd "
                    << c.evt << " " << c.expr_id << " " << c.data_id;
            }
            
            void operator()(const cmd::pause &c) {
                out << "pause";
            }
            
            void operator()(const cmd::resume &c) {
                out << "resume";
            }
            
            void operator()(const cmd::alive &c) {
                out << "alive";
            }
            
            void operator()(const cmd::kill_me &c) {
                out << "kill-me";
            }
            
            void operator()(const cmd::echo &c) {
                out << "echo ";
                if (c.state) {
                    out << std::boolalpha << c.state.value();
                }
                else {
                }
            }
            
            void operator()(const cmd::execute &c) {
                out << "execute " << c.line;
            }
            
            void operator()(const cmd::execute_script &c) {
                out << "script " << c.size;
            }
            
            void operator()(const cmd::execute_cached &c) {
                out << "executec " << c.id;
            }
            
            void operator()(const cmd::store &c) {
                out << "store ";
                for (auto const &cc: c.line)
                    out << cc;
            }
            
            void operator()(const cmd::store_binary &c) {
                out << "storeb " << c.size;
            }
            
            void operator()(const cmd::release &c) {
                out << "release " << c.id;
            }
            
            void operator()(const cmd::nil &) {
                
            }
        private:
            parsers::sfe::expression true_expr{parsers::sfe::parse(".*")};
        };
    }
}

inline std::ostream &operator<<(
    std::ostream &out,
    const riot::server::parsers::command::command &cmd) {
    using namespace riot::server;
    parsers::command::printer p{out};
    boost::apply_visitor(p, cmd);
    return out;
}

#endif // RIOT_SERVER_COMMAND_PARSER_INCLUDED
