/*
 * ScriptedActionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/bonuses/BonusParameters.h"
#include "../../../lib/modding/ModScope.h"
#include "../../../lib/modding/IdentifierStorage.h"
#include "../../../lib/scripting/ScriptService.h"
#include "../../../lib/texts/CGeneralTextHandler.h"
#include "../../../lib/texts/MetaString.h"

/// Actions a unit offers because it carries a COMBAT_ACTION bonus, rather than because the engine
/// knows about them. The fixture script hits the unit it is aimed at together with everything
/// standing next to it, which no engine action does.
class ScriptedActionTest : public BattleTestFixture
{
public:
	static constexpr int32_t damagePerVictim = 500;

	/// Three defenders: two next to each other and one out of reach of both.
	static constexpr int attackerHex = 5 * GameConstants::BFIELD_WIDTH + 5;
	static constexpr int aimedHex = 5 * GameConstants::BFIELD_WIDTH + 10;
	static constexpr int adjacentHex = aimedHex + 1;
	static constexpr int distantHex = 8 * GameConstants::BFIELD_WIDTH + 10;

	/// Grants the bonus that offers the action, the way content declaring it would.
	static void giveScriptedAction(CStack * stack, const ScriptID & script, const JsonNode & parameters)
	{
		auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::COMBAT_ACTION, BonusSource::OTHER, 0, BonusSourceID(), BonusSubtypeID(script));
		bonus->parameters = std::make_shared<BonusParameters>(parameters);
		stack->addNewBonus(bonus);
	}
};

TEST_F(ScriptedActionTest, hitsAimedTargetAndItsNeighbour)
{
	startGame();
	startBattle();

	CStack * actor = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(attackerHex), 10);
	CStack * aimed = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(aimedHex), 1000);
	CStack * adjacent = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(adjacentHex), 1000);
	CStack * distant = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(distantHex), 1000);
	ASSERT_NE(actor, nullptr);

	JsonNode parameters;
	parameters["damage"].Integer() = damagePerVictim;

	const ScriptID script = scriptByName("vcmi-test:probeAction");
	giveScriptedAction(actor, script, parameters);

	beginCombat();

	const int64_t aimedBefore = aimed->getAvailableHealth();
	const int64_t adjacentBefore = adjacent->getAvailableHealth();
	const int64_t distantBefore = distant->getAvailableHealth();

	ASSERT_TRUE(useScriptedAction(actor, script, BattleHex(aimedHex)));

	EXPECT_EQ(aimedBefore - aimed->getAvailableHealth(), damagePerVictim);
	EXPECT_EQ(adjacentBefore - adjacent->getAvailableHealth(), damagePerVictim);
	EXPECT_EQ(distantBefore - distant->getAvailableHealth(), 0);
}

TEST_F(ScriptedActionTest, refusesActionTheUnitDoesNotHave)
{
	startGame();
	startBattle();

	CStack * actor = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(attackerHex), 10);
	CStack * aimed = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(aimedHex), 1000);
	ASSERT_NE(actor, nullptr);

	beginCombat();

	const int64_t aimedBefore = aimed->getAvailableHealth();

	EXPECT_FALSE(useScriptedAction(actor, scriptByName("vcmi-test:probeAction"), BattleHex(aimedHex)));
	EXPECT_EQ(aimed->getAvailableHealth(), aimedBefore);
}

TEST_F(ScriptedActionTest, refusesTargetTheScriptDoesNotOffer)
{
	startGame();
	startBattle();

	CStack * actor = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(attackerHex), 10);
	CStack * aimed = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(aimedHex), 1000);
	ASSERT_NE(actor, nullptr);

	JsonNode parameters;
	parameters["damage"].Integer() = damagePerVictim;

	const ScriptID script = scriptByName("vcmi-test:probeAction");
	giveScriptedAction(actor, script, parameters);

	beginCombat();

	const int64_t aimedBefore = aimed->getAvailableHealth();

	// an empty hex is not among the ones the script offers, so the server must not run it
	EXPECT_FALSE(useScriptedAction(actor, script, BattleHex(distantHex)));
	EXPECT_EQ(aimed->getAvailableHealth(), aimedBefore);
}

/// An action with nothing to aim at is aimed at its own bearer, which is a hex like any other - so
/// it needs no separate notion of a target-less action on the server.
TEST_F(ScriptedActionTest, battleWideActionIsAimedAtItsOwnBearer)
{
	startGame();
	startBattle();

	CStack * actor = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(attackerHex), 10);
	CStack * near = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(aimedHex), 1000);
	CStack * far = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(distantHex), 1000);
	ASSERT_NE(actor, nullptr);

	JsonNode parameters;
	parameters["damage"].Integer() = damagePerVictim;

	const ScriptID script = scriptByName("vcmi-test:battleWideAction");
	giveScriptedAction(actor, script, parameters);

	beginCombat();

	const int64_t nearBefore = near->getAvailableHealth();
	const int64_t farBefore = far->getAvailableHealth();
	const int64_t actorBefore = actor->getAvailableHealth();

	ASSERT_TRUE(useScriptedAction(actor, script, actor->getPosition()));

	EXPECT_EQ(nearBefore - near->getAvailableHealth(), damagePerVictim);
	EXPECT_EQ(farBefore - far->getAvailableHealth(), damagePerVictim);
	EXPECT_EQ(actor->getAvailableHealth(), actorBefore);
}

/// What the client asks a scripted action before the player commits to it: where it may be aimed,
/// what it would hit, which cursor to show and what to write in the status bar. All of it runs off
/// the battle callback alone, which is what lets the client answer without the server.
TEST_F(ScriptedActionTest, answersTheFeedbackTheClientNeeds)
{
	startGame();
	startBattle();

	CStack * actor = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(attackerHex), 10);
	CStack * aimed = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(aimedHex), 1000);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(adjacentHex), 1000);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(distantHex), 1000);
	ASSERT_NE(actor, nullptr);

	JsonNode parameters;
	parameters["damage"].Integer() = damagePerVictim;

	const ScriptID script = scriptByName("vcmi-test:probeAction");
	giveScriptedAction(actor, script, parameters);

	beginCombat();

	ScriptedActionInfo info = battle()->getScriptedAction(actor, script);
	ASSERT_NE(info.script, nullptr);

	const BattleHexArray targets({BattleHex(aimedHex)});

	// aimable at every enemy, including the one out of reach of the blast
	BattleHexArray selectable = info.script->getSelectableHexes(*battle(), actor, info.parameters);
	EXPECT_TRUE(selectable.contains(BattleHex(aimedHex)));
	EXPECT_TRUE(selectable.contains(BattleHex(distantHex)));
	EXPECT_FALSE(selectable.contains(BattleHex(attackerHex)));

	// but hitting only the aimed unit and the one beside it
	BattleHexArray affected = info.script->getAffectedHexes(*battle(), actor, targets, info.parameters);
	EXPECT_TRUE(affected.contains(BattleHex(aimedHex)));
	EXPECT_TRUE(affected.contains(BattleHex(adjacentHex)));
	EXPECT_FALSE(affected.contains(BattleHex(distantHex)));

	EXPECT_EQ(info.script->getCursor(*battle(), actor, targets, info.parameters), "combatHitNorth");

	// the message stays unresolved until something renders it, and carries the count as a number
	// rather than as text glued onto the end of a translated string
	MetaString message = info.script->getStatusMessage(*battle(), actor, targets, info.parameters);
	EXPECT_EQ(message.toString(LIBRARY->generaltexth.get()), "Strike 2 units");

	// the parameters the bonus carried reach the script, alongside the value of that bonus
	EXPECT_EQ(info.parameters["damage"].Integer(), damagePerVictim);

	(void)aimed;
}

/// A script that implements none of the feedback methods still works - the base class answers for it.
TEST_F(ScriptedActionTest, fallsBackToTheBaseClassForFeedback)
{
	startGame();
	startBattle();

	CStack * actor = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(attackerHex), 10);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(aimedHex), 1000);
	ASSERT_NE(actor, nullptr);

	JsonNode parameters;
	parameters["damage"].Integer() = damagePerVictim;

	const ScriptID script = scriptByName("vcmi-test:battleWideAction");
	giveScriptedAction(actor, script, parameters);

	beginCombat();

	ScriptedActionInfo info = battle()->getScriptedAction(actor, script);
	ASSERT_NE(info.script, nullptr);

	const BattleHexArray targets({actor->getPosition()});

	EXPECT_TRUE(info.script->getAffectedHexes(*battle(), actor, targets, info.parameters).contains(actor->getPosition()));
	EXPECT_EQ(info.script->getCursor(*battle(), actor, targets, info.parameters), "");

	EXPECT_EQ(info.script->getStatusMessage(*battle(), actor, targets, info.parameters).toString(LIBRARY->generaltexth.get()), "");
}

/// Walking and attacking are handed only to a combat action. A reaction to a combat event holds an
/// ordinary server callback, which does not carry them - so an attack cannot ask for another attack
/// from inside itself, which used to run until the stack gave out.
TEST_F(ScriptedActionTest, aReactionCanNotAskForAnAttack)
{
	startGame();
	startBattle();

	CStack * attacker = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(attackerHex), 100);
	CStack * defender = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(attackerHex + 1), 100);
	ASSERT_NE(attacker, nullptr);

	auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::COMBAT_EVENT_TRIGGER, BonusSource::OTHER, 0, BonusSourceID(), BonusSubtypeID(scriptByName("vcmi-test:recursionProbe")));
	attacker->addNewBonus(bonus);

	beginCombat();

	const int64_t before = defender->getAvailableHealth();

	ASSERT_TRUE(attack(attacker, defender->getPosition()));
	EXPECT_LT(defender->getAvailableHealth(), before) << "the attack itself must still happen";
}
