/*
 * ScriptedActionEvaluationTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../server/battles/BattleTestFixture.h"

#include "AI/BattleAI/StackWithBonuses.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/bonuses/BonusParameters.h"
#include "../../lib/combatScripts/ICombatActionScript.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/modding/ModScope.h"
#include "../../server/CGameHandler.h"

/// The AI values an action it knows nothing about by running it on a copy of the battle and looking
/// at what changed. That only works if the script reaches the copy rather than the real battle.
class ScriptedActionEvaluationTest : public BattleTestFixture
{
public:
	static constexpr int32_t damagePerVictim = 500;

	static constexpr int actorHex = 5 * GameConstants::BFIELD_WIDTH + 5;
	static constexpr int aimedHex = 5 * GameConstants::BFIELD_WIDTH + 10;
	static constexpr int adjacentHex = aimedHex + 1;
	static constexpr int loneHex = 8 * GameConstants::BFIELD_WIDTH + 10;

	CStack * actor = nullptr;
	ScriptID script;

	void setUpBattle()
	{
		startGame();
		startBattle();

		actor = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(actorHex), 10);
		addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(aimedHex), 1000);
		addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(adjacentHex), 1000);
		addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(loneHex), 1000);
		ASSERT_NE(actor, nullptr);

		script = scriptByName("vcmi-test:probeAction");

		JsonNode parameters;
		parameters["damage"].Integer() = damagePerVictim;

		auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::COMBAT_ACTION, BonusSource::OTHER, 0, BonusSourceID(), BonusSubtypeID(script));
		bonus->parameters = std::make_shared<BonusParameters>(parameters);
		actor->addNewBonus(bonus);

		beginCombat();
	}

	/// The battle the AI simulates on, over the one that is really being fought.
	std::shared_ptr<HypotheticBattle> makeHypotheticBattle()
	{
		auto subject = std::shared_ptr<CBattleInfoCallback>(battle(), [](CBattleInfoCallback *){});
		return std::make_shared<HypotheticBattle>(gameHandler.get(), subject);
	}

	/// Health every unit of the given battle has left, keyed by unit.
	static std::map<uint32_t, int64_t> healthOf(const CBattleInfoCallback & source, const battle::Units & units)
	{
		std::map<uint32_t, int64_t> result;
		for(const auto * unit : units)
			result[unit->unitId()] = source.battleGetUnitByID(unit->unitId())->getAvailableHealth();
		return result;
	}

	/// Runs the action against the simulated battle and answers the health lost in it, per unit.
	std::map<uint32_t, int64_t> simulate(HypotheticBattle & state, const BattleHex & target)
	{
		const battle::Units units = battle()->battleGetUnitsIf([](const battle::Unit *){ return true; });
		auto before = healthOf(state, units);

		const battle::Unit * simulatedActor = state.battleGetUnitByID(actor->unitId());
		ScriptedActionInfo info = state.getScriptedAction(simulatedActor, script);
		if(!info.script)
		{
			ADD_FAILURE() << "unit does not offer the scripted action";
			return {};
		}

		info.script->execute(state.getServerCallback(), state.getCombatActionCallback(), state, simulatedActor, BattleHexArray({target}), info.parameters);

		auto after = healthOf(state, units);

		std::map<uint32_t, int64_t> lost;
		for(const auto & entry : before)
			lost[entry.first] = entry.second - after[entry.first];
		return lost;
	}
};

TEST_F(ScriptedActionEvaluationTest, actionReachesTheSimulatedBattleAndNotTheRealOne)
{
	setUpBattle();

	const battle::Units units = battle()->battleGetUnitsIf([](const battle::Unit *){ return true; });
	auto realBefore = healthOf(*battle(), units);

	auto state = makeHypotheticBattle();
	auto lost = simulate(*state, BattleHex(aimedHex));

	// the aimed unit and the one beside it were hit, in the simulation only
	EXPECT_EQ(lost[battle()->battleGetUnitByPos(BattleHex(aimedHex), true)->unitId()], damagePerVictim);
	EXPECT_EQ(lost[battle()->battleGetUnitByPos(BattleHex(adjacentHex), true)->unitId()], damagePerVictim);
	EXPECT_EQ(lost[battle()->battleGetUnitByPos(BattleHex(loneHex), true)->unitId()], 0);

	EXPECT_EQ(healthOf(*battle(), units), realBefore) << "simulating the action changed the real battle";
}

/// What makes the AI able to prefer a special attack: aiming where it catches two units is worth
/// more than aiming where it catches one, and the difference is visible without the engine knowing
/// anything about what the action does.
TEST_F(ScriptedActionEvaluationTest, aimCatchingTwoUnitsIsWorthMoreThanOne)
{
	setUpBattle();

	auto pairState = makeHypotheticBattle();
	auto lostOnPair = simulate(*pairState, BattleHex(aimedHex));

	auto loneState = makeHypotheticBattle();
	auto lostOnLone = simulate(*loneState, BattleHex(loneHex));

	const auto total = [](const std::map<uint32_t, int64_t> & lost)
	{
		int64_t sum = 0;
		for(const auto & entry : lost)
			sum += entry.second;
		return sum;
	};

	EXPECT_EQ(total(lostOnPair), 2 * damagePerVictim);
	EXPECT_EQ(total(lostOnLone), damagePerVictim);
	EXPECT_GT(total(lostOnPair), total(lostOnLone));
}
