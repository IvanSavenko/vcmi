local Base = require("combat/combatScript")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Test fixture: a reaction that tries to ask for an attack from inside the attack that woke it.
--- Walking and attacking are not on the server callback a reaction holds, so the call cannot be
--- made at all - which is the whole point of keeping them apart.

function Script:onBeforeAttack(server, battle, unit, other, payload)
	if other and server.performAttack then
		error("a combat event script must not be able to ask for an attack")
	end
end

return Script
