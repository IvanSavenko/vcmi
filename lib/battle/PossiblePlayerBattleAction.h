/*
 * PossiblePlayerBattleAction.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../GameConstants.h"

class PossiblePlayerBattleAction // actions performed at l-click
{
public:
	enum Actions {
		INVALID = -1,
		CREATURE_INFO,
		HERO_INFO,
		MOVE_TACTICS,
		CHOOSE_TACTICS_STACK,

		MOVE_STACK,
		ATTACK,
		LONG_WEAPON_ATTACK,
		WALK_AND_ATTACK,
		SHOOT,
		CATAPULT,
		HEAL,
		WALK_AND_SPELLCAST,


		NO_LOCATION,          // massive spells that affect every possible target, automatic casts
		ANY_LOCATION,
		OBSTACLE,
		TELEPORT,
		SACRIFICE,
		FREE_LOCATION,        // used with Force Field and Fire Wall - all tiles affected by spell must be free
		AIMED_SPELL_CREATURE, // spell targeted at creature

		SCRIPTED_ACTION,      // action defined by a script, named by actionScript
	};

private:
	Actions action;
	SpellID spellToCast;
	ScriptID actionScript;

public:
	bool spellcast() const
	{
		return action == ANY_LOCATION || action == NO_LOCATION || action == OBSTACLE || action == TELEPORT ||
			   action == SACRIFICE || action == FREE_LOCATION || action == AIMED_SPELL_CREATURE || action == WALK_AND_SPELLCAST;
	}

	Actions get() const
	{
		return action;
	}

	SpellID spell() const
	{
		return spellToCast;
	}

	ScriptID script() const
	{
		return actionScript;
	}

	PossiblePlayerBattleAction(Actions action, SpellID spellToCast = SpellID::NONE):
		action(action),
		spellToCast(spellToCast)
	{
		assert((spellToCast != SpellID::NONE) == spellcast());
		assert(action != SCRIPTED_ACTION);
	}

	/// A unit offers one of these per COMBAT_ACTION bonus it carries, so the script is what tells
	/// two of them apart - everything else about them is identical
	explicit PossiblePlayerBattleAction(ScriptID actionScript):
		action(SCRIPTED_ACTION),
		spellToCast(SpellID::NONE),
		actionScript(actionScript)
	{
		assert(actionScript.hasValue());
	}

	bool operator == (const PossiblePlayerBattleAction & other) const
	{
		return action == other.action && spellToCast == other.spellToCast && actionScript == other.actionScript;
	}

	bool operator != (const PossiblePlayerBattleAction & other) const
	{
		return !(*this == other);
	}
};
