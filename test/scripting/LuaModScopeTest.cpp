/*
 * LuaModScopeTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#if __has_include(<lua.hpp>)
#  include <lua.hpp>
#else
#  include <lua.h>
#  include <lauxlib.h>
#  include <lualib.h>
#endif

#include "../../luascript/LuaStack.h"
#include "../../lib/json/JsonNode.h"

namespace test
{
using scripting::LuaStack;
using scripting::ScriptModScope;

/// Identifiers in json that goes through Lua must be resolved from the mod that wrote them, otherwise
/// a name defined by several mods becomes ambiguous
class LuaModScopeTest : public ::testing::Test
{
public:
	static inline JsonNode captured;
	static inline std::string capturedScope;

	lua_State * L = luaL_newstate();

	LuaModScopeTest()
	{
		luaL_openlibs(L);
		lua_register(L, "capture", &LuaModScopeTest::capture);
	}

	~LuaModScopeTest() override
	{
		lua_close(L);
	}

	/// Stands for a binding like addUnitBonus: reads its arguments while called from the script
	static int capture(lua_State * L)
	{
		LuaStack S(L);
		S.get(1, captured);
		ScriptModScope scope;
		S.get(2, scope);
		capturedScope = scope.value;
		return 0;
	}

	void run(const std::string & chunkName, const std::string & code, const JsonNode & input)
	{
		ASSERT_EQ(luaL_loadbuffer(L, code.data(), code.size(), chunkName.c_str()), 0) << lua_tostring(L, -1);
		LuaStack S(L);
		S.push(input);
		ASSERT_EQ(lua_pcall(L, 1, 0, 0), 0) << lua_tostring(L, -1);
	}
};

TEST_F(LuaModScopeTest, ShallowCopyKeepsScopeOfEveryNode)
{
	JsonNode input;
	input["subtype"].String() = "spell.acidBurst";
	input["limiters"]["creature"].String() = "abyssYuanTiLeader";
	input["patched"].String() = "spell.other";
	input.setModScope("moda");
	input["limiters"].setModScope("modc");
	input["patched"].setModScope("modb");

	run("script:test", R"(
		local b = ...
		local copy = {}
		for k, v in pairs(b) do copy[k] = v end
		copy.val = 1
		capture(setmetatable(copy, getmetatable(b)))
	)", input);

	EXPECT_EQ(captured.getModScope(), "moda");
	EXPECT_EQ(captured["subtype"].getModScope(), "moda");
	EXPECT_EQ(captured["val"].getModScope(), "moda");
	EXPECT_EQ(captured["patched"].getModScope(), "modb");
	EXPECT_EQ(captured["limiters"].getModScope(), "modc");
	EXPECT_EQ(captured["limiters"]["creature"].getModScope(), "modc");
	EXPECT_EQ(captured["limiters"]["creature"].String(), "abyssYuanTiLeader");
}

TEST_F(LuaModScopeTest, ScriptValuesUseScopeOfScript)
{
	run("script:test", R"(
		capture({ subtype = "spell.heatStroke", limiters = { { type = "CREATURE_TYPE_LIMITER" } } })
	)", JsonNode());

	EXPECT_EQ(captured["subtype"].getModScope(), "script");
	EXPECT_EQ(captured["limiters"][0]["type"].getModScope(), "script");
	EXPECT_EQ(capturedScope, "script");
}

TEST_F(LuaModScopeTest, ExplicitScopeOverridesScopeOfScript)
{
	run("script:test", R"(
		capture("core:magicArrow", "game")
	)", JsonNode());

	EXPECT_EQ(capturedScope, "game");
}

}
