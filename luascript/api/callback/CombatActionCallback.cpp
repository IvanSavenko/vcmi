/*
 * api/callback/CombatActionCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "CombatActionCallback.h"

#include "../../../lib/battle/IBattleInfoCallback.h"
#include "../../../lib/battle/Unit.h"

namespace scripting::api
{

void CombatActionCallbackProxy::registerMethods(MethodRegistrar & R)
{
	R.function<&CombatActionCallbackProxy::walkUnit>("walkUnit",
		{
			{"battle",      "Battle in which the unit walks."},
			{"unit",        "Unit to walk."},
			{"destination", "Hex to walk to."}
		}, {},
		"Walks the unit along a path to the destination the way a move action does, triggering "
		"whatever it crosses, and answers how far it got. That may be short of the destination when "
		"something stopped it, so a script that cares has to check where the unit ended up. Unlike "
		"server:moveUnit, which places the unit on a hex without it travelling there.");
	R.function<&CombatActionCallbackProxy::performAttack>("performAttack",
		{
			{"battle",    "Battle the attack happens in."},
			{"attacker",  "Unit making the attack."},
			{"defender",  "Unit being attacked."},
			{"targetHex", "Hex the blow lands on."},
			{"distance",  "Hexes the attacker travelled to reach it, which a charge scales with."}
		}, {},
		"Runs one melee attack to its end - first strike, every blow the attacker is entitled to, "
		"the retaliation - exactly as the engine runs the attack of a unit. The attack rules stay "
		"with the engine, so a script asking for one does not have to know any of them.");
}

int CombatActionCallbackProxy::walkUnit(ICombatActionCallback & object, const IBattleInfoCallback & battle, const battle::Unit & unit, BattleHex destination)
{
	return object.walkUnit(battle, unit, destination);
}

void CombatActionCallbackProxy::performAttack(ICombatActionCallback & object, const IBattleInfoCallback & battle, const battle::Unit & attacker, const battle::Unit & defender, BattleHex targetHex, int distance)
{
	object.performAttack(battle, attacker, defender, targetHex, distance);
}

}
