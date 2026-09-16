local Base = require("combat/genieSpell")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Test fixture: the shipped genie ability cut down to one conditional spell, so that a condition
--- deciding whether a spell is worth casting can be seen from outside. Precision is the one that
--- answers "only for a subject that shoots".

for _, spell in ipairs({
	"airShield", "antiMagic", "bless", "bloodlust", "counterstrike", "cure", "fireShield", "fortune",
	"frenzy", "haste", "magicMirror", "mirth", "prayer", "protectAir", "protectEarth", "protectFire",
	"protectWater", "shield", "slayer", "stoneSkin"
}) do
	Script:removeSpell(spell)
end

return Script
