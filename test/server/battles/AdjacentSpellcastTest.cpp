/*
 * AdjacentSpellcastTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../lib/battle/CBattleInfoCallback.h"
#include "../../../lib/bonuses/BonusMigration.h"
#include "../../../lib/bonuses/BonusParameters.h"
#include "../../../lib/bonuses/BonusSelector.h"
#include "../../../lib/combatScripts/ICombatActionScript.h"
#include "../../../lib/constants/Enumerations.h"

namespace
{

constexpr int32_t stackCount = 10;

/// Far enough apart that the caster has to walk to reach either.
constexpr int casterHex = 5 * GameConstants::BFIELD_WIDTH + 3;
constexpr int allyHex = 5 * GameConstants::BFIELD_WIDTH + 7;
constexpr int enemyHex = 5 * GameConstants::BFIELD_WIDTH + 12;

bool isEnchanted(const CStack * stack)
{
	return stack->hasBonus(Selector::sourceType()(BonusSource::SPELL_EFFECT));
}

}

/// Walking up to a unit and casting on it from there - what HotA engineers do, and what the
/// ADJACENT_SPELLCASTER bonus used to do. No core creature has the ability, so the tests grant it.
class AdjacentSpellcastTest : public BattleTestFixture
{
public:
	ScriptID script() const { return scriptByName("core:adjacentSpellcast"); }

	/// Grants the action, casting the named spell at the given mastery.
	void grant(CStack * caster, const std::string & spell, int mastery = 1)
	{
		JsonNode parameters;
		parameters["spell"].String() = spell;

		auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::COMBAT_ACTION, BonusSource::OTHER, mastery, BonusSourceID(), BonusSubtypeID(script()));
		bonus->parameters = std::make_shared<BonusParameters>(parameters);
		caster->addNewBonus(bonus);

		// the ability is a cast, so the unit needs something to spend on it
		caster->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::CASTS, BonusSource::OTHER, 3, BonusSourceID()));
	}

	BattleHexArray offeredFor(const CStack * caster)
	{
		ScriptedActionInfo info = battle()->getScriptedAction(caster, script());
		EXPECT_NE(info.script, nullptr);

		if(!info.script)
			return BattleHexArray();

		return info.script->getSelectableHexes(*battle(), caster, info.parameters);
	}
};

TEST_F(AdjacentSpellcastTest, WalksUpToTheAllyItIsAimedAtAndCastsThere)
{
	startGame();
	startBattle();

	CStack * caster = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(casterHex), stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, creatureByName("core:archer"), BattleHex(allyHex), stackCount);
	ASSERT_NE(caster, nullptr);
	ASSERT_NE(ally, nullptr);

	grant(caster, "core:bless");
	beginCombat();

	ASSERT_FALSE(isEnchanted(ally));
	ASSERT_TRUE(useScriptedAction(caster, script(), BattleHex(allyHex)));

	EXPECT_TRUE(isEnchanted(ally)) << "the spell never reached the ally";
	EXPECT_TRUE(BattleHex(allyHex).getNeighbouringTiles().contains(caster->getPosition())) << "the caster did not walk up to it";
}

/// The owner picks which side to walk up from, and that target comes from the client, so it has to be
/// checked like any other.
TEST_F(AdjacentSpellcastTest, ApproachHexIsHonouredAndChecked)
{
	startGame();
	startBattle();

	CStack * caster = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(casterHex), stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, creatureByName("core:archer"), BattleHex(allyHex), stackCount);
	ASSERT_NE(caster, nullptr);
	ASSERT_NE(ally, nullptr);

	grant(caster, "core:bless");
	beginCombat();

	// a hex nowhere near the ally is not a side of it
	EXPECT_FALSE(useScriptedAction(caster, script(), BattleHexArray({BattleHex(allyHex), BattleHex(casterHex)})));
	EXPECT_FALSE(isEnchanted(ally)) << "a refused action must not cast";

	const BattleHex approach(allyHex - 1);
	ASSERT_TRUE(useScriptedAction(caster, script(), BattleHexArray({BattleHex(allyHex), approach})));

	EXPECT_TRUE(isEnchanted(ally));
	EXPECT_EQ(caster->getPosition(), approach) << "the caster ignored the side it was sent to";
}

/// The spell decides where the action may be aimed, which is what keeps a repair off a healthy
/// machine and a blessing off an enemy.
TEST_F(AdjacentSpellcastTest, OffersOnlyWhereTheSpellWouldDoSomething)
{
	startGame();
	startBattle();

	CStack * caster = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(casterHex), stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, creatureByName("core:archer"), BattleHex(allyHex), stackCount);
	CStack * enemy = addStack(BattleSide::DEFENDER, creatureByName("core:skeleton"), BattleHex(enemyHex), stackCount);
	ASSERT_NE(ally, nullptr);
	ASSERT_NE(enemy, nullptr);

	grant(caster, "core:cure");
	beginCombat();

	// cure heals, and neither of them is hurt, so there is nothing to aim at yet
	EXPECT_FALSE(offeredFor(caster).contains(BattleHex(allyHex)));
	EXPECT_FALSE(useScriptedAction(caster, script(), BattleHex(allyHex)));

	int64_t wound = ally->getMaxHealth();
	ally->damage(wound);

	EXPECT_TRUE(offeredFor(caster).contains(BattleHex(allyHex)));
	// an undead enemy is out of reach of a positive spell whatever its state
	EXPECT_FALSE(offeredFor(caster).contains(BattleHex(enemyHex)));
}

/// Spending the casts the ability draws on takes the action away, which is how CASTS limits it.
TEST_F(AdjacentSpellcastTest, DisappearsOnceTheCastsAreSpent)
{
	startGame();
	startBattle();

	CStack * caster = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(casterHex), stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, creatureByName("core:archer"), BattleHex(allyHex), stackCount);
	ASSERT_NE(caster, nullptr);
	ASSERT_NE(ally, nullptr);

	grant(caster, "core:bless");
	caster->removeBonusesRecursive(Selector::type()(BonusType::CASTS));
	beginCombat();

	EXPECT_TRUE(offeredFor(caster).empty()) << "a unit with nothing to cast with offers no cast";
	EXPECT_FALSE(useScriptedAction(caster, script(), BattleHex(allyHex)));
}

/// The cast is the unit's action, so it is announced as a spellcast of that unit - what an ability
/// reacting to its bearer casting listens for. Casting through a script must not lose that.
TEST_F(AdjacentSpellcastTest, IsAnnouncedAsASpellcastOfTheCaster)
{
	startGame();
	startBattle();

	CStack * caster = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(casterHex), stackCount);
	CStack * ally = addStack(BattleSide::ATTACKER, creatureByName("core:archer"), BattleHex(allyHex), stackCount);
	ASSERT_NE(caster, nullptr);
	ASSERT_NE(ally, nullptr);

	grant(caster, "core:bless");

	// the reaction grants luck, which nothing else in the scenario does
	BonusParametersOnCombatEvent::CombatEffectBonus effect;
	effect.bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::LUCK, BonusSource::OTHER, 1, BonusSourceID());
	effect.targetEnemy = false;

	BonusParametersOnCombatEvent reaction;
	reaction.effects.emplace_back(effect);

	auto trigger = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::ON_COMBAT_EVENT, BonusSource::OTHER, 0, BonusSourceID(), BonusSubtypeID(BonusCustomSubtype(static_cast<int>(CombatEventType::UNIT_SPELLCAST))));
	trigger->parameters = std::make_shared<BonusParameters>(reaction);
	caster->addNewBonus(trigger);

	beginCombat();

	ASSERT_TRUE(useScriptedAction(caster, script(), BattleHex(allyHex)));

	EXPECT_EQ(caster->valOfBonuses(BonusType::LUCK), 1) << "the cast was never announced as a spellcast";
}

TEST_F(AdjacentSpellcastTest, ConvertsTheRetiredBonusModsStillDeclare)
{
	startGame();

	JsonNode ability;
	ability["type"].String() = "ADJACENT_SPELLCASTER";
	ability["subtype"].String() = "core:bless";
	ability["val"].Integer() = 2;

	JsonNode migrated;
	ASSERT_TRUE(BonusMigration::migrateBonus(ability, migrated));

	EXPECT_EQ(migrated["type"].String(), "COMBAT_ACTION");
	EXPECT_EQ(migrated["subtype"].String(), "adjacentSpellcast");
	EXPECT_EQ(migrated["addInfo"]["spell"].String(), "core:bless") << "the spell has to move into a parameter";
	EXPECT_EQ(migrated["val"].Integer(), 2) << "val still means the mastery level";
}
