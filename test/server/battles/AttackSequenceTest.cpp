/*
 * AttackSequenceTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../lib/bonuses/BonusParameters.h"

namespace
{

struct AttackSequenceCase
{
	const char * name;
	const char * creature;
	bool returns;
};

}

class AttackSequenceTestBase : public BattleTestFixture
{
public:
	static constexpr int32_t attackerCount = 100;
	static constexpr int32_t defenderCount = 1000;

	/// Positions requiring movement before the melee attack
	static constexpr int originHex = leftHex;
	static constexpr int attackFromHex = leftHex + 3;
	static constexpr int targetHex = leftHex + 4;
};

class AttackSequenceTest : public AttackSequenceTestBase, public ::testing::WithParamInterface<AttackSequenceCase>
{
};

TEST_P(AttackSequenceTest, EndsWhereTheCreatureIsSupposedTo)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, creatureByName(GetParam().creature), BattleHex(originHex), attackerCount);
	CStack * defender = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(targetHex), defenderCount);

	// Keep the attacker alive for RETURN_AFTER_STRIKE.
	blockRetaliation(attacker);

	// Return movement requires initialized movement state.
	beginCombat();

	const int64_t healthBefore = defender->getAvailableHealth();

	ASSERT_TRUE(attackFrom(attacker, BattleHex(targetHex), BattleHex(attackFromHex)));
	ASSERT_LT(defender->getAvailableHealth(), healthBefore) << "attack dealt no damage";

	EXPECT_EQ(attacker->getPosition(), BattleHex(GetParam().returns ? originHex : attackFromHex));
}

INSTANTIATE_TEST_SUITE_P(Creatures, AttackSequenceTest, ::testing::Values(
	AttackSequenceCase{"harpyReturnsToStartingHex", "core:harpy", true},
	AttackSequenceCase{"griffinStaysWhereItStruck", "core:griffin", false}
),
	[](const ::testing::TestParamInfo<AttackSequenceCase> & info) { return info.param.name; });

/// The same battlefield, with the return driven by a script instead of by the engine.
class ScriptedAttackSequenceTest : public AttackSequenceTestBase
{
};

/// The same walk-and-attack driven by a script instead of by the engine, which is what the harpy
/// ability becomes. It has to end in the same place, having dealt the same damage - the script owns
/// only the return, and asks the engine for the attack.
TEST_F(ScriptedAttackSequenceTest, scriptedReturnEndsWhereTheEngineWouldHave)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, creatureByName("core:griffin"), BattleHex(originHex), attackerCount);
	CStack * defender = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(targetHex), defenderCount);
	ASSERT_NE(attacker, nullptr);

	blockRetaliation(attacker);

	const ScriptID script = scriptByName("core:attackAndReturn");
	auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::COMBAT_ACTION, BonusSource::OTHER, 0, BonusSourceID(), BonusSubtypeID(script));
	attacker->addNewBonus(bonus);

	beginCombat();

	const int64_t healthBefore = defender->getAvailableHealth();

	ASSERT_TRUE(useScriptedAction(attacker, script, BattleHex(targetHex)));

	EXPECT_LT(defender->getAvailableHealth(), healthBefore) << "the scripted attack dealt no damage";
	EXPECT_EQ(attacker->getPosition(), BattleHex(originHex)) << "the attacker did not fly back";
}

/// The owner picks which side to approach from, so the action takes a second target. It has to be
/// checked: everything past the first target arrives from the client and nothing else looks at it.
TEST_F(ScriptedAttackSequenceTest, approachHexIsHonouredAndChecked)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, creatureByName("core:griffin"), BattleHex(originHex), attackerCount);
	CStack * defender = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(targetHex), defenderCount);
	ASSERT_NE(attacker, nullptr);

	blockRetaliation(attacker);

	const ScriptID script = scriptByName("core:attackAndReturn");
	attacker->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::COMBAT_ACTION, BonusSource::OTHER, 0, BonusSourceID(), BonusSubtypeID(script)));

	beginCombat();

	// a hex nowhere near the victim is not a side of it, so the action must be refused outright
	EXPECT_FALSE(useScriptedAction(attacker, script, BattleHexArray({BattleHex(targetHex), BattleHex(originHex)})));
	EXPECT_EQ(defender->getAvailableHealth(), defenderCount * defender->getMaxHealth()) << "a refused action must not strike";

	// one of the victim's own sides is accepted, and is where the attacker strikes from
	const BattleHex approach(targetHex - 1);
	ASSERT_TRUE(useScriptedAction(attacker, script, BattleHexArray({BattleHex(targetHex), approach})));
	EXPECT_LT(defender->getAvailableHealth(), defenderCount * defender->getMaxHealth());
	EXPECT_EQ(attacker->getPosition(), BattleHex(originHex)) << "the attacker did not fly back";
}
