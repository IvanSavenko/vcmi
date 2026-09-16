local Base = require("combat/genieSpell")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Test fixture: keeps only the spell named in `only` out of whatever the shipped ability would
--- offer, so that the condition guarding one spell can be observed on its own - the roll otherwise
--- hides which of twenty spells was found useful.

function Script:getUsefulSpells(battle, caster, subject)
	local useful = {}

	for _, spell in ipairs(Base.getUsefulSpells(self, battle, caster, subject)) do
		if spell:getJsonKey() == self.only then
			table.insert(useful, spell)
		end
	end

	return useful
end

return Script
