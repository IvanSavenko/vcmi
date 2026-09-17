local Base = require("combat/combatAction")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Walks up to a unit and casts a spell on it from there, the way a HotA engineer repairs a
--- machine. Scripted equivalent of the ADJACENT_SPELLCASTER bonus.
---
--- Parameters:
---  spell - the spell to cast
---  val   - mastery level it is cast at

function Script:getSpell()
	local spell = LIBRARY:getSpellByName(self.spell)

	if spell == nil then
		error("adjacent spellcast is given the spell " .. tostring(self.spell) .. ", which no mod provides")
	end

	return spell
end

--- The unit the aim names, or nil when the action can do nothing there. Answering nil is what makes
--- a hex unselectable.
function Script:getSubject(battle, unit, hex)
	local subject = battle:getUnitByPos(hex, true)

	if not subject or not self:isSubject(battle, unit, subject) then return nil end

	return subject
end

--- Whether the spell would do anything to this unit, and whether its caster can walk up to it.
function Script:isSubject(battle, unit, subject)
	return subject:isAlive()
		and subject:unitID() ~= unit:unitID()
		and self:hexToReach(battle, unit, subject) ~= nil
		and self:getSpell():canBeCastAt(battle, unit, subject, self.val or 0)
end

function Script:getSelectableHexes(battle, unit, hexes)
	if not unit:canCast() then return hexes end

	for _, other in ipairs(battle:getUnitsIf(function(candidate) return self:isSubject(battle, unit, candidate) end)) do
		local occupied = other:getHexes()
		for i = 1, occupied:size() do
			hexes:insert(occupied:at(i))
		end
	end

	return hexes
end

--- The owner may pick which side to walk up from, so a second target is accepted - but only when it
--- is a hex the spell could really be cast from.
function Script:validateTargets(battle, unit, targets)
	if targets:size() == 1 then return true end
	if targets:size() ~= 2 then return false end

	local subject = battle:getUnitByPos(targets:at(1), true)
	if not subject then return false end

	return self:isHexToReachFrom(battle, unit, subject, targets:at(2))
end

function Script:getAffectedHexes(battle, unit, targets, hexes)
	local subject = self:getSubject(battle, unit, targets:at(1))
	if not subject then return hexes end

	local occupied = subject:getHexes()
	for i = 1, occupied:size() do
		hexes:insert(occupied:at(i))
	end

	return hexes
end

function Script:getCursor(battle, unit, targets)
	return "castSpell"
end

function Script:getStatusMessage(battle, unit, targets)
	local subject = self:getSubject(battle, unit, targets:at(1))

	-- "Select Spell Target", which is what the engine says of a spell aimed at the wrong thing
	if not subject then return { append = { "core.genrltxt.23" } } end

	-- "Cast %s on %s"
	return {
		append = { "core.genrltxt.27" },
		replaceStrings = { self:getSpell():getNameTextID(), subject:getCreature():getNameTextID(subject:getCount()) }
	}
end

function Script:execute(server, actions, battle, unit, targets)
	local subject = self:getSubject(battle, unit, targets:at(1))
	if not subject then return end

	--- the owner's chosen side when it sent one, otherwise whatever this unit can reach
	local castFrom = targets:size() > 1 and targets:at(2) or self:hexToReach(battle, unit, subject)
	if not castFrom then return end

	if castFrom ~= unit:getPosition() then
		actions:walkUnit(battle, unit, castFrom)
		--- something on the way stopped it, so it never arrived to cast
		if unit:getPosition() ~= castFrom then return end
	end

	server:castSpellAsAction(battle, unit, self:getSpell(), { subject }, self.val or 0)
end

return Script
