/*
 * ICombatActionScript.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../battle/BattleHexArray.h"

class CBattleInfoCallback;
class JsonNode;
class ServerCallback;

namespace battle
{
class Unit;
}

/// One battle action a unit offers its owner, defined by a script rather than by the engine.
/// A unit carries one COMBAT_ACTION bonus per action it offers, so the same script may back
/// several of them with different parameters. Implementations are stateless and shared.
class DLL_LINKAGE ICombatActionScript
{
public:
	virtual ~ICombatActionScript() = default;

	/// Hexes the owner may aim this action at. An empty answer means the action is unavailable this
	/// turn. Also what the server validates an incoming action against, so it must not consult
	/// anything the client cannot see.
	virtual BattleHexArray getSelectableHexes(const CBattleInfoCallback & battle, const battle::Unit * unit, const JsonNode & parameters) const = 0;

	/// Carries the action out. `targets` is what the owner aimed at, its first entry being the hex
	/// the action was aimed at.
	virtual void execute(ServerCallback * server, const CBattleInfoCallback & battle, const battle::Unit * unit, const BattleHexArray & targets, const JsonNode & parameters) const = 0;
};
