--- Random spell of a genie: casts a beneficial spell, picked at random by the server, at a unit of the own side.
--- A mod adds a spell to the candidates with a patch that appends to the list of `Base.getCandidateSpells(self)`
local Base = require("battleAction/unitAction")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

local CREATURE_ACTIVE = ENUM.SpellMode.creatureActive

local function addProblem(problem, textID)
	problem:addCustom({ append = { textID } })
end

local function otherSide(side)
	if side == ENUM.BattleSide.attacker then
		return ENUM.BattleSide.defender
	end
	return ENUM.BattleSide.attacker
end

--- True if an enemy of the subject that can be targeted matches the predicate
function Script:hasEnemy(battle, subject, predicate)
	local found = battle:getUnitsIf(function(unit)
		return battle:isHostile(subject, unit) and unit:isValidTarget(false) and predicate(unit)
	end)
	return #found > 0
end

--- True if the side opposing the subject has a hero, who may cast spells at it
function Script:hasEnemyHero(battle, subject)
	return battle:hasHero(otherSide(subject:getSide()))
end

--- Spells that the genie may cast, in the order of the random pick. `check` tells whether the spell is of use to the subject;
--- spells without a check are of use whenever they can be cast at the subject
function Script:getCandidateSpells()
	return {
		{ spell = "shield", check = function(self, battle, caster, subject) return self:hasEnemy(battle, subject, function(unit) return not unit:canShoot() end) end },
		{ spell = "airShield", check = function(self, battle, caster, subject) return self:hasEnemy(battle, subject, function(unit) return unit:canShoot() end) end },
		{ spell = "fireShield", check = function(self, battle, caster, subject) return self:hasEnemy(battle, subject, function(unit) return not unit:canShoot() end) end },
		{ spell = "protectAir", check = function(self, battle, caster, subject) return self:hasEnemyHero(battle, subject) end },
		{ spell = "protectFire", check = function(self, battle, caster, subject) return self:hasEnemyHero(battle, subject) end },
		{ spell = "protectWater", check = function(self, battle, caster, subject) return self:hasEnemyHero(battle, subject) end },
		{ spell = "protectEarth", check = function(self, battle, caster, subject) return self:hasEnemyHero(battle, subject) end },
		{ spell = "antiMagic", check = function(self, battle, caster, subject) return self:hasEnemyHero(battle, subject) end },
		{ spell = "magicMirror", check = function(self, battle, caster, subject) return self:hasEnemyHero(battle, subject) end },
		{ spell = "cure", check = function(self, battle, caster, subject) return subject:getFirstHPleft() < subject:getMaxHealth() end },
		{ spell = "bless" },
		{ spell = "bloodlust", check = function(self, battle, caster, subject) return not subject:canShoot() end },
		{ spell = "precision", check = function(self, battle, caster, subject) return subject:canShoot() end },
		{ spell = "stoneSkin" },
		{ spell = "prayer" },
		{ spell = "mirth" },
		{ spell = "fortune" },
		{ spell = "haste" },
		{ spell = "slayer", check = function(self, battle, caster, subject) return self:hasEnemy(battle, subject, function(unit) return unit:hasBonuses({ type = "KING" }) end) end },
		{ spell = "counterstrike" },
	}
end

--- Candidate spells that are of use to the subject: not already affecting it, castable at it, and passing their check
function Script:getBeneficialSpells(battle, caster, subject)
	local active = {}
	local effects = subject:getBonuses({ sourceType = ENUM.BonusSource.spellEffect })
	for i = 1, effects:size() do
		local source = LIBRARY:getSpellByName(effects:getBonus(i):getSourceID())
		if source then
			active[source:getJsonKey()] = true
		end
	end

	local result = {}
	local target = { { unit = subject, hex = subject:getPosition() } }
	for _, candidate in ipairs(self:getCandidateSpells()) do
		local spell = LIBRARY:getSpellByName(candidate.spell)
		if spell and not active[spell:getJsonKey()]
			and battle:canCastAt(caster, spell, CREATURE_ACTIVE, target, nil)
			and (candidate.check == nil or candidate.check(self, battle, caster, subject)) then
			table.insert(result, spell)
		end
	end
	return result
end

--- Unit named by the destination, or the living unit on its hex
function Script:getDestinationUnit(battle, destination)
	return destination.unit or battle:getUnitByPos(destination.hex, true)
end

function Script:isAvailable(battle, unit, option)
	return battle:getTacticDistance() == 0 and unit:canCast()
end

function Script:preview(battle, option, context)
	if not context.legal then
		return { cursor = "combatBlocked", text = { append = { "core.genrltxt.23" } } }
	end

	local subject = battle:getUnitByPos(context.hoveredHex, true)
	return {
		cursor = "castSpell",
		text = { append = { "core.genrltxt.301" }, replaceStrings = { subject:getCreature():getNameTextID(subject:getCount()) } }, -- Cast a spell on %s
	}
end

function Script:buildTarget(battle, option, context)
	return { { hex = context.hoveredHex } }
end

function Script:validate(battle, action, problem)
	local unit = action.actor
	if not unit:canCast() then
		addProblem(problem, "vcmi.battle.problem.cannotCastNow")
		return false
	end

	local subject = nil
	if #action.target == 1 then
		subject = self:getDestinationUnit(battle, action.target[1])
	end
	if not subject or subject == unit or not subject:isAlive() or battle:isHostile(unit, subject) then
		addProblem(problem, "vcmi.battle.problem.randomSpellTargetInvalid")
		return false
	end

	if #self:getBeneficialSpells(battle, unit, subject) == 0 then
		addProblem(problem, "vcmi.battle.problem.noBeneficialSpell")
		return false
	end
	return true
end

function Script:apply(env, battle, action)
	local unit = action.actor
	local subject = self:getDestinationUnit(battle, action.target[1])
	local spells = self:getBeneficialSpells(battle, unit, subject)
	local spell = spells[env:rngInt(0, #spells - 1) + 1]
	local level = unit:getAbilitySpellLevel(math.max(0, action.bonuses:getBonus(1):getVal()), spell)

	env:castSpell(unit, spell, CREATURE_ACTIVE, level, action.target)
	env:fireCombatEvent(ENUM.CombatEvent.unitSpellcast, unit, nil, spell)
	env:triggerObstaclesUnder(unit)
end

return Script
