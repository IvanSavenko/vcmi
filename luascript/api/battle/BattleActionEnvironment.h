/*
 * BattleActionEnvironment.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"
#include "../../../lib/battle/Destination.h"
#include "../../../lib/battle/actions/IBattleActionEnvironment.h"

namespace spells
{
class Spell;
enum class Mode;
}

namespace scripting::api
{

class BattleActionEnvironmentProxy : public RawPointerWrapper<IBattleActionEnvironment, BattleActionEnvironmentProxy>
{
public:
	static constexpr std::string_view luaName = "BattleActionEnvironment";
	static constexpr std::string_view luaDescription =
		"Server operations available to a battle action while it is applied, bound to the battle of the action. "
		"Passed to the `apply` function of a battle action script.";

	static void registerMethods(MethodRegistrar & R);

	static void castSpell(IBattleActionEnvironment & object, const battle::Unit & caster, const spells::Spell & spell, spells::Mode mode, int level, const std::vector<battle::Destination> & target);
	static void fireCombatEvent(IBattleActionEnvironment & object, CombatEventType event, const battle::Unit & unit, const battle::Unit * other, const spells::Spell * spell);
	static void triggerObstaclesUnder(IBattleActionEnvironment & object, const battle::Unit & unit);
	static int64_t rngInt(IBattleActionEnvironment & object, int64_t low, int64_t high);
};

}
