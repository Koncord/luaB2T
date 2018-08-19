#include "library.h"

#include <nlohmann/json.hpp>

#define SOL_CHECK_ARGUMENTS 1
#include <sol/sol.hpp>
#include <fstream>

namespace B2T
{
    sol::object jsonParseValue(const nlohmann::json::value_type &value, sol::state_view &lua);
    sol::object jsonParseObj(const nlohmann::json::object_t &obj, sol::state_view &lua);
    sol::object jsonParseArray(const nlohmann::json::array_t &obj, sol::state_view &lua);

    sol::object jsonParseValue(const nlohmann::json::value_type &value, sol::state_view &lua)
    {
        if (value.is_object())
            return jsonParseObj(value, lua);
        else if (value.is_array())
            return jsonParseArray(value, lua);
        if (value.is_null())
            return sol::make_object(lua, sol::nil); // todo: return B2T.null would be better
        else if (value.is_number_float())
            return sol::object(lua, sol::in_place, value.get<double>());
        else if (value.is_number_integer())
            return sol::object(lua, sol::in_place, value.get<long>());
        else if (value.is_string())
            return sol::object(lua, sol::in_place, value.get<std::string>());
        else if (value.is_boolean())
            return sol::object(lua, sol::in_place, value.get<bool>());
    }

    sol::object jsonParseObj(const nlohmann::json::object_t &obj, sol::state_view &lua)
    {
        sol::table table = lua.create_table(0, obj.size());
        for (const auto &[key, value] : obj)
        {
            if (value.is_object())
                table[key] = jsonParseObj(value, lua);
            else if (value.is_array())
                table[key] = jsonParseArray(value, lua);
            else
                table[key] = jsonParseValue(value, lua);
        }
        return table;
    }

    sol::object jsonParseArray(const nlohmann::json::array_t &array, sol::state_view &lua)
    {
        sol::table table = lua.create_table(array.size());
        for (int key = 0; key < array.size(); ++key)
        {
            auto &value = array[key];
            if (value.is_object())
                table[key] = jsonParseObj(value, lua);
            else if (value.is_array())
                table[key] = jsonParseArray(value, lua);
            else
                table[key] = jsonParseValue(value, lua);
        }
        return table;
    }

    sol::table open_B2T(sol::this_state L)
    {
        sol::state_view lua(L);
        sol::table module = lua.create_table();

        module["to_cbor"] = [](const char *jsonData) {
            auto json = nlohmann::json::parse(jsonData);
            std::vector<uint8_t> adata = nlohmann::json::to_cbor(json);
            return std::string((char*) adata.data(), adata.size());
        };

        module["from_cbor"] = [](sol::object data, sol::this_state L) {
            sol::state_view lua(L);
            nlohmann::json json = nlohmann::json::from_cbor(data.as<std::string>());

            return jsonParseValue(json, lua);
        };

        module["to_msgpack"] = [](const char *jsonData) {
            auto json = nlohmann::json::parse(jsonData);
            std::vector<uint8_t> adata = nlohmann::json::to_msgpack(json);
            return std::string((char*) adata.data(), adata.size());
        };

        module["from_msgpack"] = [](sol::object data, sol::this_state L) {
            sol::state_view lua(L);
            nlohmann::json json = nlohmann::json::from_msgpack(data.as<std::string>());

            return jsonParseValue(json, lua);
        };

        module["to_ubjson"] = [](const char *jsonData) {
            auto json = nlohmann::json::parse(jsonData);
            std::vector<uint8_t> adata = nlohmann::json::to_ubjson(json);
            return std::string((char*) adata.data(), adata.size());
        };

        module["from_ubjson"] = [](sol::object data, sol::this_state L) {
            sol::state_view lua(L);
            nlohmann::json json = nlohmann::json::from_ubjson(data.as<std::string>());

            return jsonParseValue(json, lua);
        };
        return module;
    }
}
#ifndef _DEBUG
extern "C" int luaopen_B2T(lua_State *L)
{
    return sol::stack::call_lua(L, 1, B2T::open_B2T);
}
#else
int main()
{
    sol::state lua;
    lua.open_libraries();
    lua["b2t"] = B2T::open_B2T(lua.lua_state());
    lua.script(R"(
local jsonData = [[{
    "string":"testString",
    "number":64,
    "array": [0, 1, 2, 3],
    "bool": true
}]]
local cborData = b2t.to_cbor(jsonData)
local luaData = b2t.from_cbor(cborData)

for k,v in pairs(luaData) do
    print(k .. ": ".. tostring(v))
end

print("jsonSize: " .. #jsonData .. " cborSize: " .. #cborData)
)");
}
#endif
