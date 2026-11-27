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

bool LuaBattleActionType::isAvailable(const CBattleInfoCallback & battle, const battle::Unit & actor, const ActionOption & option) const
{
	return contextOf(battle)->callMethod<bool>("isAvailable", noParameters(), &battle, &actor, api::ActionOptionPayload(option));
}

int LuaBattleActionType::getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const
{
	const auto priority = contextOf(battle)->callMethod<std::optional<int>>("getPriority", noParameters(), &battle, api::ActionOptionPayload(option), &actor, target);
	return priority.value_or(getConfig().priority);
}

ActionPreview LuaBattleActionType::preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	const bool legal = isLegal(game, battle, option, context);
	const auto descriptor = contextOf(battle)->callMethod<api::ActionPreviewDescriptor>("preview", noParameters(), &battle, api::ActionOptionPayload(option), api::ActionContextPayload(context, legal));
	return descriptor.toPreview();
}

std::optional<battle::Target> LuaBattleActionType::buildTarget(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const
{
	return contextOf(battle)->callMethod<std::optional<battle::Target>>("buildTarget", noParameters(), &battle, api::ActionOptionPayload(option), api::ActionContextPayload(context, false));
}

bool LuaBattleActionType::validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const
{
	return contextOf(battle)->callMethod<bool>("validate", noParameters(), &battle, api::BattleActionPayload(battle, action), &problem);
}

void LuaBattleActionType::apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const
{
	contextOf(battle)->callMethod<void>("apply", noParameters(), &env, &battle, api::BattleActionPayload(battle, action));
}

}
