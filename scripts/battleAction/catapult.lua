--- Catapult shot: casts the spell of the CATAPULT bonus at a wall section.
--- Without a target, the spell picks the wall section
local Base = require("battleAction/unitAction")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

local SPELL_LIKE_ATTACK = ENUM.SpellMode.spellLikeAttack

--- Spell of the trigger bonuses of the option or action, or nil
function Script:getCatapultSpell(bonuses)
	return LIBRARY:getSpellByName(bonuses:getBonus(1):getSubtype())
end

function Script:isAvailable(battle, unit, option)
	return battle:getTacticDistance() == 0 and battle:hasFortifications()
end

function Script:preview(battle, option, context)
	if not context.legal then
		return { cursor = "combatBlocked" }
	end
	return { cursor = "combatShootCatapult" }
end

function Script:buildTarget(battle, option, context)
	return { { hex = context.hoveredHex } }
end

function Script:validate(battle, action, problem)
	if not self:getCatapultSpell(action.bonuses) then
		problem:addCustom({ append = { "vcmi.battle.problem.cannotShootWalls" } })
		return false
	end

	local canHitWall
	if #action.target == 0 then
		canHitWall = battle:getAttackableWallParts():size() > 0
	else
		canHitWall = battle:isWallPartAttackable(battle:hexToWallPart(action.target[1].hex))
	end

	if not canHitWall then
		problem:addCustom({ append = { "vcmi.battle.problem.noWallToShoot" } })
		return false
	end
	return true
end

function Script:apply(env, battle, action)
	local unit = action.actor
	local ability = action.bonuses:getBonus(1)
	local spell = self:getCatapultSpell(action.bonuses)
	local shotLevel = unit:getBonusesValue({ type = "CATAPULT_EXTRA_SHOTS", subtype = ability:getSubtype() })

	env:castSpell(unit, spell, SPELL_LIKE_ATTACK, shotLevel, action.target)
end

return Script
