/**
 * @author Canberk Sönmez
 * 
 * @date Thu Jul  5 23:29:13 +03 2018
 * 
 */

#ifndef RIOT_SERVER_SERVER_ARTIFACTS_INCLUDED
#define RIOT_SERVER_SERVER_ARTIFACTS_INCLUDED

#include <string>
#include <riot/defs.hpp>
#include <riot/constants.hpp>

namespace riot::server {

namespace artifacts {
    template <typename ResultType>
    struct query_base {
        using result_type = ResultType;
    };
    
    template <typename ConnectionBase>
    struct header_message_max_size: query_base<std::size_t> {  };
    
    template <typename ConnectionBase>
    struct header_max_size: query_base<std::size_t> {  };
    
    template <typename ConnectionBase>
    struct can_activate: query_base<bool> {  };

    template <typename ConnectionBase>
    struct minimum_time_between_triggers: query_base<duration_t> {  };

    template <typename ConnectionBase>
    struct can_execute_code: query_base<bool> {  };

    template <typename ConnectionBase>
    struct freeze_duration: query_base<duration_t> {
        protocol_error_code ec {err_no_error};
        
        freeze_duration() {}
        explicit freeze_duration(protocol_error_code ec_): ec{ec_} {}
    };
    
    template <typename ConnectionBase>
    struct can_receive_event: query_base<bool> {
        typename ConnectionBase::event const *event;
        
        can_receive_event() {}
        explicit can_receive_event(
            typename ConnectionBase::event const *event_):
            event{event_} {}
    };
    
    template <typename ConnectionBase>
    struct can_trigger_event: query_base<bool> {
        std::string evt;
        
        can_trigger_event() {}
        explicit can_trigger_event(std::string evt_): evt{std::move(evt_)} {}
    };
    
    template <typename ConnectionBase>
    struct keep_alive_period: query_base<duration_t> {  };
};

}

#endif // RIOT_SERVER_SERVER_ARTIFACTS_INCLUDED
