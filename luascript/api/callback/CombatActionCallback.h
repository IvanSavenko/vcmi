/*
 * api/callback/CombatActionCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../../lib/combatScripts/ICombatActionCallback.h"

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"

namespace scripting::api
{

class CombatActionCallbackProxy : public RawPointerWrapper<ICombatActionCallback, CombatActionCallbackProxy>
{
public:
	static constexpr std::string_view luaName = "CombatActionCallback";
	static constexpr std::string_view luaDescription =
		"Moving and attacking, as the engine does them when a unit acts. Handed only to a combat "
		"action script, and deliberately absent from the server callback every other kind of script "
		"holds: an attack wakes combat event scripts, and one of those asking for another attack "
		"from inside it would never end.";

	static void registerMethods(MethodRegistrar & R);

	static int walkUnit(ICombatActionCallback & object, const IBattleInfoCallback & battle, const battle::Unit & unit, BattleHex destination);
	static void performAttack(ICombatActionCallback & object, const IBattleInfoCallback & battle, const battle::Unit & attacker, const battle::Unit & defender, BattleHex targetHex, int distance);
};

}
