local Base = require("combat/combatAction")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Walks up to an enemy, strikes it, and flies back to where it started - what harpies do.
---
--- The attack itself is the engine's, so this script owns none of the rules about first strike,
--- multiple blows or retaliation. What it owns is the return, and the one subtlety in it: a unit
--- slowed while attacking does not make it all the way home, so the flight back is shortened by
--- however much movement it lost.

function Script:getSelectableHexes(battle, unit, hexes)
	for _, other in ipairs(battle:getUnitsIf(function(candidate)
		return candidate:isAlive() and candidate:isValidTarget(false) and candidate:getSide() ~= unit:getSide()
	end)) do
		local occupied = other:getHexes()
		for i = 1, occupied:size() do
			hexes:insert(occupied:at(i))
		end
	end

	return hexes
end

--- The owner may pick which side to approach from, so a second target is accepted - but only when
--- it is a hex this unit could really strike the victim from.
function Script:validateTargets(battle, unit, targets)
	if targets:size() == 1 then return true end
	if targets:size() ~= 2 then return false end

	local victim = battle:getUnitByPos(targets:at(1), true)
	if not victim then return false end

	local around = victim:getSurroundingHexes()
	for i = 1, around:size() do
		if around:at(i) == targets:at(2) then
			return battle:isAccessibleForUnit(unit, targets:at(2))
		end
	end

	return false
end

function Script:getAffectedHexes(battle, unit, targets, hexes)
	hexes:insert(targets:at(1))
	return hexes
end

function Script:execute(server, actions, battle, unit, targets)
	local origin = unit:getPosition()
	local speedBefore = unit:getMovementRange()

	local victim = battle:getUnitByPos(targets:at(1), true)
	if not victim then return end

	--- the owner's chosen side when it sent one, otherwise whatever this unit can reach
	local attackFrom = targets:size() > 1 and targets:at(2) or self:hexToStrikeFrom(battle, unit, victim)
	if not attackFrom then return end

	local distance = 0
	if attackFrom ~= origin then
		distance = actions:walkUnit(battle, unit, attackFrom)
		--- something on the way stopped it, so there is nothing to strike from here
		if unit:getPosition() ~= attackFrom then return end
	end

	actions:performAttack(battle, unit, victim, targets:at(1), distance)

	if not unit:isAlive() or unit:getPosition() == origin then return end

	local path = battle:getPath(unit:getPosition(), origin, unit)
	if path:size() == 0 then return end

	--- the path runs destination first, so movement lost during the attack is how many hexes short
	--- of home the unit stops
	local lost = math.max(0, speedBefore - unit:getMovementRange())
	if lost < path:size() then
		actions:walkUnit(battle, unit, path:at(lost + 1))
	end
end

--- Hex the unit can strike the victim from, preferring to stay where it is.
function Script:hexToStrikeFrom(battle, unit, victim)
	if battle:isMeleeAttackPossible(unit, victim) then
		return unit:getPosition()
	end

	local around = victim:getSurroundingHexes()
	for i = 1, around:size() do
		local hex = around:at(i)
		if battle:isAccessibleForUnit(unit, hex) then
			return hex
		end
	end

	return nil
end

return Script
