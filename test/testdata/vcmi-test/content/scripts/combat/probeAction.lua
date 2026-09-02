local Base = require("combat/combatAction")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Test fixture: an attack that hits the enemy it is aimed at together with everything standing
--- next to it, so that one aim can be told apart from a plain attack. Aimable at any live enemy.

function Script:isEnemy(unit, other)
	return other and other:isAlive() and other:getSide() ~= unit:getSide()
end

function Script:getVictims(battle, unit, aimHex)
	local aim = battle:getUnitByPos(aimHex, true)
	if not aim then return {} end

	local victims = { [aim:unitID()] = aim }
	local around = aim:getSurroundingHexes()

	for i = 1, around:size() do
		local other = battle:getUnitByPos(around:at(i), true)
		if self:isEnemy(unit, other) then
			victims[other:unitID()] = other
		end
	end

	return victims
end

function Script:getSelectableHexes(battle, unit, hexes)
	for _, other in ipairs(battle:getUnitsIf(function(candidate) return self:isEnemy(unit, candidate) end)) do
		local occupied = other:getHexes()
		for i = 1, occupied:size() do
			hexes:insert(occupied:at(i))
		end
	end

	return hexes
end

function Script:getAffectedHexes(battle, unit, targets, hexes)
	for _, victim in pairs(self:getVictims(battle, unit, targets:at(1))) do
		local occupied = victim:getHexes()
		for i = 1, occupied:size() do
			hexes:insert(occupied:at(i))
		end
	end

	return hexes
end

function Script:getCursor(battle, unit, targets)
	return "combatHitNorth"
end

function Script:getStatusMessage(battle, unit, targets)
	local count = 0
	for _ in pairs(self:getVictims(battle, unit, targets:at(1))) do count = count + 1 end

	return { append = { "vcmi-test.action.probeStrike" }, replaceNumbers = { count } }
end

function Script:execute(server, battle, unit, targets)
	for _, victim in pairs(self:getVictims(battle, unit, targets:at(1))) do
		server:damageUnit(battle, victim, self.damage)
	end
end

return Script
