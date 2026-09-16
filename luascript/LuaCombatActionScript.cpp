/*
 * LuaCombatActionScript.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LuaCombatActionScript.h"

#include "LuaContext.h"
#include "LuaScriptInstance.h"
#include "LuaScriptPool.h"

#include "api/LuaMetaString.h"

#include <vcmi/ServerCallback.h>

#include "../lib/battle/CBattleInfoCallback.h"
#include "../lib/battle/Unit.h"
#include "../lib/combatScripts/ICombatActionCallback.h"

namespace scripting
{

static const std::string GET_SELECTABLE_HEXES = "getSelectableHexes";
static const std::string VALIDATE_TARGETS = "validateTargets";
static const std::string GET_AFFECTED_HEXES = "getAffectedHexes";
static const std::string GET_CURSOR = "getCursor";
static const std::string GET_STATUS_MESSAGE = "getStatusMessage";
static const std::string EXECUTE = "execute";

LuaCombatActionScript::LuaCombatActionScript(const LuaScriptInstance * script)
	: script(script)
{
}

LuaCombatActionScript::~LuaCombatActionScript() = default;

std::shared_ptr<LuaContext> LuaCombatActionScript::contextOf(const CBattleInfoCallback & battle) const
{
	return LuaContext::of(battle.getScriptContextPool(), script);
}

BattleHexArray LuaCombatActionScript::getSelectableHexes(const CBattleInfoCallback & battle, const battle::Unit * unit, const JsonNode & parameters) const
{
	// a script cannot construct a BattleHexArray, so it is handed an empty one to fill - the same
	// arrangement spell effects use for adjustAffectedHexes
	return contextOf(battle)->callMethod<BattleHexArray>(GET_SELECTABLE_HEXES, parameters, &battle, unit, BattleHexArray());
}

bool LuaCombatActionScript::validateTargets(const CBattleInfoCallback & battle, const battle::Unit * unit, const BattleHexArray & targets, const JsonNode & parameters) const
{
	return contextOf(battle)->callMethod<bool>(VALIDATE_TARGETS, parameters, &battle, unit, targets);
}

BattleHexArray LuaCombatActionScript::getAffectedHexes(const CBattleInfoCallback & battle, const battle::Unit * unit, const BattleHexArray & targets, const JsonNode & parameters) const
{
	return contextOf(battle)->callMethod<BattleHexArray>(GET_AFFECTED_HEXES, parameters, &battle, unit, targets, BattleHexArray());
}

std::string LuaCombatActionScript::getCursor(const CBattleInfoCallback & battle, const battle::Unit * unit, const BattleHexArray & targets, const JsonNode & parameters) const
{
	return contextOf(battle)->callMethod<std::string>(GET_CURSOR, parameters, &battle, unit, targets);
}

MetaString LuaCombatActionScript::getStatusMessage(const CBattleInfoCallback & battle, const battle::Unit * unit, const BattleHexArray & targets, const JsonNode & parameters) const
{
	return contextOf(battle)->callMethod<api::LuaMetaString>(GET_STATUS_MESSAGE, parameters, &battle, unit, targets).toMetaString();
}

void LuaCombatActionScript::execute(ServerCallback * server, ICombatActionCallback * actions, const CBattleInfoCallback & battle, const battle::Unit * unit, const BattleHexArray & targets, const JsonNode & parameters) const
{
	contextOf(battle)->callMethod<void>(EXECUTE, parameters, server, actions, &battle, unit, targets);
}

}
