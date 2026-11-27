--- First aid: heals a unit of the own side with the spell of the HEALER bonus.
--- Without a target, heals the most wounded unit, as the first aid tent does on its own
local Base = require("battleAction/unitAction")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

local SPELL_LIKE_ATTACK = ENUM.SpellMode.spellLikeAttack

local function addProblem(problem, textID)
	if problem then
		problem:addCustom({ append = { textID } })
	end
end

--- Spell of the trigger bonuses of the option or action, or nil
function Script:getHealerSpell(bonuses)
	return LIBRARY:getSpellByName(bonuses:getBonus(1):getSubtype())
end

--- Unit named by the destination, or the living unit on its hex
function Script:getDestinationUnit(battle, destination)
	return destination.unit or battle:getUnitByPos(destination.hex, true)
end

--- True if the healer can heal the unit with the spell. Otherwise, explains the reason through `problem`, if given
function Script:canHeal(battle, healer, spell, healed, problem)
	if healed:getSide() ~= healer:getSide() then
		addProblem(problem, "vcmi.battle.problem.healTargetNotOwnSide")
		return false
	end

	-- canCastAt accepts immune units, on which H3 lets spells be wasted, but first aid must not waste the turn of the tent
	local target = { { unit = healed, hex = healed:getPosition() } }
	if not battle:canCastAt(healer, spell, SPELL_LIKE_ATTACK, target, problem) or not battle:isReceptive(healer, spell, SPELL_LIKE_ATTACK, healed) then
		addProblem(problem, "vcmi.battle.problem.healTargetNotHealable")
		return false
	end
	return true
end

--- Unit that has lost the most health and that the healer can heal, or nil
function Script:findMostWounded(battle, healer, spell)
	local result = nil
	local mostLost = 0
	local units = battle:getUnitsIf(function(unit) return unit:isAlive() end)
	for _, unit in ipairs(units) do
		local lost = unit:getMaxHealth() - unit:getFirstHPleft()
		if lost > mostLost and self:canHeal(battle, healer, spell, unit, nil) then
			mostLost = lost
			result = unit
		end
	end
	return result
end

function Script:isAvailable(battle, unit, option)
	return battle:getTacticDistance() == 0
end

function Script:preview(battle, option, context)
	if not context.legal then
		return { cursor = "combatBlocked" }
	end

	local target = battle:getUnitByPos(context.hoveredHex, true)
	local creature = target:getCreature()
	return {
		cursor = "combatHeal",
		text = { append = { "core.genrltxt.419" }, replaceStrings = { creature:getNameTextID(target:getCount()) } }, -- Apply first aid to the %s
		healthGain = battle:getFirstAidHealValue(context.actor, target),
		healthGainTextID = "vcmi.battleWindow.healValuePreview",
	}
end

function Script:buildTarget(battle, option, context)
	return { { hex = context.hoveredHex } }
end

function Script:validate(battle, action, problem)
	local unit = action.actor
	local spell = self:getHealerSpell(action.bonuses)
	if not spell then
		addProblem(problem, "vcmi.battle.problem.cannotHeal")
		return false
	end

	if #action.target == 0 then
		return true
	end

	local healed = self:getDestinationUnit(battle, action.target[1])
	if not healed then
		addProblem(problem, "vcmi.battle.problem.healTargetNotUnit")
		return false
	end

	return self:canHeal(battle, unit, spell, healed, problem)
end

function Script:apply(env, battle, action)
	local unit = action.actor
	local spell = self:getHealerSpell(action.bonuses)

	if #action.target > 0 then
		local destination = action.target[1]
		env:castSpell(unit, spell, SPELL_LIKE_ATTACK, 0, { { unit = self:getDestinationUnit(battle, destination), hex = destination.hex } })
		return
	end

	local healed = self:findMostWounded(battle, unit, spell)
	if healed then
		env:castSpell(unit, spell, SPELL_LIKE_ATTACK, 0, { { unit = healed, hex = healed:getPosition() } })
	end
end

return Script
