#pragma once

extern "C" {

#include <lua.h>
#include <lauxlib.h>

#ifndef _DEBUG
#include <b2t_export.h>

B2T_EXPORT int luaopen_B2T(lua_State *L);
#endif
}
