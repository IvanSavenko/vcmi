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
#include "../json/JsonNode.h"
#include "../texts/MetaString.h"

class ICombatActionCallback;

class CBattleInfoCallback;
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

	/// Whether aiming the action this way is legal. `targets` holds what the owner aimed at, its
	/// first entry already checked against getSelectableHexes. The base script answers false for any
	/// target beyond the first, so an action receives more than one only by saying that it wants to -
	/// the extra ones arrive from the client and are otherwise unchecked.
	virtual bool validateTargets(const CBattleInfoCallback & battle, const battle::Unit * unit, const BattleHexArray & targets, const JsonNode & parameters) const = 0;

	/// Hexes the action would affect if used as aimed, which is what the player sees shaded. May be
	/// empty for an action with nothing to show.
	virtual BattleHexArray getAffectedHexes(const CBattleInfoCallback & battle, const battle::Unit * unit, const BattleHexArray & targets, const JsonNode & parameters) const = 0;

	/// Which of the cursors the script declares to show, by the name it declared it under. An empty
	/// answer keeps whatever the engine chose.
	virtual std::string getCursor(const CBattleInfoCallback & battle, const battle::Unit * unit, const BattleHexArray & targets, const JsonNode & parameters) const = 0;

	/// Line for the status bar, left unresolved so that each client renders it in its own language.
	virtual MetaString getStatusMessage(const CBattleInfoCallback & battle, const battle::Unit * unit, const BattleHexArray & targets, const JsonNode & parameters) const = 0;

	/// Carries the action out. `targets` is what the owner aimed at, its first entry being the hex
	/// the action was aimed at. `actions` carries moving and attacking, which only an action may ask
	/// for - see ICombatActionCallback for why they are not on `server`.
	virtual void execute(ServerCallback * server, ICombatActionCallback * actions, const CBattleInfoCallback & battle, const battle::Unit * unit, const BattleHexArray & targets, const JsonNode & parameters) const = 0;
};

/// One scripted action a unit offers, resolved from the bonus granting it.
struct DLL_LINKAGE ScriptedActionInfo
{
	const ICombatActionScript * script = nullptr;
	JsonNode parameters;
};
