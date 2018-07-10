/**
 * @author Canberk Sönmez
 * 
 * @date Sat Jul  7 13:15:16 +03 2018
 * 
 */

#ifndef RIOT_SERVER_LUA_INTERFACE_INCLUDED
#define RIOT_SERVER_LUA_INTERFACE_INCLUDED

struct lua_State;

namespace riot::server {

namespace lua {

void prepare_for_initscript(lua_State *lua);

}

};

#endif // RIOT_SERVER_LUA_INTERFACE_INCLUDED
