/*
 * LuaStack.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LuaStack.h"

#include "../lib/json/JsonNode.h"
#include "../lib/int3.h"
#include "../lib/battle/BattleHex.h"
#include "../lib/constants/EntityIdentifiers.h"
#include "../lib/GameLibrary.h"

#include <vcmi/Creature.h>

namespace scripting
{

LuaStack::LuaStack(lua_State * L_):
	L(L_),
	initialTop(lua_gettop(L))
{
}

void LuaStack::restoreInitialTop()
{
	lua_settop(L, initialTop);
}

void LuaStack::clear()
{
	lua_settop(L, 0);
}

void LuaStack::debugPrintStack()
{
	int top = lua_gettop(L);
	logScript->error("Lua stack (top=%d):\n", top);
	for (int i = 1; i <= top; ++i) {
		int t = lua_type(L, i);
		const char *tn = lua_typename(L, t);
		switch (t) {
			case LUA_TSTRING:
				logScript->error("  %d: %s = '%s'", i, tn, lua_tostring(L, i));
				break;
			case LUA_TNUMBER:
				logScript->error("  %d: %s = %g", i, tn, lua_tonumber(L, i));
				break;
			case LUA_TBOOLEAN:
				logScript->error("  %d: %s = %s", i, tn, lua_toboolean(L, i) ? "true" : "false");
				break;
			case LUA_TNIL:
				logScript->error("  %d: %s = (nil)", i, tn);
				break;
			default:
				logScript->error("  %d: %s (%p)", i, tn, lua_topointer(L, i));
				break;
		}
	}
}

int LuaStack::absindex(int idx)
{
	if (idx > 0)
		return idx;

	return lua_gettop(L) + 1 + idx;
}

void LuaStack::pushNil()
{
	lua_pushnil(L);
}

void LuaStack::pushInteger(lua_Integer value)
{
	lua_pushinteger(L, value);
}

void LuaStack::push(bool value)
{
	lua_pushboolean(L, value);
}

void LuaStack::push(double value)
{
	lua_pushnumber(L, value);
}

void LuaStack::push(const char * value)
{
	lua_pushstring(L, value);
}

void LuaStack::push(const std::string & value)
{
	lua_pushlstring(L, value.c_str(), value.size());
}

void LuaStack::push(const int3 & value)
{
	push(value.x);
	push(value.y);
	push(value.z);
}

void LuaStack::push(const JsonNode & value)
{
	switch(value.getType())
	{
	case JsonNode::JsonType::DATA_BOOL:
		{
			push(value.Bool());
		}
		break;
	case JsonNode::JsonType::DATA_FLOAT:
		{
			lua_pushnumber(L, value.Float());
		}
		break;
	case JsonNode::JsonType::DATA_INTEGER:
		{
			pushInteger(value.Integer());
		}
		break;
	case JsonNode::JsonType::DATA_STRUCT:
		{
			lua_newtable(L);
			for(const auto & keyValue : value.Struct())
			{
				push(keyValue.first);
				push(keyValue.second);
				lua_rawset(L, -3);
			}
			setModScopeMetatable(value);
		}
		break;
	case JsonNode::JsonType::DATA_STRING:
		push(value.String());
		break;
	case JsonNode::JsonType::DATA_VECTOR:
		{
			lua_newtable(L);
			for(int idx = 0; idx < value.Vector().size(); idx++)
			{
				pushInteger(idx + 1);
				push(value.Vector()[idx]);
				lua_rawset(L, -3);
			}
			setModScopeMetatable(value);
		}
		break;

	default:
		pushNil();
		break;
	}
}

void LuaStack::get(int position, bool & value)
{
	if(!lua_isboolean(L, position))
	{
		const char * actualType = lua_typename(L, lua_type(L, position));
		throw LuaApiException(std::string("Invalid Lua value! Expected boolean at position ") + std::to_string(position) + ", but found " + actualType);
	}
	value = (lua_toboolean(L, position) != 0);
}

void LuaStack::get(int position, double & value)
{
	if(!lua_isnumber(L, position))
	{
		const char * actualType = lua_typename(L, lua_type(L, position));
		throw LuaApiException(std::string("Invalid Lua value! Expected number at position ") + std::to_string(position) + ", but found " + actualType);
	}
	value = lua_tonumber(L, position);
}

void LuaStack::getInteger(int position, lua_Integer & value)
{
	if(!lua_isnumber(L, position))
	{
		const char * actualType = lua_typename(L, lua_type(L, position));
		throw LuaApiException(std::string("Invalid Lua value! Expected integer at position ") + std::to_string(position) + ", but found " + actualType);
	}
	value = lua_tointeger(L, position);
}

void LuaStack::get(int position, std::string & value)
{
	if(!lua_isstring(L, position))
	{
		const char * actualType = lua_typename(L, lua_type(L, position));
		throw LuaApiException(std::string("Invalid Lua value! Expected string at position ") + std::to_string(position) + ", but found " + actualType);
	}

	size_t len = 0;
	const auto *raw = lua_tolstring(L, position, &len);
	value = std::string(raw, len);
}

void LuaStack::get(int position, ScriptModScope & value)
{
	if(lua_isnoneornil(L, position))
		value.value = callerModScope();
	else
		get(position, value.value);
}

void LuaStack::get(int position, int3 & value)
{
	get(position,   value.x);
	get(position+1, value.y);
	get(position+2, value.z);
}

void LuaStack::get(int position, BattleHex & value)
{
	if(lua_isnumber(L, position))
	{
		lua_Integer temp;
		getInteger(position, temp);
		value = BattleHex(static_cast<si16>(temp));
	}
	else
	{
		getUData<BattleHex>(position, value);
	}
}

void LuaStack::push(const CreatureID & value)
{
	push(value.toEntity(LIBRARY));
}

void LuaStack::get(int position, CreatureID & value)
{
	const Creature * creature = nullptr;
	get(position, creature);
	value = creature ? creature->getId() : CreatureID(CreatureID::NONE);
}

void LuaStack::get(int position, JsonNode & value)
{
	getJson(absindex(position), value, callerModScope());
}

void LuaStack::getJson(int position, JsonNode & value, const std::string & inheritedScope)
{
	auto type = lua_type(L, position);

	switch(type)
	{
	case LUA_TNIL:
		value.clear();
		return;
	case LUA_TNUMBER:
		get(position, value.Float());
		break;
	case LUA_TBOOLEAN:
		value.Bool() = (lua_toboolean(L, position) != 0);
		break;
	case LUA_TSTRING:
		get(position, value.String());
		break;
	case LUA_TTABLE:
		{
			const std::string scope = tableModScope(position, inheritedScope);
			JsonNode asVector;
			JsonNode asStruct;

			lua_pushnil(L);  /* first key */

			while(lua_next(L, position) != 0)
			{
				/* 'key' (at index -2) and 'value' (at index -1) */

				JsonNode fieldValue;
				try
				{
					getJson(lua_gettop(L), fieldValue, fieldModScope(position, absindex(-2), scope));
				}
				catch(...)
				{
					lua_pop(L, 2);
					value.clear();
					throw;
				}

				lua_pop(L, 1); //pop value

				if(lua_type(L, -1) == LUA_TNUMBER)
				{
					auto key = lua_tointeger(L, -1);

					if(key > 0)
					{
						if(asVector.Vector().size() < key)
							asVector.Vector().resize(key);
						--key;
						asVector.Vector().at(key) = fieldValue;
					}
				}
				else if(lua_isstring(L, -1))
				{
					std::string key;
					get(-1, key);
					asStruct[key] = fieldValue;
				}
			}

			if(!asVector.Vector().empty())
			{
				std::swap(value, asVector);
			}
			else
			{
				std::swap(value, asStruct);
			}
			value.setModScope(scope, false);
		}
		return;
	default:
		{
			const char * actualType = lua_typename(L, type);
			throw LuaApiException(std::string("Unsupported Lua type for JsonNode: ") + actualType);
		}
	}
	value.setModScope(inheritedScope, false);
}

static const char * const MOD_SCOPE_FIELD = "__vcmiScope";
static const char * const LEAF_SCOPES_FIELD = "__vcmiLeafScopes";

void LuaStack::setModScopeMetatable(const JsonNode & json)
{
	const std::string & scope = json.getModScope();
	const auto isForeignString = [&scope](const JsonNode & child)
	{
		// json merge replaces strings as a whole, so a string in a table of one mod may come from another mod
		return child.isString() && !child.getModScope().empty() && child.getModScope() != scope;
	};

	bool hasForeignStrings = json.isStruct()
		? std::ranges::any_of(json.Struct(), [&](const auto & entry){ return isForeignString(entry.second); })
		: std::ranges::any_of(json.Vector(), isForeignString);

	if(!hasForeignStrings)
	{
		if(scope.empty())
			return;

		// most tables have no foreign strings, so they can share one metatable per mod
		if(luaL_newmetatable(L, ("vcmi.modScope:" + scope).c_str()) != 0)
		{
			push(scope);
			lua_setfield(L, -2, MOD_SCOPE_FIELD);
		}
		lua_setmetatable(L, -2);
		return;
	}

	lua_newtable(L);
	if(!scope.empty())
	{
		push(scope);
		lua_setfield(L, -2, MOD_SCOPE_FIELD);
	}

	lua_newtable(L);
	if(json.isStruct())
	{
		for(const auto & [key, child] : json.Struct())
		{
			if(!isForeignString(child))
				continue;
			push(key);
			push(child.getModScope());
			lua_rawset(L, -3);
		}
	}
	else
	{
		for(size_t i = 0; i < json.Vector().size(); ++i)
		{
			if(!isForeignString(json.Vector()[i]))
				continue;
			pushInteger(i + 1);
			push(json.Vector()[i].getModScope());
			lua_rawset(L, -3);
		}
	}
	lua_setfield(L, -2, LEAF_SCOPES_FIELD);
	lua_setmetatable(L, -2);
}

std::string LuaStack::tableModScope(int tablePosition, const std::string & fallback)
{
	tablePosition = absindex(tablePosition);
	std::string result = fallback;

	if(lua_getmetatable(L, tablePosition) == 0)
		return result;

	lua_pushstring(L, MOD_SCOPE_FIELD);
	lua_rawget(L, -2);
	if(lua_type(L, -1) == LUA_TSTRING)
		get(-1, result);
	lua_pop(L, 2);
	return result;
}

std::string LuaStack::fieldModScope(int tablePosition, int keyPosition, const std::string & tableScope)
{
	tablePosition = absindex(tablePosition);
	keyPosition = absindex(keyPosition);
	std::string result = tableScope;

	if(lua_getmetatable(L, tablePosition) == 0)
		return result;

	lua_pushstring(L, LEAF_SCOPES_FIELD);
	lua_rawget(L, -2);
	if(lua_istable(L, -1))
	{
		lua_pushvalue(L, keyPosition);
		lua_rawget(L, -2);
		if(lua_type(L, -1) == LUA_TSTRING)
			get(-1, result);
		lua_pop(L, 1);
	}
	lua_pop(L, 2);
	return result;
}

void LuaStack::copyModScope(int tablePosition, int metatablePosition)
{
	tablePosition = absindex(tablePosition);
	metatablePosition = absindex(metatablePosition);

	if(lua_getmetatable(L, tablePosition) == 0)
		return;

	for(const char * field : {MOD_SCOPE_FIELD, LEAF_SCOPES_FIELD})
	{
		lua_getfield(L, -1, field);
		lua_setfield(L, metatablePosition, field);
	}
	lua_pop(L, 1);
}

std::string LuaStack::callerModScope()
{
	lua_Debug info;
	for(int level = 0; lua_getstack(L, level, &info); ++level)
	{
		lua_getinfo(L, "S", &info);
		if(std::string_view(info.what) == "C")
			continue;

		std::string_view source(info.source);
		auto separator = source.find(':');
		if(separator != std::string_view::npos)
			return std::string(source.substr(0, separator));
	}
	throw LuaApiException("Unable to determine mod of the calling script!");
}

int LuaStack::retVoid()
{
	clear();
	return 0;
}


}
