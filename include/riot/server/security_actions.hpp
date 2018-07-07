/**
 * @author Canberk Sönmez
 * 
 * @date Thu Jul  5 23:29:13 +03 2018
 * 
 */

#ifndef RIOT_SERVER_SECURITY_ACTIONS_INCLUDED
#define RIOT_SERVER_SECURITY_ACTIONS_INCLUDED

#include <string>

namespace riot::server {

namespace security_actions {
    enum action: char {
        
        /// basically means no problem
        action_allowed = 0,
        
        /// not allowed, send ok but do not do anything
        action_not_allowed = 1,
        
        /* BEGIN do not use them directly !!! */
        _action_raise_warning   = 0b000010,
        _action_raise_error     = 0b000100,
        
        _action_halt            = 0b001000,
        _action_block           = 0b010000,
        _action_freeze          = 0b100000,
        /* END */
        
        action_raise_warning_and_ignore =
            _action_raise_warning | action_not_allowed,
        
        action_raise_warning_and_freeze =
            _action_raise_warning | _action_freeze | action_not_allowed,
        
        action_raise_error_and_halt =
            _action_raise_error | _action_halt | action_not_allowed,
        
        action_raise_error_and_halt_then_block =
            action_raise_error_and_halt | _action_block | action_not_allowed,
    };
    
    struct header_wrong_protocol {  };
    struct header_no_name {  };
    struct header_malformed_header {  };
    struct header_size_limit_reached {  };
    struct malformed_command {  };
    struct invalid_argument {  };
    struct malformed_regex {  };
    struct too_frequent_trigger {  };
    struct unpermitted_code_execution {  };
    struct malformed_code {  };
    struct trigger_prohibited {
        std::string evt;
    };
};

}

#endif // RIOT_SERVER_SECURITY_ACTIONS_INCLUDED
