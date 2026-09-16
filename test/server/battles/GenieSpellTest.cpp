/*
 * GenieSpellTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "AI/BattleAI/StackWithBonuses.h"

#include "../../../lib/battle/CBattleInfoCallback.h"
#include "../../../lib/bonuses/BonusSelector.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/bonuses/BonusMigration.h"
#include "../../../lib/bonuses/BonusParameters.h"
#include "../../../lib/texts/CGeneralTextHandler.h"
#include "../../../lib/texts/MetaString.h"
#include "../../../lib/scripting/ScriptService.h"
#include "../../../lib/combatScripts/ICombatActionScript.h"
#include "../../../server/CGameHandler.h"

namespace
{

constexpr int32_t stackCount = 10;

/// Free hexes far enough apart that nothing the genie does depends on who stands where.
constexpr int genieHex = 5 * GameConstants::BFIELD_WIDTH + 5;
constexpr int allyHex = 5 * GameConstants::BFIELD_WIDTH + 7;
constexpr int enemyHex = 5 * GameConstants::BFIELD_WIDTH + 12;
constexpr int secondAllyHex = 3 * GameConstants::BFIELD_WIDTH + 7;

/// Whether the unit carries anything a spell put on it, which is the whole of what the genie does
/// regardless of which spell it rolled.
bool isEnchanted(const CStack * stack)
{
	return stack->hasBonus(Selector::sourceType()(BonusSource::SPELL_EFFECT));
}

}

/// The master genie casts a beneficial spell, rolled when the action is carried out, on another
/// allied unit. It is the first ability shipped as a combat action script, so these also check
/// that a real creature picks the action up from its own configuration.
class GenieSpellTest : public BattleTestFixture
{
public:
	ScriptID script() const { return scriptByName("core:genieSpell"); }

	/// Grants a genie whose repertoire is only the named spell, and answers where it may aim it.
	/// The mastery matches the master genie's, since some conditions depend on it.
	BattleHexArray offeredFor(CStack * genie, const std::string & spell)
	{
		const ScriptID probe = scriptByName("vcmi-test:genieSingleSpell");

		JsonNode parameters;
		parameters["only"].String() = spell;

		// one probe at a time, or the second call would read the parameters of the first
		genie->removeBonusesRecursive(Selector::typeSubtype(BonusType::COMBAT_ACTION, BonusSubtypeID(probe)));

		auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::COMBAT_ACTION, BonusSource::OTHER, 2, BonusSourceID(), BonusSubtypeID(probe));
		bonus->parameters = std::make_shared<BonusParameters>(parameters);
		genie->addNewBonus(bonus);

		ScriptedActionInfo info = battle()->getScriptedAction(genie, probe);
		EXPECT_NE(info.script, nullptr);

		if(!info.script)
			return BattleHexArray();

		return info.script->getSelectableHexes(*battle(), genie, info.parameters);
	}
};

TEST_F(GenieSpellTest, EnchantsAnAllyItIsAimedAt)
{
	startGame();
	startBattle();

	CStack * genie = addStack(BattleSide::ATTACKER, creatureByName("core:masterGenie"), BattleHex(genieHex), stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(allyHex), stackCount);
	ASSERT_NE(genie, nullptr);
	ASSERT_NE(ally, nullptr);
	ASSERT_FALSE(isEnchanted(ally));

	beginCombat();

	ASSERT_TRUE(useScriptedAction(genie, script(), BattleHex(allyHex)));

	EXPECT_TRUE(isEnchanted(ally));
}

TEST_F(GenieSpellTest, OffersAlliesOnlyAndNotItself)
{
	startGame();
	startBattle();

	CStack * genie = addStack(BattleSide::ATTACKER, creatureByName("core:masterGenie"), BattleHex(genieHex), stackCount);
	addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(allyHex), stackCount);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(enemyHex), stackCount);
	ASSERT_NE(genie, nullptr);

	beginCombat();

	// the creature carries the action because of its own configuration, not because the test
	// granted it - which is what makes the config, the migration and the script one working chain
	ScriptedActionInfo info = battle()->getScriptedAction(genie, script());
	ASSERT_NE(info.script, nullptr);

	BattleHexArray selectable = info.script->getSelectableHexes(*battle(), genie, info.parameters);
	EXPECT_TRUE(selectable.contains(BattleHex(allyHex)));
	EXPECT_FALSE(selectable.contains(BattleHex(enemyHex)));
	EXPECT_FALSE(selectable.contains(BattleHex(genieHex)));

	// the status bar says what it would do, and why it will not, rather than going blank
	MetaString onAlly = info.script->getStatusMessage(*battle(), genie, BattleHexArray({BattleHex(allyHex)}), info.parameters);
	MetaString onEnemy = info.script->getStatusMessage(*battle(), genie, BattleHexArray({BattleHex(enemyHex)}), info.parameters);
	EXPECT_EQ(onAlly.toString(LIBRARY->generaltexth.get()), "Cast a spell on Pikemen");
	EXPECT_EQ(onEnemy.toString(LIBRARY->generaltexth.get()), "Select Spell Target");
}

TEST_F(GenieSpellTest, RefusesAnEnemyAsTarget)
{
	startGame();
	startBattle();

	CStack * genie = addStack(BattleSide::ATTACKER, creatureByName("core:masterGenie"), BattleHex(genieHex), stackCount);
	CStack * enemy = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(enemyHex), stackCount);
	ASSERT_NE(genie, nullptr);
	ASSERT_NE(enemy, nullptr);

	beginCombat();

	EXPECT_FALSE(useScriptedAction(genie, script(), BattleHex(enemyHex)));
	EXPECT_FALSE(isEnchanted(enemy));
}

TEST_F(GenieSpellTest, SimulatesOnTheAiCopyOfTheBattleRatherThanOnTheRealOne)
{
	startGame();
	startBattle();

	CStack * genie = addStack(BattleSide::ATTACKER, creatureByName("core:masterGenie"), BattleHex(genieHex), stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(allyHex), stackCount);
	ASSERT_NE(genie, nullptr);
	ASSERT_NE(ally, nullptr);

	beginCombat();

	auto subject = std::shared_ptr<CBattleInfoCallback>(battle(), [](CBattleInfoCallback *){});
	HypotheticBattle state(gameHandler.get(), subject);

	ScriptedActionInfo info = state.getScriptedAction(state.battleGetUnitByID(genie->unitId()), script());
	ASSERT_NE(info.script, nullptr);

	info.script->execute(state.getServerCallback(), state.getCombatActionCallback(), state, state.battleGetUnitByID(genie->unitId()), BattleHexArray({BattleHex(allyHex)}), info.parameters);

	// the AI values an action by running it and looking at what changed, so the cast has to reach
	// the simulated ally and no further
	EXPECT_TRUE(state.battleGetUnitByID(ally->unitId())->hasBonus(Selector::sourceType()(BonusSource::SPELL_EFFECT)));
	EXPECT_FALSE(isEnchanted(ally));
}

TEST_F(GenieSpellTest, ConvertsTheRetiredBonusModsStillDeclare)
{
	startGame();

	JsonNode ability;
	ability["type"].String() = "RANDOM_SPELLCASTER";
	ability["val"].Integer() = 3;

	JsonNode migrated;
	ASSERT_TRUE(BonusMigration::migrateBonus(ability, migrated));

	// COMBAT_ACTION rather than the COMBAT_EVENT_TRIGGER every earlier retirement became, which is
	// the whole reason the conversion table names the bonus type instead of assuming it
	EXPECT_EQ(migrated["type"].String(), "COMBAT_ACTION");
	EXPECT_EQ(migrated["subtype"].String(), "genieSpell");
	EXPECT_EQ(migrated["val"].Integer(), 3);
}

TEST_F(GenieSpellTest, TakesItsButtonIconFromTheModThatShipsIt)
{
	startGame();

	// the script is part of the base install, but VCMI's own artwork is not - so the icon arrives
	// as a patch from the mod holding the image, and the button is blank if that stops working
	EXPECT_EQ(LIBRARY->scriptTypes()->getById(script()).icon.getOriginalName(), "battle/actionGenie");
}

TEST_F(GenieSpellTest, PatchesDecideWhichSpellsAreWorthCasting)
{
	startGame();
	startBattle();

	CStack * genie = addStack(BattleSide::ATTACKER, creatureByName("core:masterGenie"), BattleHex(genieHex), stackCount);
	CStack * shooter = addStack(BattleSide::ATTACKER, creatureByName("core:archer"), BattleHex(allyHex), stackCount);
	CStack * fighter = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(secondAllyHex), stackCount);
	ASSERT_NE(genie, nullptr);
	ASSERT_NE(shooter, nullptr);
	ASSERT_NE(fighter, nullptr);

	// the fixture patches the shipped script down to precision alone, which is worth casting only on
	// a subject that shoots - so what the two allies get offered is what the condition decided
	const ScriptID probe = scriptByName("vcmi-test:geniePrecisionOnly");
	auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::COMBAT_ACTION, BonusSource::OTHER, 2, BonusSourceID(), BonusSubtypeID(probe));
	genie->addNewBonus(bonus);

	beginCombat();

	ScriptedActionInfo info = battle()->getScriptedAction(genie, probe);
	ASSERT_NE(info.script, nullptr);

	BattleHexArray selectable = info.script->getSelectableHexes(*battle(), genie, info.parameters);
	EXPECT_TRUE(selectable.contains(BattleHex(allyHex)));
	EXPECT_FALSE(selectable.contains(BattleHex(secondAllyHex)));

	ASSERT_TRUE(useScriptedAction(genie, probe, BattleHex(allyHex)));
	EXPECT_TRUE(shooter->hasBonus(Selector::source(BonusSource::SPELL_EFFECT, BonusSourceID(SpellID(SpellID::PRECISION)))));

	// and now there is nothing left to gain there, so the only spell it has is not offered twice
	EXPECT_FALSE(info.script->getSelectableHexes(*battle(), genie, info.parameters).contains(BattleHex(allyHex)));
}

TEST_F(GenieSpellTest, OffersWardingSpellsOnlyAgainstAHeroCarryingASpellbook)
{
	startGame();
	startBattle();

	CStack * genie = addStack(BattleSide::ATTACKER, creatureByName("core:masterGenie"), BattleHex(genieHex), stackCount);
	addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(allyHex), stackCount);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(enemyHex), stackCount);
	ASSERT_NE(genie, nullptr);

	beginCombat();

	EXPECT_FALSE(offeredFor(genie, "core:antiMagic").contains(BattleHex(allyHex)));

	giveArtifact(defenderSideHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);

	EXPECT_TRUE(offeredFor(genie, "core:antiMagic").contains(BattleHex(allyHex)));
}

TEST_F(GenieSpellTest, OffersPrayerOnlyWhileTheAllyStillHasItsTurn)
{
	startGame();
	startBattle();

	CStack * genie = addStack(BattleSide::ATTACKER, creatureByName("core:masterGenie"), BattleHex(genieHex), stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(allyHex), stackCount);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(enemyHex), stackCount);
	ASSERT_NE(genie, nullptr);
	ASSERT_NE(ally, nullptr);

	beginCombat();

	EXPECT_TRUE(offeredFor(genie, "core:prayer").contains(BattleHex(allyHex)));

	ASSERT_TRUE(defend(ally));

	EXPECT_FALSE(offeredFor(genie, "core:prayer").contains(BattleHex(allyHex)));
}

TEST_F(GenieSpellTest, CountsOnlyEnemiesThatCouldActuallyStrikeInMelee)
{
	startGame();
	startBattle();

	CStack * genie = addStack(BattleSide::ATTACKER, creatureByName("core:masterGenie"), BattleHex(genieHex), stackCount);
	addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(allyHex), stackCount);
	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(enemyHex), stackCount);
	ASSERT_NE(genie, nullptr);

	beginCombat();

	EXPECT_TRUE(offeredFor(genie, "core:shield").contains(BattleHex(allyHex)));

	// blinded, so nothing on that side is a melee threat and the shield guards against nothing.
	// The hero garrisons the battle is built with are enemies too, so every one of them counts
	for(const auto & stack : battle()->stacks)
		if(stack->unitSide() == BattleSide::DEFENDER)
			stack->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::NOT_ACTIVE, BonusSource::OTHER, 0, BonusSourceID()));

	EXPECT_FALSE(offeredFor(genie, "core:shield").contains(BattleHex(allyHex)));
}

TEST_F(GenieSpellTest, TreatsABlockedShooterAsAMeleeFighterOnBothSides)
{
	startGame();
	startBattle();

	CStack * genie = addStack(BattleSide::ATTACKER, creatureByName("core:masterGenie"), BattleHex(genieHex), stackCount);
	CStack * archer = addStack(BattleSide::ATTACKER, creatureByName("core:archer"), BattleHex(allyHex), stackCount);
	ASSERT_NE(genie, nullptr);
	ASSERT_NE(archer, nullptr);

	beginCombat();

	// free to shoot: worth precision, and bloodlust would do nothing for it
	EXPECT_TRUE(offeredFor(genie, "core:precision").contains(BattleHex(allyHex)));
	EXPECT_FALSE(offeredFor(genie, "core:bloodlust").contains(BattleHex(allyHex)));

	addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(allyHex + 1), stackCount);

	// hemmed in, so it is down to its fists and the two answers swap over
	EXPECT_FALSE(offeredFor(genie, "core:precision").contains(BattleHex(allyHex)));
	EXPECT_TRUE(offeredFor(genie, "core:bloodlust").contains(BattleHex(allyHex)));
}

TEST_F(GenieSpellTest, OffersSlayerOnlyForAnEnemyItsMasteryWouldReach)
{
	startGame();
	startBattle();

	CStack * genie = addStack(BattleSide::ATTACKER, creatureByName("core:masterGenie"), BattleHex(genieHex), stackCount);
	addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(allyHex), stackCount);
	CStack * enemy = addStack(BattleSide::DEFENDER, creatureByName("core:pikeman"), BattleHex(enemyHex), stackCount);
	ASSERT_NE(genie, nullptr);
	ASSERT_NE(enemy, nullptr);

	beginCombat();

	EXPECT_FALSE(offeredFor(genie, "core:slayer").contains(BattleHex(allyHex)));

	// a creature only expert slayer reaches is no reason for an advanced one to be cast
	enemy->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::KING, BonusSource::OTHER, 3, BonusSourceID()));
	EXPECT_FALSE(offeredFor(genie, "core:slayer").contains(BattleHex(allyHex)));

	enemy->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::KING, BonusSource::OTHER, 2, BonusSourceID()));
	EXPECT_TRUE(offeredFor(genie, "core:slayer").contains(BattleHex(allyHex)));
}
