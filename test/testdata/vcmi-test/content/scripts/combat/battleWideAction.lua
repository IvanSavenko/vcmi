local Base = require("combat/combatAction")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Test fixture: an action with nothing to aim at - it damages every enemy on the field. Offered on
--- the bearer's own tile, which is what an action that needs no target is aimed at.

function Script:getSelectableHexes(battle, unit, hexes)
	hexes:insert(unit:getPosition())
	return hexes
end

function Script:execute(server, battle, unit, targets)
	for _, other in ipairs(battle:getUnitsIf(function(candidate)
		return candidate:isAlive() and candidate:getSide() ~= unit:getSide()
	end)) do
		server:damageUnit(battle, other, self.damage)
	end
end

return Script
