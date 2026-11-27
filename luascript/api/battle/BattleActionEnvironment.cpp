/*
 * BattleActionEnvironment.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleActionEnvironment.h"

#include "Unit.h"
#include "../library/Spell.h"

#include "../../LuaStack.h"
#include "../../../lib/battle/Unit.h"
#include "../../../lib/spells/CSpell.h"

#include <vstd/RNG.h>

namespace scripting::api
{

void BattleActionEnvironmentProxy::registerMethods(MethodRegistrar & R)
{
	R.function<&BattleActionEnvironmentProxy::castSpell>("castSpell",
		{
			{"caster", "Unit whose ability casts the spell."},
			{"spell",  "Spell to cast."},
			{"mode",   "Way the spell is cast, from `ENUM.SpellMode`."},
			{"level",  "Mastery level of the spell."},
			{"target", "Destinations the spell is aimed at."}
		}, {},
		"Casts the spell of the unit at the target, with its animation, its effects and the battle log line.");
	R.function<&BattleActionEnvironmentProxy::fireCombatEvent>("fireCombatEvent",
		{
			{"event", "Event to fire, from `ENUM.CombatEvent`."},
			{"unit",  "Unit the event happens to."},
			{"other", "Other unit of the event, or nil."},
			{"spell", "Spell the event is about, or nil."}
		}, {},
		"Runs the combat event scripts of the unit and of every other unit reacting to the event.");
	R.function<&BattleActionEnvironmentProxy::triggerObstaclesUnder>("triggerObstaclesUnder",
		{{"unit", "Unit whose hexes are checked."}}, {},
		"Triggers the obstacles on the hexes the unit occupies.");
	R.function<&BattleActionEnvironmentProxy::rngInt>("rngInt",
		{
			{"low",  "Inclusive lower bound."},
			{"high", "Inclusive upper bound."}
		},
		{"Random integer in [low, high]."},
		"Returns a random integer from the random generator of the server.");
}

void BattleActionEnvironmentProxy::castSpell(IBattleActionEnvironment & object, const battle::Unit & caster, const spells::Spell & spell, spells::Mode mode, int level, const std::vector<battle::Destination> & target)
{
	object.castSpell(caster, spell.getId().toSpell(), mode, level, target);
}

void BattleActionEnvironmentProxy::fireCombatEvent(IBattleActionEnvironment & object, CombatEventType event, const battle::Unit & unit, const battle::Unit * other, const spells::Spell * spell)
{
	object.fireCombatEvent(event, &unit, other, spell ? spell->getId().toSpell() : nullptr);
}

void BattleActionEnvironmentProxy::triggerObstaclesUnder(IBattleActionEnvironment & object, const battle::Unit & unit)
{
	object.triggerObstaclesUnder(unit);
}

int BattleActionEnvironmentProxy::rngInt(IBattleActionEnvironment & object, int low, int high)
{
	return object.getRandom().nextInt(low, high);
}

}
