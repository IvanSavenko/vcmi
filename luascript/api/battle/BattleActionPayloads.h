/*
 * BattleActionPayloads.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/scripting/ApiTags.h>
#include <vcmi/spells/Spell.h>

#include "../LuaMetaString.h"
#include "../../../lib/battle/BattleHex.h"
#include "../../../lib/battle/BattleSide.h"
#include "../../../lib/battle/Destination.h"
#include "../../../lib/battle/Unit.h"

class BattleAction;
class CBattleInfoCallback;
struct ActionContext;
struct ActionOption;
struct ActionPreview;

namespace scripting::api
{

/// Option of a battle action type, as the script of the type receives it
struct ActionOptionPayload final : ApiSerializable<ActionOptionPayload>
{
	static constexpr std::string_view luaName = "ActionOption";
	static constexpr std::string_view luaDescription =
		"One way for a unit to use a battle action, offered to the player on the unit's turn. "
		"Passed to the functions of a battle action script.";

	const spells::Spell * spell = nullptr;

	explicit ActionOptionPayload(const ActionOption & option);
	ActionOptionPayload() = default;

	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("spell", spell, "Spell cast by the option, or nil.");
	}
};

/// Acting unit and hovered hex of a query about an option
struct ActionContextPayload final : ApiSerializable<ActionContextPayload>
{
	static constexpr std::string_view luaName = "ActionContext";
	static constexpr std::string_view luaDescription =
		"Acting unit and hovered hex of a query about an option of a battle action.";

	const battle::Unit * actor = nullptr;
	BattleHex hoveredHex;
	std::vector<battle::Destination> partialTarget;
	BattleSide side = BattleSide::NONE;
	bool legal = false;

	ActionContextPayload(const ActionContext & context, bool legal);
	ActionContextPayload() = default;

	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("actor", actor, "Unit that would make the action.");
		s("hoveredHex", hoveredHex, "Hex under the cursor.");
		s("partialTarget", partialTarget, "Destinations chosen with earlier clicks.");
		s("side", side, "Side that would make the action.");
		s("legal", legal, "True if clicking the hovered hex makes an action that passes `validate`. Set only for `preview`.");
	}
};

/// Battle action of a scripted type, with its target resolved against the battle
struct BattleActionPayload final : ApiSerializable<BattleActionPayload>
{
	static constexpr std::string_view luaName = "BattleAction";
	static constexpr std::string_view luaDescription =
		"Battle action made by a unit, as the script of its type receives it in `validate` and `apply`.";

	BattleSide side = BattleSide::NONE;
	const battle::Unit * actor = nullptr;
	const spells::Spell * spell = nullptr;
	std::vector<battle::Destination> target;

	BattleActionPayload(const CBattleInfoCallback & battle, const BattleAction & action);
	BattleActionPayload() = default;

	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("side", side, "Side making the action.");
		s("actor", actor, "Unit making the action, or nil if the battle has no such unit.");
		s("spell", spell, "Spell of the action, or nil.");
		s("target", target, "Destinations of the action.");
	}
};

/// What the client shows for an option on the hovered hex, as a battle action script returns it
struct ActionPreviewDescriptor final : ApiSerializable<ActionPreviewDescriptor>
{
	static constexpr std::string_view luaName = "ActionPreview";
	static constexpr std::string_view luaDescription =
		"Cursor and status bar text that the client shows for an option of a battle action on the hovered hex. "
		"Returned by the `preview` function of a battle action script.";

	std::string cursor;
	LuaMetaString text;
	int64_t healthGain = 0;
	std::string healthGainTextID;

	ActionPreview toPreview() const;

	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("cursor", cursor, "Identifier of the cursor from `config/cursors.json`.");
		s("text", text, "Status bar text.");
		s("healthGain", healthGain, "If above zero, appended to `text` in brackets with the plural forms of `healthGainTextID`.");
		s("healthGainTextID", healthGainTextID, "Base text ID of the health gain, such as `vcmi.battleWindow.healValuePreview`.");
	}
};

}
