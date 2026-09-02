local Script = {}
Script.__index = Script
Script.type = "combatAction"

--- Base class for scripts attached to units via the COMBAT_ACTION bonus. Each such bonus offers
--- its bearer one more action to pick from, alongside the ones the engine provides.
---
--- A script implements two methods:
---
--- `function Script:getSelectableHexes(battle, unit, hexes)` fills and returns `hexes` with the
--- tiles the owner may aim the action at, leaving it empty when the action is unavailable this
--- turn. The server validates an incoming action against it, so it must not look at anything the
--- client cannot see. `hexes` arrives empty and is the only way to build such a list.
---
--- `function Script:execute(server, battle, unit, targets)` carries the action out. `targets` holds
--- the hexes the owner aimed at, the first of which is the one the action was aimed at.
---
--- Parameters stored in the bonus are available as fields on `self` and are read-only, so a script
--- offering two variants of an action is attached twice with different parameters rather than
--- remembering which variant is active.

function Script:getSelectableHexes(battle, unit, hexes)
	return hexes
end

function Script:execute(server, battle, unit, targets)
end

return Script
