#pragma once

extern "C" {

#include <lua.h>
#include <lauxlib.h>
#include <luacbor_export.h>

LUACBOR_EXPORT int luaopen_B2T(lua_State *L);
}
