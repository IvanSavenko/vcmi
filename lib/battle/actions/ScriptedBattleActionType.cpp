/*
 * ScriptedBattleActionType.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ScriptedBattleActionType.h"

#include "../BattleAction.h"
#include "../CBattleInfoCallback.h"
#include "../Unit.h"
#include "../../bonuses/BonusList.h"
#include "../../spells/Problem.h"

ScriptedBattleActionType::ScriptedBattleActionType(BattleActionID id, ScriptedBattleActionConfig config)
	: BattleActionType(id)
	, config(std::move(config))
{
}

const ScriptedBattleActionConfig & ScriptedBattleActionType::getConfig() const
{
	return config;
}

TConstBonusListPtr ScriptedBattleActionType::getTriggerBonuses(const battle::Unit & actor, const BonusSubtypeID & subtype, bool exactSubtype) const
{
	auto result = std::make_shared<BonusList>();
	if(config.triggers.empty())
		return result;

	for(const auto trigger : config.triggers)
		for(const auto & bonus : *actor.getBonusesOfType(trigger))
			if(!exactSubtype || bonus->subtype == subtype)
				result->push_back(bonus);

	if(result->empty())
		return nullptr;

	if(!exactSubtype)
		for(const auto & bonus : *result)
			if(bonus->subtype != result->front()->subtype)
				return nullptr;

	return result;
}

bool ScriptedBattleActionType::isUnitAction() const
{
	return true;
}

bool ScriptedBattleActionType::isTacticsAction() const
{
	return config.tacticsAction;
}

std::optional<battle::UnitTurnStatus> ScriptedBattleActionType::getStartTurnStatus(const BattleAction & action) const
{
	return config.turnStatus;
}

bool ScriptedBattleActionType::usesUnitSpellcast() const
{
	return config.usesUnitSpellcast;
}

void ScriptedBattleActionType::collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const
{
	std::vector<BonusSubtypeID> subtypes;
	if(config.triggers.empty())
		subtypes.emplace_back();

	for(const auto trigger : config.triggers)
		for(const auto & bonus : *actor.getBonusesOfType(trigger))
			if(!vstd::contains(subtypes, bonus->subtype))
				subtypes.push_back(bonus->subtype);

	for(const auto & subtype : subtypes)
	{
		ActionOption option{this, config.button};
		option.subtype = subtype;
		if(isAvailable(battle, actor, option, *getTriggerBonuses(actor, subtype, true)))
			out.push_back(option);
	}
}

bool ScriptedBattleActionType::validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	const battle::Unit * unit = battle.battleGetUnitByID(action.stackNumber);
	if(!checkUnitCanAct(battle, unit, problem))
		return false;

	const auto bonuses = getTriggerBonuses(*unit, action.subtype, action.subtype.hasValue());
	if(!bonuses)
	{
		problem.add(MetaString::createFromTextID("vcmi.battle.problem.noActionAbility"));
		return false;
	}

	return validateRules(battle, action, *bonuses, problem);
}
