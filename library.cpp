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
            return sol::object(lua, sol::in_place, sol::nil); // todo: return B2T.null would be better
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
                table[key + 1] = jsonParseObj(value, lua);
            else if (value.is_array())
                table[key + 1] = jsonParseArray(value, lua);
            else
                table[key + 1] = jsonParseValue(value, lua);
        }
        return table;
    }

    enum class BinType : int {
        cbor = 0,
        msgpack = 1,
        ubjson = 2
    };

    enum class Type : int {
        json = 0,
        table = 1
    };


    nlohmann::json luaTableToJson(sol::table table, sol::state_view &lua);

    nlohmann::json luaToJson(sol::object data, sol::state_view &lua)
    {
        nlohmann::json json;
        sol::type type = data.get_type();

        if (type == sol::type::string)
            json = data.as<const char *>();
        else if (type == sol::type::number)
        {
            double val = data.as<double>();
            double ip;
            if (modf(val, &ip) == 0.0)
                json = (long)(val);
            else
                json = val;
        }
        else if (type == sol::type::boolean)
            json = data.as<bool>();
        else if (type == sol::type::table)
            json = luaTableToJson(data, lua);
        return json;
    }

    nlohmann::json luaTableToJson(sol::table table, sol::state_view &lua)
    {
        bool allNums = true;

        for (const auto &[key, value] : table)
        {
            if (key.get_type() != sol::type::number)
            {
                allNums = false;
                break;
            }
        }

        if (allNums)
        {
            nlohmann::json::array_t array(table.size());
            for (size_t i = 0; i < table.size(); ++i)
                array[i] = luaToJson(table[i + 1], lua);
            return array;
        }
        else
        {
            nlohmann::json::object_t object;
            for (const auto &[key, value] : table)
                object[key.as<std::string>()] = luaToJson(value, lua);
            return object;
        }
    }

    sol::table open_B2T(sol::this_state L)
    {
        sol::state_view lua(L);
        sol::table module = lua.create_table();

        module["json"] = Type::json;
        module["table"] = Type::table;

        module["cbor"] = BinType::cbor;
        module["msgpack"] = BinType::msgpack;
        module["ubjson"] = BinType::ubjson;

        module["to_bin"] = [](sol::object data, Type inType, BinType binType, sol::this_state L) ->sol::object {
            sol::state_view lua(L);

            nlohmann::json json;

            if (inType == Type::table)
                json = luaToJson(data, lua);
            else
                json = json.parse(data.as<std::string>());

            std::vector<uint8_t> binData;

            switch (binType)
            {
                case BinType::cbor:
                    binData = nlohmann::json::to_cbor(json);
                    break;
                case BinType::msgpack:
                    binData = nlohmann::json::to_msgpack(json);
                    break;
                case BinType::ubjson:
                    binData = nlohmann::json::to_ubjson(json);
                    break;
                default:
                    return sol::object(lua, sol::in_place, sol::nil);
            }

            return sol::object(lua, sol::in_place, std::string((char*) binData.data(), binData.size()));
        };

        module["from_bin"] = [](std::string binData, Type outType, BinType binType, sol::object jsonIndent, sol::this_state L) {
            sol::state_view lua(L);

            nlohmann::json json;
            switch (binType)
            {
                case BinType::cbor:
                    json = nlohmann::json::from_cbor(binData);
                    break;
                case BinType::msgpack:
                    json = nlohmann::json::from_msgpack(binData);
                    break;
                case BinType::ubjson:
                    json = nlohmann::json::from_ubjson(binData);
                    break;
                default:
                    return sol::object(lua, sol::in_place, sol::nil);
            }

            if(outType == Type::json)
            {
                int indent = -1;
                if(jsonIndent.is<int>())
                    indent = jsonIndent.as<int>();
                return sol::object(lua, sol::in_place, json.dump(indent));
            }
            else
                return sol::object(lua, sol::in_place,jsonParseValue(json, lua));
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
local cborData = b2t.to_bin(jsonData, b2t.json, b2t.cbor)
local luaData = b2t.from_bin(cborData, b2t.table, b2t.cbor)

print(b2t.from_bin(cborData, b2t.json, b2t.cbor, 4))

print("-------------------------")
for k,v in pairs(luaData) do
    print(k .. ": ".. tostring(v))
end

print("-------------------------")

local luaData2 = {
    string = "MyString",
    number = 42,
    bool = false,
    array = { 9, 8, 7, 6, 5.432, 1, 0 }
}

local msgData = b2t.to_bin(luaData2, b2t.table, b2t.msgpack)
print(b2t.from_bin(msgData, b2t.json, b2t.msgpack))
)");
}
#endif
