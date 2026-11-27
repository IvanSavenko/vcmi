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
#include "../../bonuses/Bonus.h"

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
	/// Bonus types that grant the action. Without any, the action is offered to every unit
	std::vector<BonusType> triggers;
};

/// Unit action type with its rules in a script and its static properties in config.
/// Offers one option per distinct subtype of the trigger bonuses of the unit
class DLL_LINKAGE ScriptedBattleActionType : public BattleActionType
{
	ScriptedBattleActionConfig config;

protected:
	/// Checks that the type offers the option to the unit on its turn. Bonuses are the trigger bonuses of the option
	virtual bool isAvailable(const CBattleInfoCallback & battle, const battle::Unit & actor, const ActionOption & option, const BonusList & bonuses) const = 0;
	/// Rules of the type for validate, asked once the acting unit may act and has the trigger bonuses of the action
	virtual bool validateRules(const CBattleInfoCallback & battle, const BattleAction & action, const BonusList & bonuses, spells::Problem & problem) const = 0;

public:
	ScriptedBattleActionType(BattleActionID id, ScriptedBattleActionConfig config);

	const ScriptedBattleActionConfig & getConfig() const;

	/// Trigger bonuses of the unit with the subtype. An action without a subtype gets them when all of them share one subtype.
	/// Nullptr if the unit has none of them, empty for a type without triggers
	TConstBonusListPtr getTriggerBonuses(const battle::Unit & actor, const BonusSubtypeID & subtype, bool exactSubtype) const;

	bool isUnitAction() const final;
	bool isTacticsAction() const final;
	std::optional<battle::UnitTurnStatus> getStartTurnStatus(const BattleAction & action) const final;
	bool usesUnitSpellcast() const final;

	void collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const final;
	bool validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const final;
};
