local Script = {}
Script.__index = Script
Script.type = "combatAction"

--- Base class for scripts attached to units via the COMBAT_ACTION bonus. Each such bonus offers
--- its bearer one more action to pick from, alongside the ones the engine provides.
---
--- Targeting and feedback, all of which run on the client as well as on the server, so none of them
--- may look at anything the owner of the unit cannot see:
---
--- `getSelectableHexes(battle, unit, hexes)` fills and returns `hexes` with the tiles the action may
--- be aimed at, leaving it empty when the action is unavailable this turn. The server validates an
--- incoming action against it. An action with nothing to aim at is aimed at its own bearer, so it
--- answers with the bearer's own position.
--- `validateTargets(battle, unit, targets)` says whether aiming the action this way is legal. The
--- first target is already checked against getSelectableHexes; anything past it comes from the
--- client unchecked, so the default answers false unless there is exactly one. An action wanting
--- more - a melee action letting the owner pick which side to approach from, say - overrides this
--- and checks them itself.
--- `getAffectedHexes(battle, unit, targets, hexes)` fills and returns `hexes` with the tiles the
--- action would affect, which is what the player sees shaded while hovering.
--- `getCursor(battle, unit, targets)` returns the name of a cursor declared in config/cursors.json,
--- or "" to keep the one the engine chose.
--- `getStatusMessage(battle, unit, targets)` returns the status bar line as a MetaString, so that
--- each client renders it in its own language.
---
--- `execute(server, actions, battle, unit, targets)` carries the action out, and runs on the server
--- only. `server` is what every kind of script has - damaging, healing, casting - while `actions`
--- carries walking and attacking, which only an action may ask for.
---
--- In all of them `targets` holds the hexes the owner aimed at, the first being the one the action
--- was aimed at. `hexes` arrives empty and is the only way to build such a list.
---
--- Parameters stored in the bonus are available as fields on `self` and are read-only, so a script
--- offering two variants of an action is attached twice with different parameters rather than
--- remembering which variant is active.

function Script:getSelectableHexes(battle, unit, hexes)
	return hexes
end

function Script:validateTargets(battle, unit, targets)
	return targets:size() == 1
end

function Script:getAffectedHexes(battle, unit, targets, hexes)
	return targets
end

function Script:getCursor(battle, unit, targets)
	return ""
end

function Script:getStatusMessage(battle, unit, targets)
	return {}
end

function Script:execute(server, actions, battle, unit, targets)
end

return Script
