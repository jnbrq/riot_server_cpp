/**
 * @author Canberk Sönmez
 * 
 * @date Thu Jul  5 23:29:13 +03 2018
 * 
 */

/**
 * riot_server_cpp is not designed to allow multiple threading. Therefore, the
 * following code SHOULD NOT work reliably if io_ctx.run() is called by 
 * multiple threads [with an assigned work, of course]. Why not using
 * multithreading?
 *      - Multithreading would definitely yield better results if the threads
 *        do not need to interact with each other. However, in our case, the
 *        operation between connections require interaction between different
 *        threads, ultimately, requiring synchronization and making
 *        multithreading less feasable.
 * 
 *      - Multithreading requires speacial threatment in code. Boost.ASIO
 *        provides "strand"s for simple implicit locking. However, using that
 *        means extra calls to strand::post and strand::wrap in the code. It
 *        makes the code harder to read and much less maintainable.
 * 
 */

/**
 * A few notes notes regarding the usage of connection_base:
 * 
 *      - do_async_read_binary
 *      - do_async_read_message
 *      - do_async_write
 * 
 * When those functions are used directly, their handlers should increase the
 * ref count of the connection object. For that purpose, you can use
 * RIOT_KEEP_CONN or RIOT_KEEP_CONN_EX macros. The usage of do_async_write
 * should be avoided. Use enqueue_async_write instead, which automatically
 * maintains a write queue, so that consequent write operations do not interfere
 * each other.
 * 
 */

/**
 * No multithreading!
 */

#ifndef RIOT_SERVER_CONNECTION_BASE_INCLUDED
#define RIOT_SERVER_CONNECTION_BASE_INCLUDED

#include <riot/server/parsers/header.hpp>
#include <riot/server/parsers/command.hpp>
#include <riot/server/security_actions.hpp>
#include <riot/server/artifacts.hpp>
#include <riot/mpl/is_specialization.hpp>
#include <sstream>
#include <memory>
#include <vector>
#include <list>
#include <utility>
#include <functional>
#include <iterator>
#include <iomanip>
#include <algorithm>
#include <iostream>
#include <type_traits>
#include <limits>
#include <string_view>
#include <optional>
#include <boost/core/ignore_unused.hpp>
#include <boost/variant.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string/trim.hpp>

namespace riot::server {

#define RIOT_INC_REF_OF(x)      _ref = (x).shared_from_this()
#define RIOT_INC_REF            RIOT_INC_REF_OF(*this)
#define RIOT_KEEP_CONN          RIOT_INC_REF
#define RIOT_KEEP_CONN_EX(x)    RIOT_INC_REF_OF(x)

template <typename ConnectionManager>
struct connection_base:
    std::enable_shared_from_this<connection_base<ConnectionManager>> {
    
    struct subscription {
        std::size_t n;
        parsers::sfe::expression expr;
        
        subscription(std::size_t n_, parsers::sfe::expression expr_):
            n(n_), expr(std::move(expr_)) {
        }
        
        subscription() {}
    };
    
    struct event: std::enable_shared_from_this<event> {
        enum trigger_type_t {
            trigger_line,
            trigger_binary,
            trigger_empty
        } trigger_type;
        const connection_base * sender;
        std::string evt;
        parsers::sfe::expression expr;
        std::vector<char> data; // should have trailing new line!
        
        event() {}
        
        event(
            const connection_base *sender_,
            trigger_type_t tt_,
            std::string evt_ /* pass by value to store! */,
            parsers::sfe::expression expr_ /* same */):
            trigger_type{tt_},
            sender{sender_},
            evt{std::move(evt_)},
            expr{std::move(expr_)} {
        }
        
        event(
            const connection_base *sender_,
            trigger_type_t tt_,
            std::string evt_):
            trigger_type{tt_},
            sender{sender_},
            evt{std::move(evt_)} {
        }
    };
    
    using simple_callback_t = std::function<void (bool)>;
    using next_cmd_callback_t = std::function<void (std::string, bool)>;
    
    /**
     * @brief Constructs a connection_base object.
     * 
     * @param connman_ Pointer to the connection manager object. It must
     * be valid throughout the operation of the connection.
     */
    connection_base(ConnectionManager &connman_):
        conn_man {connman_} {
    }
    
    enum write_mode_t {
        write_text = 0,
        write_binary
    };
    
    /**
     * @brief Enqueues a write operation to the connection. The
     * consequent calls to this routine does not cause any problems.
     * 
     * "this" is guaranteed to be valid when completed. (so no need for
     * increasing ref-count for "this" manually)
     * 
     * Please note that the sequential calls to this function should not cause
     * any problems, since after the first call, "write_queue_.size() > 1"
     * for the following calls, so async_write_next() is not called
     * repeatedly.
     * 
     * @param data A non-owning pointer to the data. It should be valid
     * until the end of the write operation.
     * @param size Size of the data.
     * @param callback Called when the write operation is over. Its
     * parameter is true if successful.
     * @param write_mode Informs underlying protocol about the content.
     * @param start_later The async_write_next() function is not
     * called if it is false. Therefore, if there is no current writing
     * sequence, the write operation of the newly enqueued data doesn't
     * start.
     */
    void enqueue_async_write(
        const char *data,
        std::size_t size,
        simple_callback_t callback = nullptr,
        write_mode_t write_mode = write_text) {
        // if I were to write multithreaded code, I would need to
        // enclose this code with strand_.post([this, ...] { ... })
        write_queue_.emplace_back(
            data,
            size,
            std::move(callback),
            write_mode);
        if (write_queue_.size() > 1) {
            // we already have an ongoing write operation, so move on
            return ;
        }
        async_write_next();
    }
    
    /**
     * @brief A simpler interface for enqueue_async_write. Please note
     * that str object is copied, therefore, callback does not need
     * to hold a reference to str.
     */
    template <typename Callback>
    void enqueue_async_write(
        std::string const& str,
        Callback &&callback,
        write_mode_t write_mode = write_text) {
        auto buffer_size = str.size();
        auto buffer = std::make_shared<std::vector<char>>(buffer_size);
        auto buffer_ptr = &(buffer->front());
        std::copy(std::begin(str), std::end(str), buffer_ptr);
        enqueue_async_write(
            buffer_ptr,
            buffer_size,
            [buffer, _f = std::forward<Callback>(callback)](bool res) {
                _f(res);
            },
            write_mode);
    }
    
    /**
     * @brief Even a simpler interface. Callback is omitted completely.
     */
    void enqueue_async_write(
        std::string const& str,
        write_mode_t write_mode = write_text) {
        enqueue_async_write(
            str,
            [](bool) {  },
            write_mode);
    }
    
    /**
     * @brief Checks the given evt object if
     *      - the given event is subscribed
     *      - the given event is targeted this device
     * and enqueues writing it.
     * 
     * evt.sender must be valid when this routine is executed
     * 
     * @param evt event object.
     */
    void trigger(std::shared_ptr<const event> const &evt) {
        // do not do anything, if sender is this
        if (evt->sender == this) // maybe this might be specialized?
            return ;
        
        // do not send if the connection is paused
        if (paused)
            return ;
 
        // first, evaluate the sfe of evt object
        if (!parsers::sfe::evaluate(
            evt->expr, name, groups.begin(), groups.end()))
            return ;
        
        std::ostringstream ssheader;
        /* header format: 
         *      evt EVT NAME N1 N2 ....\n
         *      evtb SZ EVT NAME N1 N2 ....\n
         *      evte EVT NAME N1 N2 ....\n
         */
        switch (evt->trigger_type) {
        case event::trigger_line: {
            ssheader << "el ";
            break;
        }
        case event::trigger_binary: {
            ssheader << "eb " << evt->data.size() << " ";
            break;
        }
        case event::trigger_empty: {
            ssheader << "ee ";
            break;
        }
        }
        
        ssheader << evt->evt << " " << evt->sender->name << " ";
        
        // second, let's see if the event sender and the event matches
        // any of our subscriptions
        bool any_match = false;
        std::list<std::size_t> matching_subscriptions;
        for (auto &subscription: subscriptions) {
            if (parsers::sfe::evaluate(
                subscription.expr,
                evt->evt,
                evt->sender->name,
                evt->sender->groups.begin(),
                evt->sender->groups.end())) {
                ssheader << subscription.n << " ";
                any_match = true;
            }
        }
        
        if (!any_match)
            return ;
        
        // check if the server allows this triggering
        if (!get_artifact(
                    artifacts::can_receive_event<connection_base>{evt.get()}))
            return ;
        
        if (send_trailing_newline)
            ssheader << "\n";
        
        switch (evt->trigger_type) {
        case event::trigger_line: {
            enqueue_async_write(ssheader.str(), write_text);
            enqueue_async_write(
                &(evt->data.front()),
                evt->data.size(),
                [evt /* INCREF */](bool) {  },
                write_text);
            break;
        }
        case event::trigger_binary: {
            enqueue_async_write(ssheader.str(), write_text);
            enqueue_async_write(
                &(evt->data.front()),
                evt->data.size(),
                [evt](bool) {  },
                write_binary); 
            break;
        }
        case event::trigger_empty: {
            enqueue_async_write(ssheader.str(), write_text);
            break;  // never forget :/
        }
        }
    }
    
    /**
     * @brief Starts the connection by triggering reading the next msg.
     */
    void async_start() {
        do_set_async_read_message_max_size(
            get_artifact(artifacts::header_message_max_size {}));
        async_read_next_message();
    }
    
    ConnectionManager &conn_man;
    
    // to use "std::string_view"s we need to use a transparent comparator
    // https://stackoverflow.com/questions/35525777/use-of-string-view-for-map-lookup
    std::map<std::string, std::list<std::string>, std::less<>> properties;
    
    std::string name;
    std::string password;
    std::list<std::string> groups;
    
    std::list<subscription> subscriptions;
    std::map<std::size_t, std::vector<char>> local_storage;
    std::map<std::size_t, parsers::sfe::expression> expression_cache;
    
    // should have both protocol and host (only for demostrative purposes)
    std::string endpoint_str;
    
    // to identify exactly which server has created this connection
    std::size_t server_id{std::numeric_limits<std::size_t>::max()};
    
    bool paused{false};
    bool echo{true};
    
    /**
     * @brief Message-oriented protocols might prefer setting this false.
     */
    bool send_trailing_newline{true};
    
    std::optional<std::string> get_property_first(std::string_view sv) {
        auto it = properties.find(sv);
        if (it != properties.end()) {
            // by definition, we should have at least one property value
            return it->second.front();
        }
        return std::nullopt;
    }
    
    virtual ~connection_base() {
        // when a connection object is destructed, its weak_ptr should be
        // removed
        
        constexpr auto vector_used =
            mpl::is_specialization_v<
                decltype(conn_man.connections),
                std::vector>;
        constexpr auto list_used =
            mpl::is_specialization_v<
                decltype(conn_man.connections),
                std::list>;
        
        static_assert(
            list_used || vector_used,
            "expected: std::vector<std::weak_ptr<...>> or "
            "std::list<std::weak_ptr<...>> for storing connections.");
        
        if constexpr (vector_used) {
            // efficiently implemented this by swapping the element to be 
            // removed with the back. please note that this does not maintain
            // the order, though.
            boost::asio::post(conn_man.io_context, [&] {
                // TODO test the following code
                // https://en.cppreference.com/w/cpp/container/vector/erase
                // https://stackoverflow.com/questions/30611584/how-to-efficien
                // tly-delete-elements-from-vector-c
                for (
                    auto it = conn_man.connections.begin();
                    it != conn_man.connections.end(); ) {
                    if (it->expired()) {
                        *it = std::move(conn_man.connections.back());
                        conn_man.connections.pop_back();
                    }
                    else ++it;
                }
                std::cout <<
                    "Cleanup: number of active connections: " << 
                    conn_man.connections.size() << std::endl;
            });
        }
        else /* if (list_used) */ {
            boost::asio::post(conn_man.io_context, [&] {
                conn_man.connections.remove_if(
                    [] (auto & r) -> bool { return r.expired(); });
                std::cout <<
                    "Cleanup: number of active connections: " << 
                    conn_man.connections.size() << std::endl;
            });
        }
    }
    
private:
#define RIOT_BEGIN_ERROR_CASE(prefix, action, error_code) \
    if ((action) & security_actions::action_not_allowed) { \
        if ((action) & security_actions::_action_raise_error) { \
            (prefix).send_error((error_code)); \
        } \
        else if ((action) & security_actions::_action_raise_warning) { \
            (prefix).send_warning((error_code)); \
        } \
        else { \
            (prefix).send_ok(); \
        }

#define RIOT_END_ERROR_CASE(prefix, action, error_code) \
        if ((action) & security_actions::_action_halt) { \
            return ; \
        } \
        else if ((action) & security_actions::_action_freeze) { \
            (prefix).freeze((error_code)); \
        } \
        \
        if ((action) & security_actions::_action_block) { \
            (prefix).do_block_endpoint(); \
        } \
    }

#define RIOT_HANDLE_ERROR_CASE(prefix, action, error_code) \
    RIOT_BEGIN_ERROR_CASE(prefix, action, error_code); \
    RIOT_END_ERROR_CASE(prefix, action, error_code);
    
    struct command_handler: boost::static_visitor<void> {
        connection_base &conn;
        ConnectionManager &conn_man;
        
        command_handler(connection_base &conn_):
            conn{conn_}, conn_man{conn_.conn_man} {
        }
        
        template <typename T>
        auto get_artifact(T &&t) {
            return conn.get_artifact(std::forward<T>(t));
        }
        
        template <typename T>
        auto get_security_action(T &&t) {
            return conn.get_security_action(std::forward<T>(t));
        }
        
        void operator()(parsers::command::cmd::subscribe &c) {
            auto nmax = std::max_element(
                std::begin(conn.subscriptions),
                std::end(conn.subscriptions),
                [](auto &a, auto &b) { return a.n < b.n; });
            auto n = nmax->n + 1;
            conn.subscriptions.emplace_back(n, std::move(c.expr));
            conn.send_text("ok ", /* std::setw(10), std::setfill('0'), */ n);
            conn.async_read_next_message();
        }
        
        void operator()(parsers::command::cmd::unsubscribe &c) {
            using namespace security_actions;
            auto it = std::find_if(
                std::begin(conn.subscriptions),
                std::end(conn.subscriptions),
                [&](auto &a) { return a.n == c.n; });
            if (it != std::end(conn.subscriptions)) {
                // found :)
                conn.subscriptions.erase(it);
                conn.send_ok();
            }
            else {
                // ErrorCase: Invalid Argument
                auto action = get_security_action(invalid_argument{});
                RIOT_HANDLE_ERROR_CASE(conn, action, err_cmd_invalid_arg);
            }
            conn.async_read_next_message();
        }
        
        template <typename Event, typename F>
        void trigger_common(Event &&e, F &&f) {
            if (get_artifact(artifacts::can_trigger_event{e->evt})) {
                f();
            }
            else {
                // ErrorCase: Trigger Prohibited
                auto security_action =
                    security_actions::trigger_prohibited{e->evt};
                auto action = get_security_action(security_action);
                RIOT_HANDLE_ERROR_CASE(conn, action, err_trigger_prohibited);
                conn.async_read_next_message();
            }
        }
        
        template <typename Event>
        void trigger_line_common(Event &&evt) {
            trigger_common(std::forward<Event>(evt), [&] {
                conn.send_ok();
                conn.do_async_read_message(
                    [&, evt /* inc refc */, RIOT_KEEP_CONN_EX(conn)](
                        const std::string &str, bool res) {
                        if (!res) {
                            return ;
                        }
                        conn.reset_idle_counter();                        
                        conn.send_ok();
                        
                        // str is what we are after basically
                        evt->data = std::vector<char>(
                            str.size() + (conn.send_trailing_newline ? 1 : 0));
                        if (conn.send_trailing_newline) {
                            evt->data.back() = '\n';
                        }
                        std::copy(str.begin(), str.end(), evt->data.begin());
                        
                        for (auto &conn_weak: conn_man.connections) {
                            if (auto conn_shared = conn_weak.lock()) {
                                conn_shared->trigger(evt);
                            }
                        }
                        conn.async_read_next_message();
                    });
            });
        }
        
        template <typename Event, typename Command>
        void trigger_binary_common(Event &&evt, Command &c) {
            trigger_common(std::forward<Event>(evt), [&] {
                conn.send_ok();
                evt->data = std::vector<char>(
                    c.size + (conn.send_trailing_newline ? 1 : 0));
                evt->data.back() = '\n';    // if !send_trailing_newline,
                                            // then it is overwritten
                conn.do_async_read_binary(
                    &(evt->data.front()),
                    c.size,
                    [=, RIOT_KEEP_CONN_EX(conn)](bool res) {
                        if (!res) {
                            return ;
                        }
                        conn.reset_idle_counter();
                        conn.send_ok();
                        
                        // now the buffer is filled, supposedly
                        
                        for (auto &conn_weak: conn_man.connections) {
                            if (auto conn_shared = conn_weak.lock()) {
                                conn_shared->trigger(evt);
                            }
                        }
                        conn.async_read_next_message();
                    }
                );
            });
        }
        
        template <typename Event>
        void trigger_empty_common(Event &&evt) {
            trigger_common(
                std::forward<Event>(evt),
                [&] {
                    conn.send_ok();
                    
                    // TODO decide the following
                    // maybe I should post it somehow to be executed later?
                    // I did the same practice for implementing
                    // do_async_read_binary
                    //
                    // or, maybe it is just fine.
                    for (auto &conn_weak: conn_man.connections) {
                        if (auto conn_shared = conn_weak.lock()) {
                            conn_shared->trigger(evt);
                        }
                    }
                    conn.async_read_next_message();
                });
        }
        
        void operator()(parsers::command::cmd::trigger &c) {
            auto evt = std::make_shared<event>(
                &conn,
                event::trigger_line,
                c.evt,
                std::move(c.expr.value_or(parsers::sfe::expression())));
            
            trigger_line_common(evt);
        }
        
        void operator()(parsers::command::cmd::trigger_binary &c) {
            auto evt = std::make_shared<event>(
                &conn,
                event::trigger_binary,
                c.evt,
                std::move(c.expr.value_or(parsers::sfe::expression())));
            
            trigger_binary_common(evt, c);
        }
        
        void operator()(parsers::command::cmd::trigger_empty &c) {
            auto evt = std::make_shared<event>(
                &conn,
                event::trigger_empty,
                c.evt,
                std::move(c.expr.value_or(parsers::sfe::expression{})));
            
            trigger_empty_common(evt);
        }
        
        /**
         * please note that any error here should have invalid_argument{}
         * action
         */
        template <typename Event, typename Command>
        auto trigger_check_cache(Event &&e, Command &&c) {
            auto expr_it = conn.expression_cache.find(c.expr_id);
            if (expr_it != conn.expression_cache.end()) {
                // we have the expression, and it is cached
                e->expr = expr_it->second;
                return err_no_error;
            }
            else {
                auto localstorage_it = conn.local_storage.find(c.expr_id);
                if (localstorage_it != conn.local_storage.end()) {
                    // we have the expresion :)
                    try {
                        auto expr = parsers::sfe::parse(localstorage_it->second);
                        conn.expression_cache[c.expr_id] = expr;
                        e->expr = std::move(expr);
                        return err_no_error;
                    }
                    catch (const std::regex_error &) {
                        // but malformed
                        return err_cmd_cached_parser_regex;
                    }
                    catch (const std::runtime_error &) {
                        // but malformed
                        return err_cmd_cached_parser;
                    }
                }
                else {
                    // the given id is wrong
                    return err_cmd_invalid_arg;
                }
            }
        }
        
        template <typename Event, typename Command, typename F>
        void trigger_cached_common(Event &&e, Command &&c, F &&f) {
            auto error = trigger_check_cache(e, c);
            if (error == err_no_error) {
                f();
            }
            else {
                auto action = get_security_action(
                    security_actions::invalid_argument{});
                RIOT_HANDLE_ERROR_CASE(conn, action, error);
                conn.async_read_next_message();
            }
        }
        
        void operator()(parsers::command::cmd::trigger_cached &c) {
            auto evt = std::make_shared<event>(
                &conn,
                event::trigger_line,
                c.evt);
            
            trigger_cached_common(evt, c, [&] {
                trigger_line_common(evt);
            });
        }
        
        void operator()(parsers::command::cmd::trigger_cached_binary &c) {
            auto evt = std::make_shared<event>(
                &conn,
                event::trigger_binary,
                c.evt);
            
            trigger_cached_common(evt, c, [&] {
                trigger_binary_common(evt, c);
            });
        }
        
        void operator()(parsers::command::cmd::trigger_cached_empty &c) {
            auto evt = std::make_shared<event>(
                &conn,
                event::trigger_binary,
                c.evt);
            
            trigger_cached_common(evt, c, [&] {
                trigger_empty_common(evt);
            });
        }
        
        void operator()(
            const parsers::command::cmd::trigger_cached_cached_data &c) {
            auto evt = std::make_shared<event>(
                &conn,
                event::trigger_binary,
                c.evt);
            
            trigger_cached_common(evt, c, [&] {
                trigger_common(evt, [&] {
                    // check local storage
                    auto localstorage_it = conn.local_storage.find(c.data_id);
                    if (localstorage_it != conn.local_storage.end()) {
                        const auto &str = localstorage_it->second;
                        
                        // it exists, good
                        conn.send_ok();
                        
                        evt->data = std::vector<char>(
                            str.size() + (conn.send_trailing_newline ? 1 : 0));
                        evt->data.back() = '\n';
                        std::copy(str.begin(), str.end(), evt->data.begin());
                        
                        for (auto &conn_weak: conn_man.connections) {
                            if (auto conn_shared = conn_weak.lock()) {
                                conn_shared->trigger(evt);
                            }
                        }
                    }
                    else {
                        // oops...
                        auto action = get_security_action(
                            security_actions::invalid_argument{});
                        RIOT_HANDLE_ERROR_CASE(
                            conn, action, err_cmd_invalid_arg);
                    }
                    conn.async_read_next_message();
                });
            });
        }
        
        void operator()(parsers::command::cmd::pause &c) {
            conn.paused = true;
            conn.send_ok();
            conn.async_read_next_message();
        }
        
        void operator()(parsers::command::cmd::resume &c) {
            conn.paused = false;
            conn.send_ok();
            conn.async_read_next_message();
        }
        
        void operator()(parsers::command::cmd::alive &c) {
            conn.reset_idle_counter();
            conn.send_ok();
            conn.async_read_next_message();
            // I guess this one should not produce any output
        }
        
        void operator()(parsers::command::cmd::kill_me &c) {
            conn.do_close();
        }
        
        void operator()(parsers::command::cmd::echo &c) {
            if (c.state) {
                conn.echo = *(c.state);
            }
            else {
                conn.echo = !conn.echo; // toggle
            }
            conn.async_read_next_message();
        }
        
        void operator()(parsers::command::cmd::execute &c) {
            conn.send_error(err_cmd_not_impl);
            conn.async_read_next_message();
        }
        
        void operator()(parsers::command::cmd::execute_script &c) {
            boost::ignore_unused(c);
            conn.send_error(err_cmd_not_impl);
            conn.async_read_next_message();
        }
        
        void operator()(parsers::command::cmd::execute_cached &c) {
            boost::ignore_unused(c);
            conn.send_error(err_cmd_not_impl);
            conn.async_read_next_message();
        }
        
        auto get_empty_local_storage_id() {
            // am I sure if this thing really works?
            // in the worst case the used IDs are:
            // 0 1 2 ... (N-1) where N is the size. So choose N.
            // for every other case, I should have an empty room in [0, N)
            // interval, I guess. (I wish it is logical though)
            for (std::size_t i = 0; i < conn.local_storage.size(); ++i) {
                auto it = conn.local_storage.find(i);
                if (it == conn.local_storage.end())
                    return i;   // an empty ide
            }
            return conn.local_storage.size();
        }
        
        void operator()(parsers::command::cmd::store &c) {
            auto id = get_empty_local_storage_id();
            conn.local_storage[id] = std::move(c.line);
            conn.send_text("ok ", id);
        }
        
        void operator()(parsers::command::cmd::store_binary &c) {
            auto id = get_empty_local_storage_id();
            conn.local_storage[id] = std::vector<char>(c.size);
            auto buf = &(conn.local_storage[id].front());
            conn.send_text("ok ", id);
            
            conn.do_async_read_binary(
                buf,
                c.size,
                [&, RIOT_KEEP_CONN_EX(conn)](bool res) {
                    if (!res)
                        return;
                    conn.reset_idle_counter();
                    conn.async_read_next_message();
                });
        }
        
        void operator()(parsers::command::cmd::release &c) {
            auto it = conn.local_storage.find(c.id);
            if (it != conn.local_storage.end()) {
                // fine, we have it
                conn.local_storage.erase(it);
                // also remove if it exists in expression cache if exists
                auto it2 = conn.expression_cache.find(c.id);
                if (it2 != conn.expression_cache.end())
                    conn.expression_cache.erase(it2);
                conn.send_ok();
            }
            else {
                auto action = get_security_action(
                    security_actions::invalid_argument{});
                RIOT_HANDLE_ERROR_CASE(conn, action, err_cmd_invalid_arg);
            }
            conn.async_read_next_message();
        }
    
    };
    
    friend struct command_handler;
    
    command_handler command_handler_ {*this};
    
    struct write_item {
        const char *data;
        std::size_t size;
        simple_callback_t callback;
        write_mode_t write_mode;
        
        write_item(
            const char *data_,
            std::size_t size_,
            simple_callback_t callback_,
            write_mode_t write_mode_):
            data{data_},
            size{size_},
            callback{std::move(callback_)},
            write_mode{write_mode_} {
        }
    };
    std::list<write_item> write_queue_;
    
    void async_write_next() {
        if (write_queue_.size() == 0)
            return ;
        auto &first = write_queue_.front();
        do_async_write(first.data, first.size,
            [this, RIOT_KEEP_CONN](bool res) {
                auto &first = write_queue_.front();
                if (first.callback) {
                    first.callback(res);
                }
                write_queue_.pop_front();
                if (res) {
                    async_write_next();
                }
            }, first.write_mode);
    }
    
    void async_read_next_message() {
        do_async_read_message(
            [this, RIOT_KEEP_CONN] (const std::string &str, bool res) {
                handle_next_message(str, res);
            });
    }
    
    enum state_t {
        st_protocol,
        st_props,
        st_active
    };
    
    state_t state{st_protocol};
    std::size_t description_total_size{0};
    
    void handle_next_message(
        const std::string &msg, bool res) {
        using namespace security_actions;
        
        if (!res) {
            return ;
        }
        
        reset_idle_counter();
        
        if (state != st_active) {
            auto description_max_size =
                get_artifact(artifacts::header_max_size {});
            if (description_max_size != 0) {
                if (description_total_size >= description_max_size) {
                    auto action = get_security_action(
                        security_actions::header_size_limit_reached{});
                    RIOT_HANDLE_ERROR_CASE(*this, action, err_header_unspecified);
                }
                description_total_size += msg.size() + 1 /* '\n' */;
            }
        }
        
        switch (state) {
        case st_protocol: {
            auto trimmed_msg = boost::trim_copy(msg);
            if (trimmed_msg == PROTOCOL_NAME) {
                // fine
                state = st_props;
                send_ok();
            }
            else if (trimmed_msg == std::string(PROTOCOL_NAME) + "_echo_off") {
                // fine still
                state = st_props;
                echo = false;
                send_ok();  // no, it doesn't do anything really though
            }
            else {
                // ErrorCase: Wrong Protocol
                auto action =
                    get_security_action( header_wrong_protocol {});
                
                RIOT_BEGIN_ERROR_CASE(*this, action, err_protocol);
                send_protocol();    // in any case, send protocol
                                    // please note that, if send_ok() is called
                                    // then the client cannot know this
                                    // coming!
                RIOT_END_ERROR_CASE(*this, action, err_protocol);
            }
            break;
        }
        case st_props: {
            if (boost::trim_copy(msg) == "END") {
                auto name_it = properties.find("name");
                if (name_it != properties.end())
                    name = name_it->second.front();
                else {
                    // ErrorCase: No Name
                    auto action =
                        get_security_action( header_no_name {});
                    
                    RIOT_HANDLE_ERROR_CASE(*this, action, err_malformed_header);
                    
                    if (action & action_not_allowed ) {
                        // if not halted, what to do?
                        // simply, read next command and return
                        async_read_next_message();
                        return ;
                    }
                }
                auto password_it = properties.find("password");
                password = password_it != properties.end() ?
                    password_it->second.front() : "";
                groups = properties["groups"];
                    // move instead? maybe...
                if (!can_activate()) {
                    send_error(err_activate_security_fail);
                    return ;
                }
                
                state = st_active;
                    // now, append itself to the list of connections
                conn_man.connections.push_back(this->shared_from_this());
                // TODO find out why it doesn't compile if this-> is omitted
                do_set_async_read_message_max_size(0);
                send_ok();
            }
            else if (boost::trim_copy(msg).empty()) {
                // empty command is not an error!
            }
            else {
                try {
                    auto r = parsers::header::parse(msg);
                    auto p = r.front();
                    r.pop_front();
                    properties[p] = r;
                    send_ok();
                }
                catch (std::exception const &ex) {
                    // ErrorCase: Malformed Header
                    auto action =
                        get_security_action( header_malformed_header {});
                    
                    RIOT_HANDLE_ERROR_CASE(*this, action, err_malformed_header);
                    
                    // read the next header
                }
            }
            break;
        }
        case st_active: {
            // command handling is to be done here
            if (boost::trim_copy(msg).empty()) {
                // empty message is not an error
            }
            else {
                protocol_error_code ec = err_no_error;
                try {
                    auto cmd = parsers::command::parse(msg);
                    boost::apply_visitor(command_handler_, cmd);
                    return ;
                }
                catch (std::regex_error &err) {
                    // we have a regex error
                    ec = err_parser_regex;
                }
                catch (std::exception &ex) {
                    // the command is malformed
                    ec = err_parser;
                }
                
                if (ec != err_no_error) {
                    // case Malformed Regex or Command
                    security_actions::action action;
                    
                    if (ec == err_parser_regex) {
                        action = get_security_action(malformed_regex {});
                    }
                    else {
                        action = get_security_action(malformed_command {});
                    }
                    
                    RIOT_HANDLE_ERROR_CASE(*this, action, ec);
                    
                    // read next command if ignored
                }
            }
        }
        }
        async_read_next_message();
    }
#undef RIOT_BEGIN_ERROR_CASE
#undef RIOT_END_ERROR_CASE
#undef RIOT_HANDLE_ERROR_CASE
    
    template <typename Action>
    auto get_security_action(Action &&action) {
        return conn_man.security_policy(*this, std::forward<Action>(action));
    }
    
    template <typename Artifact>
    auto get_artifact(Artifact &&artifact) {
        return conn_man.artifact_provider(
            *this, std::forward<Artifact>(artifact));
    }
    
protected:
    /**
     * @brief Sends text to the client without caring the echo condition.
     * Use this function for information that the client really need to receive.
     * Provided to simplify the writing process.
     * 
     * @param s A list of strings or io-manipulators.
     */
    template <typename ...S>
    void send_text(S && ...s) {
        std::ostringstream oss;
        ((oss << std::forward<S>(s)), ...);
        if (send_trailing_newline)
            oss << "\n";
        enqueue_async_write(oss.str());
    }
    
    /**
     * @brief Sends a error code in decimal form to te client without caring
     * the echo condition.
     * 
     * @param ec The error code to be sent.
     * @param token A token sent as precursor, something like "err " or "info ".
     */
    void send_error_code(
        protocol_error_code ec,
        const std::string &token) {
        using int_t = std::underlying_type_t<protocol_error_code>;
        constexpr auto width =
            std::numeric_limits<int_t>::digits10 + 1;
        send_text(token, " ", std::setw(width), std::setfill('0'), (int_t) ec);
    }
    
    /**
     * @brief send_error_code with token "err ". Cares the echo state.
     */
    void send_error(protocol_error_code ec) {
        if (echo) send_error_code(ec, "err ");
    }
    
    /**
     * @brief Send_error_code with token "warn". Cares the echo state.
     */
    void send_warning(protocol_error_code ec) {
        if (echo) send_error_code(ec, "warn");
    }
    
    /**
     * @brief Sends text "ok". Cares the echo state.
     */
    void send_ok() {
        if (echo) send_text("ok");
    }
    
    /**
     * @brief Sends text prefixed with str. Cares the echo state.
     */
    void send_info(std::string const& str) {
        if (echo) send_text("info ", str);
    }
    
    /**
     * @brief Sends the protocol.
     */
    void send_protocol() {
        send_info(PROTOCOL_NAME);
    }
    
    /**
     * @brief Freezes the connection for some time. When a connection is
     * freezed, it doesn't process commands until it is thawn.
     * 
     * @param duration Freeze duration, in the time unit of the application.
     */
    void freeze_for(duration_t duration) {
        // FIXME implement me!
    }
    
    /**
     * @brief 
     */
    void freeze(protocol_error_code ec = err_no_error) {
        freeze_for(get_artifact(artifacts::freeze_duration{ec}));
    }
    
    bool can_activate() {
        return get_artifact(artifacts::can_activate{});
    }
    
    void reset_idle_counter() {
        // FIXME implement me!
    }
    
    void close() {
        do_close();
    }
protected:
    /**
     * @brief The derived classes must implement this function to provide
     * asynchronous write ability. The data is guaranteed to be valid during
     * the operation. Therefore, it doesn't required to be stored anywhere.
     * In case of byte-oriented protocols, this function simple writes bytes
     * and write_mode is usually not considered. In case of message-oriented
     * protocols, this function should deliver a single message.
     * 
     * The refcount of *this should not be increased by this function.
     * 
     * @param data A non-owning pointer to the data buffer. Lifetime is
     * guaranteed to span the whole write operation.
     * @param size Size of the data buffer.
     * @param callback Callback to be called when the write operation is done.
     * The bool argument is true if successful.
     * @param write_mode Describes whether binary data or text data is written.
     * It is intended only for message oriented protocols.
     */
    virtual void do_async_write(
        const char *data,
        std::size_t size,
        std::function<void (bool)> callback,
        write_mode_t write_mode) = 0;
    
    /**
     * @brief The derived classes must implement this function to provide
     * asynchronous read ability. This function either reads a line of string
     * by using some kind of read_line or read_until('\n') call in case of
     * byte-oriented protocols, or reads a message in case of message-oriented
     * protocols. Please note that in case byte-oriented protocols, there
     * should be no trailing new line character!
     * 
     * The refcount of *this should be increased by this function.
     * 
     * @param callback Callback to be called when the read operation is
     * complete. The read message should be received using std::string 
     * parameter. The bool argument is for success.
     */
    virtual void do_async_read_message(
        std::function<void (const std::string &, bool)> callback) = 0;
    
    /**
     * @brief This function affects the behaviour of do_async_read_message.
     * 
     * @param sz New maximum size. If sz is set to 0, it means no limit.
     */
    virtual void do_set_async_read_message_max_size(std::size_t sz) = 0;
    
    /**
     * @brief This function is similar to do_async_read_message(...). However,
     * the read data is binary. The read buffer is provided by the caller,
     * and it is guaranteed to be valid until the end of the read operation.
     * For message-oriented protocols, the read message may be checked against
     * type being binary. However, this is not a necessity.
     * 
     * The refcount of *this should not be increased by this function.
     * 
     * @param buffer Read buffer, owned by the caller.
     * @param size Size of the read bufer and the requested number of bytes.
     * @param callback Callback to be called 
     */
    virtual void do_async_read_binary(
        char *buffer,
        std::size_t size,
        std::function<void (bool)> callback) = 0;
    
    /**
     * @brief Should cancel all of the asynchronous operations on the underlying
     * layers, effectively failing all of the callbacks, and causing connection
     * to die. Other possible callbacks should also fail since it is closed.
     */
    virtual void do_close() = 0;
    
    /**
     * @brief This function should block this client. It may need to
     * communicate with its spawning server.
     */
    virtual void do_block_endpoint() = 0;
};

}

#undef RIOT_KEEP_CONN
#undef RIOT_KEEP_CONN_EX
#undef RIOT_INC_REF
#undef RIOT_KEEP_CONN

#endif // RIOT_SERVER_CONNECTION_BASE_INCLUDED
