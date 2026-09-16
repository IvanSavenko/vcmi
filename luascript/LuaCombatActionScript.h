/*
 * LuaCombatActionScript.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../lib/combatScripts/ICombatActionScript.h"

namespace scripting
{
class LuaContext;
class LuaScriptInstance;

/// Forwards a scripted battle action to the Lua methods of the script backing it.
class LuaCombatActionScript final : public ICombatActionScript
{
public:
	LuaCombatActionScript(const LuaScriptInstance * script);
	~LuaCombatActionScript() override;

	BattleHexArray getSelectableHexes(const CBattleInfoCallback & battle, const battle::Unit * unit, const JsonNode & parameters) const override;
	bool validateTargets(const CBattleInfoCallback & battle, const battle::Unit * unit, const BattleHexArray & targets, const JsonNode & parameters) const override;
	BattleHexArray getAffectedHexes(const CBattleInfoCallback & battle, const battle::Unit * unit, const BattleHexArray & targets, const JsonNode & parameters) const override;
	std::string getCursor(const CBattleInfoCallback & battle, const battle::Unit * unit, const BattleHexArray & targets, const JsonNode & parameters) const override;
	MetaString getStatusMessage(const CBattleInfoCallback & battle, const battle::Unit * unit, const BattleHexArray & targets, const JsonNode & parameters) const override;
	void execute(ServerCallback * server, ICombatActionCallback * actions, const CBattleInfoCallback & battle, const battle::Unit * unit, const BattleHexArray & targets, const JsonNode & parameters) const override;

private:
	const LuaScriptInstance * script;

	std::shared_ptr<LuaContext> contextOf(const CBattleInfoCallback & battle) const;
};

}
