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

class AttackSequenceTest : public BattleTestFixture
{
public:
	static constexpr int32_t attackerCount = 100;
	static constexpr int32_t defenderCount = 1000;

	/// Positions requiring movement before the melee attack
	static constexpr int originHex = leftHex;
	static constexpr int attackFromHex = leftHex + 3;
	static constexpr int targetHex = leftHex + 4;

	/// Across the field and several rows up, further than any of these creatures walks in one turn.
	static constexpr int farHex = GameConstants::BFIELD_WIDTH + 15;
};

TEST_F(AttackSequenceTest, anOrdinaryAttackerStaysWhereItStruck)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, creatureByName("core:griffin"), BattleHex(originHex), attackerCount);
	CStack * defender = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(targetHex), defenderCount);

	// Keep the attacker alive for whatever follows the blow.
	blockRetaliation(attacker);

	// Return movement requires initialized movement state.
	beginCombat();

	const int64_t healthBefore = defender->getAvailableHealth();

	ASSERT_TRUE(attackFrom(attacker, BattleHex(targetHex), BattleHex(attackFromHex)));
	ASSERT_LT(defender->getAvailableHealth(), healthBefore) << "attack dealt no damage";

	EXPECT_EQ(attacker->getPosition(), BattleHex(attackFromHex));
}

/// The harpy asks for its return through the action its ability grants it, which is the only thing
/// that makes an attack end anywhere but where it was struck from.
TEST_F(AttackSequenceTest, aHarpyFliesBackToWhereItStarted)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, creatureByName("core:harpy"), BattleHex(originHex), attackerCount);
	CStack * defender = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(targetHex), defenderCount);

	blockRetaliation(attacker);
	beginCombat();

	const int64_t healthBefore = defender->getAvailableHealth();

	ASSERT_TRUE(useScriptedAction(attacker, scriptByName("core:attackAndReturn"), BattleHexArray({BattleHex(targetHex), BattleHex(attackFromHex)})));

	ASSERT_LT(defender->getAvailableHealth(), healthBefore) << "attack dealt no damage";
	EXPECT_EQ(attacker->getPosition(), BattleHex(originHex));
}

/// The same walk-and-attack driven by a script instead of by the engine, which is what the harpy
/// ability becomes. It has to end in the same place, having dealt the same damage - the script owns
/// only the return, and asks the engine for the attack.
TEST_F(AttackSequenceTest, scriptedReturnEndsWhereTheEngineWouldHave)
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

/// A victim the unit cannot walk up to must not be offered at all. Offering one costs the owner the
/// turn: the action is accepted, the script finds nowhere to strike from, and nothing happens.
TEST_F(AttackSequenceTest, refusesAVictimItCanNotReach)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, creatureByName("core:harpy"), BattleHex(originHex), attackerCount);
	CStack * defender = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(farHex), defenderCount);
	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(defender, nullptr);

	beginCombat();

	const ScriptID script = scriptByName("core:attackAndReturn");
	ScriptedActionInfo info = battle()->getScriptedAction(attacker, script);
	ASSERT_NE(info.script, nullptr);

	EXPECT_FALSE(info.script->getSelectableHexes(*battle(), attacker, info.parameters).contains(BattleHex(farHex)))
		<< "a victim beyond the unit's reach was offered as a target";
	EXPECT_FALSE(useScriptedAction(attacker, script, BattleHex(farHex)));
	EXPECT_EQ(attacker->getPosition(), BattleHex(originHex));
}

/// The owner picks which side to approach from, so the action takes a second target. It has to be
/// checked: everything past the first target arrives from the client and nothing else looks at it.
TEST_F(AttackSequenceTest, approachHexIsHonouredAndChecked)
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
