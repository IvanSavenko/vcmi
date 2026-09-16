/*
 * ICombatActionCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../battle/BattleHex.h"

#include <vcmi/scripting/ApiTags.h>

class IBattleInfoCallback;

namespace battle
{
class Unit;
}

/// Moving and attacking, as the engine does them when a unit acts.
///
/// Deliberately not part of ServerCallback. An attack fires combat events, and a combat event script
/// holds a ServerCallback - so putting these there lets such a script ask for another attack from
/// inside the one that woke it, without end. Only a combat action is handed one of these, which
/// bounds the chain at action -> attack -> combat script and no further.
class DLL_LINKAGE ICombatActionCallback : public scripting::ApiRawPointer<ICombatActionCallback>
{
public:
	virtual ~ICombatActionCallback() = default;

	/// Walks the unit to the destination the way a move action does - along a path, and triggering
	/// whatever it crosses. Answers how far it actually got, which is what a charge scales with, and
	/// which may be short of the destination when something stopped it.
	virtual int walkUnit(const IBattleInfoCallback & battle, const battle::Unit & unit, const BattleHex & destination) = 0;

	/// Runs one melee attack to its end: first strike, every blow the attacker is entitled to, the
	/// retaliation, and the expiry of bonuses that last for the sequence.
	virtual void performAttack(const IBattleInfoCallback & battle, const battle::Unit & attacker, const battle::Unit & defender, const BattleHex & targetHex, int distance) = 0;
};
