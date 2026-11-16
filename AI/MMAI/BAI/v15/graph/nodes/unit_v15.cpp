/*
 * unit.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "BAI/v15/graph/nodes/unit_v15.h"

#include "BAI/v15/graph/util_v15.h"

#include "AI/MMAI/common.h"
#include "GameLibrary.h"
#include "battle/CombatValue.h"
#include "bonuses/BonusEnum.h"
#include "constants/EntityIdentifiers.h"
#include "modding/IdentifierStorage.h"
#include "modding/ModScope.h"
#include "schema/v15/constants.h"

// XXX: clangd warns these are unused, but they are in fact required
#include "bonuses/BonusParameters.h"
#include "spells/CSpellHandler.h"

namespace MMAI::BAI::V15::Graph::Nodes
{

namespace
{
	std::string CalculateAlias(const CStack & cstack)
	{
		int slot = cstack.unitSlot();

		if(slot == SlotID::WAR_MACHINES_SLOT)
			slot = S15::STACK_SLOT_WARMACHINES;
		else if(slot < 0 || slot >= 7)
			slot = S15::STACK_SLOT_SPECIAL;

		switch(slot)
		{
			case S15::STACK_SLOT_SPECIAL:
				return "S";
			case S15::STACK_SLOT_WARMACHINES:
				return "M";
			default:
				return std::to_string(slot);
		}
	}
}

// static
ScriptID Unit::CombatScriptID(const std::string & name)
{
	auto index = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "script", name, false);
	return index.has_value() ? ScriptID(*index) : ScriptID::NONE;
}

// static
bool Unit::RunsCombatScript(const Bonus & bonus, const std::string & script)
{
	if(bonus.type != BonusType::COMBAT_EVENT_TRIGGER)
		return false;

	auto scriptID = CombatScriptID(script);

	return scriptID != ScriptID::NONE && bonus.subtype.as<ScriptID>() == scriptID;
}

// static
bool Unit::HasCombatScript(const CStack * cstack, const std::string & script)
{
	auto scriptID = CombatScriptID(script);

	if(scriptID == ScriptID::NONE)
		return false;

	return cstack->hasBonusOfType(BonusType::COMBAT_EVENT_TRIGGER, BonusSubtypeID(scriptID));
}

// static
int Unit::GetValue(const CCreature * creature, bool isClone, bool isSummon)
{
	if(!creature)
		throw std::runtime_error("GetValue: nullptr given");

	auto v = static_cast<int>(LIBRARY->creh->getCombatValue().getAIValue(creature));

	if(isClone)
		v *= 5;

	if(isSummon)
		v /= 5;

	return v;
}

Unit::Unit(const Args & args)
	: cstack(args.cstack)
	, alias(CalculateAlias(args.cstack))
	, distances(args.distances)
	, isActive(args.isActive)
	, isFlying(args.isFlying) // cache to prevent repeated bonus checks
	, speed(args.speed) // cache to prevent repeated bonus checks
	, valueOne(GetValue(cstack.unitType(), cstack.isClone(), cstack.unitSlot() == SlotID::SUMMONED_SLOT_PLACEHOLDER))
{
	guardflags.set(); // See note for attr()/setattr() in Unit.h

	// Warn against potential overflows by e.g. 5000 azure dragons (vcmiazure cheat)
	// Unlikely to ever trigger, currently 1 azureDragon is valued at ~41K,
	// and the enforced limit is ~430K, more than 10 times higher.
	int64_t v64 = static_cast<int64_t>(valueOne) * cstack.getCount();
	if(v64 > std::numeric_limits<int>().max())
	{
		int max = std::numeric_limits<int>().max();
		logAi->warn("Stack %s AI value=%ld will be capped at %d", cstack.getDescription(), v64, max);
		v64 = max;
	}

	auto value = static_cast<int>(v64);

	// damage modifiers that care if attack is ranged (e.g. archery skill)
	// would affect max and min proportionally which would have no effect
	// on uncertainty calculations
	auto dmgrange = cstack.getMaxDamage(false) - cstack.getMinDamage(false);
	auto k = std::min(cstack.getCount(), 10); // see BattleInfo::getActualDamage()
	auto dmgstd = std::sqrt((dmgrange * dmgrange) / (12.0 * k));
	auto dmgmean = (cstack.getMaxDamage(false) + cstack.getMinDamage(false)) / 2.0;
	auto dmgstdnorm = dmgstd / dmgmean; // relative uncertainty for the dmg dealt

	setattr(UA::VALUE_REL, permille(value, args.bfieldValue));
	setattr(UA::SHOTS, cstack.shots.available());
	setattr(UA::DMG_UNCERTAINTY, permille(dmgstdnorm, 1));
	setattr(UA::IS_ACTIVE, isActive);
	setattr(UA::IS_ENEMY, args.isEnemy);

	auto bonuses = cstack.getAllBonuses(Selector::all);

	for(const auto & bonus : *bonuses)
	{
		switch(bonus->type)
		{
			case BonusType::FLYING:
				setattr(UA::HAS_FLYING, 1);
				break;
			case BonusType::UNDEAD:
			case BonusType::NON_LIVING:
				setattr(UA::HAS_NON_LIVING, 1);
				break;
			case BonusType::SIEGE_WEAPON:
				setattr(UA::IS_WAR_MACHINE, 1);
				break;
			case BonusType::BLOCKS_RETALIATION:
				setattr(UA::HAS_BLOCKS_RETALIATION, 1);
				break;
			case BonusType::NO_MELEE_PENALTY:
				setattr(UA::HAS_NO_MELEE_PENALTY, 1);
				break;
			case BonusType::TWO_HEX_ATTACK_BREATH:
				setattr(UA::HAS_TWO_HEX_ATTACK_BREATH, 1);
				break;
			case BonusType::ADDITIONAL_ATTACK:
				setattr(UA::HAS_ADDITIONAL_ATTACK, 1);
				break;
			case BonusType::SPELL_AFTER_ATTACK:
				switch(bonus->subtype.as<SpellID>())
				{
					case SpellID::BLIND:
					case SpellID::PARALYZE:
						setattr(UA::HAS_BLIND_ATTACK, permille(bonus->val, 100));
						break;
					case SpellID::STONE_GAZE:
						setattr(UA::HAS_PETRIFY_ATTACK, permille(bonus->val, 100));
						break;
					case SpellID::BIND:
						setattr(UA::HAS_BIND_ATTACK, permille(bonus->val, 100));
						break;
					case SpellID::WEAKNESS:
						setattr(UA::HAS_WEAKNESS_ATTACK, permille(bonus->val, 100));
						break;
					case SpellID::DISPEL:
					case SpellID::DISPEL_HELPFUL_SPELLS:
						setattr(UA::HAS_DISPEL_ATTACK, permille(bonus->val, 100));
						break;
					case SpellID::POISON:
						setattr(UA::HAS_POISON_ATTACK, permille(bonus->val, 100));
						break;
					case SpellID::CURSE:
						setattr(UA::HAS_CURSE_ATTACK, permille(bonus->val, 100));
						break;
					case SpellID::AGE:
						setattr(UA::HAS_AGE_ATTACK, permille(bonus->val, 100));
						break;
					default:
						break;
				}
				break;
			case BonusType::SPELL_LIKE_ATTACK:
				switch(bonus->subtype.as<SpellID>())
				{
					case SpellID::FIREBALL:
						setattr(UA::HAS_FIREBALL, 1);
						break;
					case SpellID::DEATH_CLOUD:
						setattr(UA::HAS_DEATH_CLOUD, 1);
						break;
					default:
						break;
				}
				break;
			case BonusType::THREE_HEADED_ATTACK:
				setattr(UA::HAS_THREE_HEADED_ATTACK, 1);
				break;
			case BonusType::ATTACKS_ALL_ADJACENT:
				setattr(UA::HAS_ALL_AROUND_ATTACK, 1);
				break;
			case BonusType::RETURN_AFTER_STRIKE:
				setattr(UA::HAS_RETURN_AFTER_STRIKE, 1);
				break;
			case BonusType::COMBAT_EVENT_TRIGGER:
				if(Unit::RunsCombatScript(*bonus, "lifeDrain"))
					setattr(UA::HAS_LIFE_DRAIN, permille(bonus->val, 100));
				break;
			case BonusType::DOUBLE_DAMAGE_CHANCE:
				setattr(UA::HAS_DOUBLE_DAMAGE_CHANCE, permille(bonus->val, 100));
				break;
			case BonusType::FEARFUL:
				// Can't figure out how to properly check if this stack is inducing fear
				// In VCMI the FEARFUL bonus of azure dragons is applied ALL stacks
				// incl. the azure dragons have a second FEARFUL bonus that overrides
				// the value to 0 (i.e. "fearless" is FEARFUL with independentMin=0...)
				// This means it's not really possible to identify the source stack.
				// One workaround is to see check:
				//     propagator == BATTLE_WIDE
				//     && source == CREATURE_ABILITY
				//     && sid.as<CreatureID> == cstack.creatureID
				break;
			case BonusType::NOT_ACTIVE:
				if(cstack.unitType()->getId() != CreatureID::AMMO_CART)
					setattr(UA::IS_SLEEPING, 1);
				break;
			default:
				break;
		}

		if(bonus->source == BonusSource::SPELL_EFFECT)
		{
			switch(bonus->sid.as<SpellWithMasteryID>().getSpellID())
			{
				case SpellID::AGE:
					setattr(UA::HAS_AGE, bonus->turnsRemain);
					break;
				case SpellID::BIND:
					setattr(UA::HAS_BIND, bonus->turnsRemain);
					break;
				case SpellID::BLIND:
				case SpellID::PARALYZE:
					setattr(UA::HAS_BLIND, bonus->turnsRemain);
					break;
				case SpellID::CURSE:
					setattr(UA::HAS_CURSE, bonus->turnsRemain);
					break;
				case SpellID::POISON:
					setattr(UA::HAS_POISON, bonus->turnsRemain);
					break;
				case SpellID::STONE_GAZE:
					setattr(UA::HAS_PETRIFY, bonus->turnsRemain);
					break;
				case SpellID::WEAKNESS:
					setattr(UA::HAS_WEAKNESS, bonus->turnsRemain);
					break;
				default:
					break;
			}
		}
	}

	static_assert(EU(UA::_count) == 35, "whistleblower in case attributes change");
}

int Unit::attr(Attribute a) const
{
	return attrs.at(EU(a));
}

void Unit::setattr(Attribute a, int value)
{
	attrs.at(EU(a)) = value;
}

}
