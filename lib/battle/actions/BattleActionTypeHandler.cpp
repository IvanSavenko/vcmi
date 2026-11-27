/*
 * BattleActionTypeHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleActionTypeHandler.h"

#include "CatapultAction.h"
#include "MeleeAttackActions.h"
#include "MoveAction.h"
#include "NoTargetActions.h"
#include "RangedAttackAction.h"
#include "ScriptedBattleActionType.h"
#include "SpellCastActions.h"

#include "../../GameLibrary.h"
#include "../../json/JsonNode.h"
#include "../../modding/IdentifierStorage.h"
#include "../../modding/LibraryLoadContext.h"
#include "../../modding/ModScope.h"
#include "../../scripting/ScriptService.h"
#include "../../texts/CGeneralTextHandler.h"
#include "../../texts/TextIdentifier.h"

static const SkipTurnAction skipTurnAction(BattleActionID::SKIP_TURN);
static const BadMoraleAction badMoraleAction(BattleActionID::BAD_MORALE);
static const WaitAction waitAction(BattleActionID::WAIT);
static const DefendAction defendAction(BattleActionID::DEFEND);
static const EndTacticsPhaseAction endTacticsPhaseAction(BattleActionID::END_TACTICS_PHASE);
static const RetreatAction retreatAction(BattleActionID::RETREAT);
static const SurrenderAction surrenderAction(BattleActionID::SURRENDER);
static const MoveAction moveAction(BattleActionID::MOVE);
static const RangedAttackAction rangedAttackAction(BattleActionID::RANGED_ATTACK);
static const CatapultAction catapultAction(BattleActionID::CATAPULT);
static const MeleeAttackAction meleeAttackAction(BattleActionID::MELEE_ATTACK);
static const MeleeAttackNoReturnAction meleeAttackNoReturnAction(BattleActionID::MELEE_ATTACK_NO_RETURN);
static const LongWeaponAttackAction longWeaponAttackAction(BattleActionID::LONG_WEAPON_ATTACK);
static const HeroSpellAction heroSpellAction(BattleActionID::HERO_SPELL);
static const UnitSpellAction unitSpellAction(BattleActionID::UNIT_SPELL);
static const RandomSpellAction randomSpellAction(BattleActionID::RANDOM_SPELL);
static const AdjacentSpellAction adjacentSpellAction(BattleActionID::ADJACENT_SPELL);

struct BuiltInActionType
{
	const BattleActionType * type;
	const char * identifier;
};

/// Built-in types written in C++. The others are loaded from config/battleActions.json
static const std::array<BuiltInActionType, 17> builtInTypes = {{
	{&skipTurnAction, "skipTurn"},
	{&badMoraleAction, "badMorale"},
	{&waitAction, "wait"},
	{&defendAction, "defend"},
	{&endTacticsPhaseAction, "endTacticsPhase"},
	{&retreatAction, "retreat"},
	{&surrenderAction, "surrender"},
	{&moveAction, "move"},
	{&rangedAttackAction, "rangedAttack"},
	{&catapultAction, "catapult"},
	{&meleeAttackAction, "meleeAttack"},
	{&meleeAttackNoReturnAction, "meleeAttackNoReturn"},
	{&longWeaponAttackAction, "longWeaponAttack"},
	{&heroSpellAction, "heroSpell"},
	{&unitSpellAction, "unitSpell"},
	{&randomSpellAction, "randomSpell"},
	{&adjacentSpellAction, "adjacentSpell"},
}};

/// Every built-in type in the order of collected options, which decides between options with equal priority
static const std::array<BattleActionID, 18> builtInOptionOrder = {
	BattleActionID::SKIP_TURN,
	BattleActionID::BAD_MORALE,
	BattleActionID::WAIT,
	BattleActionID::DEFEND,
	BattleActionID::END_TACTICS_PHASE,
	BattleActionID::RETREAT,
	BattleActionID::SURRENDER,
	BattleActionID::MOVE,
	BattleActionID::RANGED_ATTACK,
	BattleActionID::HEAL,
	BattleActionID::CATAPULT,
	BattleActionID::MELEE_ATTACK,
	BattleActionID::MELEE_ATTACK_NO_RETURN,
	BattleActionID::LONG_WEAPON_ATTACK,
	BattleActionID::HERO_SPELL,
	BattleActionID::UNIT_SPELL,
	BattleActionID::RANDOM_SPELL,
	BattleActionID::ADJACENT_SPELL,
};

static std::optional<battle::UnitTurnStatus> parseTurnStatus(const std::string & name)
{
	if(name == "waiting")
		return battle::UnitTurnStatus::WAITING;
	if(name == "defended")
		return battle::UnitTurnStatus::DEFENDED;
	if(name == "keep")
		return std::nullopt;
	return battle::UnitTurnStatus::ACTED;
}

static UnitActionButton parseButton(LibraryLoadContext & context, const std::string & scope, const std::string & name, const JsonNode & data)
{
	UnitActionButton button;
	button.order = data["order"].Integer();
	button.icon = ImagePath::fromJson(data["icon"]);

	const std::string & tooltip = data["tooltip"].String();
	if(!tooltip.empty() && tooltip[0] == '@')
	{
		button.tooltipTextID = tooltip.substr(1);
	}
	else
	{
		button.tooltipTextID = TextIdentifier("battleAction", scope, name, "tooltip").get();
		context.lib.generaltexth->registerString(scope, button.tooltipTextID, data["tooltip"]);
	}
	return button;
}

BattleActionTypeHandler::BattleActionTypeHandler(LibraryLoadContext & context)
{
	typesById.resize(builtInOptionOrder.size());
	jsonKeys.resize(builtInOptionOrder.size());

	for(const auto & builtIn : builtInTypes)
	{
		const auto id = static_cast<int32_t>(builtIn.type->getId());
		typesById.at(id) = builtIn.type;
		jsonKeys.at(id) = ModScope::scopeBuiltin() + ":" + builtIn.identifier;
		context.lib.identifiersHandler->registerObject(ModScope::scopeBuiltin(), "battleAction", builtIn.identifier, id);
	}
}

BattleActionTypeHandler::~BattleActionTypeHandler() = default;

std::vector<JsonNode> BattleActionTypeHandler::loadLegacyData(LibraryLoadContext & context)
{
	return {};
}

void BattleActionTypeHandler::loadObject(LibraryLoadContext & context, const std::string & scope, const std::string & name, const JsonNode & data)
{
	loadScriptedType(context, scope, name, data, std::max(typesById.size(), builtInOptionOrder.size()));
}

void BattleActionTypeHandler::loadObject(LibraryLoadContext & context, const std::string & scope, const std::string & name, const JsonNode & data, size_t index)
{
	if(index >= builtInOptionOrder.size() || typesById.at(index) || !jsonKeys.at(index).empty())
		throw std::runtime_error("Battle action '" + name + "' has index " + std::to_string(index) + ", which is not a free built-in slot");

	loadScriptedType(context, scope, name, data, index);
}

void BattleActionTypeHandler::loadScriptedType(LibraryLoadContext & context, const std::string & scope, const std::string & name, const JsonNode & data, size_t index)
{
	ScriptedBattleActionConfig config;
	config.priority = data["priority"].Integer();
	config.turnStatus = parseTurnStatus(data["turnStatus"].String());
	config.usesUnitSpellcast = data["usesUnitSpellcast"].Bool();
	config.tacticsAction = data["tacticsAction"].Bool();
	if(!data["button"].isNull())
		config.button = parseButton(context, scope, name, data["button"]);

	if(typesById.size() <= index)
	{
		typesById.resize(index + 1);
		jsonKeys.resize(index + 1);
	}
	jsonKeys.at(index) = scope + ":" + name;
	registerObject(context, scope, "battleAction", name, data, index);

	context.resolver.requestIdentifier("script", data["script"], [this, &context, config, index](int32_t scriptIndex)
	{
		const ScriptID scriptID(scriptIndex);
		const ScriptService & scripts = *context.lib.scriptTypes();
		const ScriptTypeDescription & script = scripts.getById(scriptID);

		if(script.kind != ScriptKind::BATTLE_ACTION)
		{
			logMod->error("Battle action '%s' uses script '%s', which is not a battle action script! The action will not be loaded.", jsonKeys.at(index), script.scriptId);
			return;
		}

		auto type = scripts.createBattleActionType(scriptID, BattleActionID(static_cast<int32_t>(index)), config);
		typesById.at(index) = type.get();
		scriptedTypes.push_back(std::move(type));
	});
}

void BattleActionTypeHandler::afterLoadFinalization(LibraryLoadContext & context)
{
	types.clear();

	for(const auto id : builtInOptionOrder)
	{
		const auto * type = typesById.at(static_cast<int32_t>(id));
		if(!type)
			throw std::runtime_error("Built-in battle action type " + std::to_string(static_cast<int32_t>(id)) + " was not loaded");
		types.push_back(type);
	}

	for(size_t id = builtInOptionOrder.size(); id < typesById.size(); ++id)
		if(typesById[id])
			types.push_back(typesById[id]);
}

const BattleActionType * BattleActionTypeHandler::getById(int32_t id) const
{
	if(id < 0 || id >= static_cast<int32_t>(typesById.size()))
		return nullptr;
	return typesById[id];
}

const std::vector<const BattleActionType *> & BattleActionTypeHandler::getAll() const
{
	return types;
}

const std::string & BattleActionTypeHandler::getJsonKey(int32_t id) const
{
	if(!getById(id))
		throw std::runtime_error("No battle action type has id " + std::to_string(id));
	return jsonKeys[id];
}
