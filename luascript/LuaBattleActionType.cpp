/*
 * LuaBattleActionType.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LuaBattleActionType.h"

#include "LuaContext.h"
#include "LuaScriptInstance.h"
#include "LuaScriptPool.h"
#include "api/battle/BattleActionPayloads.h"

#include "../lib/battle/BattleAction.h"
#include "../lib/battle/CBattleInfoCallback.h"
#include "../lib/battle/actions/IBattleActionEnvironment.h"

namespace scripting
{

/// self of every call. Action scripts take no parameters of their own
static const JsonNode & noParameters()
{
	static const JsonNode parameters(JsonMap{});
	return parameters;
}

LuaBattleActionType::LuaBattleActionType(const LuaScriptInstance * script, BattleActionID id, ScriptedBattleActionConfig config)
	: ScriptedBattleActionType(id, std::move(config))
	, script(script)
{
}

LuaBattleActionType::~LuaBattleActionType() = default;

std::shared_ptr<LuaContext> LuaBattleActionType::contextOf(const CBattleInfoCallback & battle) const
{
	return LuaContext::of(battle.getScriptContextPool(), script);
}

BonusList LuaBattleActionType::getOptionBonuses(const battle::Unit * actor, const ActionOption & option) const
{
	const auto bonuses = actor ? getTriggerBonuses(*actor, option.subtype, true) : nullptr;
	return bonuses ? *bonuses : BonusList();
}

bool LuaBattleActionType::isAvailable(const CBattleInfoCallback & battle, const battle::Unit & actor, const ActionOption & option, const BonusList & bonuses) const
{
	return contextOf(battle)->callMethod<bool>("isAvailable", noParameters(), &battle, &actor, api::ActionOptionPayload(option, bonuses));
}

int LuaBattleActionType::getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const
{
	const api::ActionOptionPayload optionPayload(option, getOptionBonuses(&actor, option));
	const auto priority = contextOf(battle)->callMethod<std::optional<int>>("getPriority", noParameters(), &battle, optionPayload, &actor, target);
	return priority.value_or(getConfig().priority);
}

ActionPreview LuaBattleActionType::preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	const bool legal = isLegal(game, battle, option, context);
	const api::ActionOptionPayload optionPayload(option, getOptionBonuses(context.actor, option));
	const auto descriptor = contextOf(battle)->callMethod<api::ActionPreviewDescriptor>("preview", noParameters(), &battle, optionPayload, api::ActionContextPayload(context, legal));
	return descriptor.toPreview();
}

std::optional<battle::Target> LuaBattleActionType::buildTarget(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	const api::ActionOptionPayload optionPayload(option, getOptionBonuses(context.actor, option));
	return contextOf(battle)->callMethod<std::optional<battle::Target>>("buildTarget", noParameters(), &battle, optionPayload, api::ActionContextPayload(context, false));
}

bool LuaBattleActionType::validateRules(const CBattleInfoCallback & battle, const BattleAction & action, const BonusList & bonuses, spells::Problem & problem) const
{
	return contextOf(battle)->callMethod<bool>("validate", noParameters(), &battle, api::BattleActionPayload(battle, action, bonuses), &problem);
}

void LuaBattleActionType::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	const auto bonuses = getTriggerBonuses(getActor(battle, action), action.subtype, action.subtype.hasValue());
	if(!bonuses)
		throw std::runtime_error("Battle action applied without the trigger bonuses that its validation requires: " + action.toString());

	contextOf(battle)->callMethod<void>("apply", noParameters(), &env, &battle, api::BattleActionPayload(battle, action, *bonuses));
}

}
