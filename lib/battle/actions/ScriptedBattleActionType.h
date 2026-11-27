/*
 * ScriptedBattleActionType.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BattleActionType.h"

#include "../UnitTurnStatus.h"

/// Properties of a scripted battle action type that its config entry declares
struct ScriptedBattleActionConfig
{
	/// Default of getPriority, used when the script returns none
	int priority = 0;
	std::optional<UnitActionButton> button;
	/// Turn status of the acting unit once the action starts, or nothing to keep it
	std::optional<battle::UnitTurnStatus> turnStatus = battle::UnitTurnStatus::ACTED;
	bool usesUnitSpellcast = false;
	bool tacticsAction = false;
};

/// Unit action type with its rules in a script and its static properties in config
class DLL_LINKAGE ScriptedBattleActionType : public BattleActionType
{
	ScriptedBattleActionConfig config;

protected:
	/// Checks that the type offers the option to the unit on its turn
	virtual bool isAvailable(const CBattleInfoCallback & battle, const battle::Unit & actor, const ActionOption & option) const = 0;

public:
	ScriptedBattleActionType(BattleActionID id, ScriptedBattleActionConfig config);

	const ScriptedBattleActionConfig & getConfig() const;

	bool isUnitAction() const final;
	bool isTacticsAction() const final;
	std::optional<battle::UnitTurnStatus> getStartTurnStatus(const BattleAction & action) const final;
	bool usesUnitSpellcast() const final;

	void collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const final;
};
