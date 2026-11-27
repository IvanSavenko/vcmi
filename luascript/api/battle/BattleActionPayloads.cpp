/*
 * BattleActionPayloads.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleActionPayloads.h"

#include "../../../lib/battle/BattleAction.h"
#include "../../../lib/battle/CBattleInfoCallback.h"
#include "../../../lib/battle/DamageEstimationTexts.h"
#include "../../../lib/battle/actions/ActionOption.h"
#include "../../../lib/spells/CSpell.h"

namespace scripting::api
{

ActionOptionPayload::ActionOptionPayload(const ActionOption & option, const BonusList & bonuses)
	: spell(option.spell.hasValue() ? option.spell.toSpell() : nullptr)
	, bonuses(bonuses)
{
}

ActionContextPayload::ActionContextPayload(const ActionContext & context, bool legal)
	: actor(context.actor)
	, hoveredHex(context.hoveredHex)
	, partialTarget(context.partialTarget)
	, side(context.side)
	, legal(legal)
{
}

BattleActionPayload::BattleActionPayload(const CBattleInfoCallback & battle, const BattleAction & action, const BonusList & bonuses)
	: side(action.side)
	, actor(battle.battleGetUnitByID(action.stackNumber))
	, spell(action.spell.hasValue() ? action.spell.toSpell() : nullptr)
	, target(action.getTarget(&battle))
	, bonuses(bonuses)
{
}

ActionPreview ActionPreviewDescriptor::toPreview() const
{
	MetaString statusText = text.toMetaString();
	if(healthGain > 0)
	{
		statusText.appendRawString(" (");
		statusText.append(DamageEstimationTexts::healthGain(healthGain, healthGainTextID));
		statusText.appendRawString(")");
	}
	return {cursor, statusText, {}, {}};
}

}
