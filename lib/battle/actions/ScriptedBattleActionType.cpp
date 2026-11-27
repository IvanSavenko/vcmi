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

ScriptedBattleActionType::ScriptedBattleActionType(BattleActionID id, ScriptedBattleActionConfig config)
	: BattleActionType(id)
	, config(std::move(config))
{
}

const ScriptedBattleActionConfig & ScriptedBattleActionType::getConfig() const
{
	return config;
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
	ActionOption option{this, config.button};
	if(isAvailable(battle, actor, option))
		out.push_back(option);
}
