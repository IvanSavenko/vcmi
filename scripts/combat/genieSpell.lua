local Base = require("combat/combatAction")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Puts a beneficial spell on another allied unit, picked at random among those that would actually
--- help it. Scripted equivalent of the RANDOM_SPELLCASTER bonus.
---
--- A mod extends the repertoire through a patch: `addSpell` puts another spell in it, and the
--- condition deciding when a spell is worth casting is a method that a patch may override.
---
--- Parameters:
---  val - mastery level the rolled spell is cast at

--- Every spell the ability may roll, each with the name of the method deciding whether it is worth
--- casting right now - or nil for a spell that always is.
Script.candidateSpells = {}

--- The repertoire this script may change, which is its own copy the first time it is asked for.
--- Without this a patch would edit the list of the script it extends, and every other user of that
--- script would see the change.
function Script:ownSpells()
	if rawget(self, "candidateSpells") == nil then
		local own = {}

		for _, entry in ipairs(self.candidateSpells) do
			table.insert(own, entry)
		end

		rawset(self, "candidateSpells", own)
	end

	return self.candidateSpells
end

--- Adds a spell to the repertoire. `condition` names a method called as
--- `self:condition(battle, caster, subject)`, by name rather than by value so that a patch stacked
--- later can override it and be the one that runs.
--- NOTE: must be called in the script body, not in a function. See the registrations below.
function Script:addSpell(spell, condition)
	if condition ~= nil and self[condition] == nil then
		error("genie is given the spell " .. spell .. " under a condition " .. condition .. ", which is not one of its methods")
	end

	local own = self:ownSpells()

	for _, entry in ipairs(own) do
		if entry.spell == spell then
			error("genie is given the spell " .. spell .. " twice")
		end
	end

	table.insert(own, { spell = spell, condition = condition })
end

--- Drops a spell from the repertoire, for a patch that wants one of the spells below gone.
function Script:removeSpell(spell)
	local own = self:ownSpells()

	for index, entry in ipairs(own) do
		if entry.spell == spell then
			table.remove(own, index)
			return
		end
	end

	error("genie can not drop the spell " .. spell .. ", which it does not have")
end

--- Any live enemy of the subject that the predicate holds for, or nil when there is none.
function Script:enemyOf(battle, subject, predicate)
	local found = battle:getUnitsIf(function(candidate)
		return candidate:getOwner() ~= subject:getOwner() and candidate:isValidTarget(false) and predicate(candidate)
	end)

	return found[1]
end

function Script:enemyCanShoot(battle, caster, subject)
	return self:enemyOf(battle, subject, function(enemy) return battle:canShoot(enemy) end) ~= nil
end

--- A unit threatens in melee when it can strike at all, is not held by blind, paralysis or the
--- like, and is not busy shooting - a shooter hemmed in by enemies is back to using its fists.
function Script:threatensInMelee(battle, unit)
	return unit:isMeleeAttacker() and not unit:hasBonuses({type = "NOT_ACTIVE"}) and not battle:canShoot(unit)
end

function Script:enemyFightsInMelee(battle, caster, subject)
	return self:enemyOf(battle, subject, function(enemy) return self:threatensInMelee(battle, enemy) end) ~= nil
end

--- Slayer only reaches creatures whose KING value the mastery of the cast covers, so an enemy it
--- would not touch is no reason to cast it.
function Script:enemyHasKing(battle, caster, subject)
	local level = self.val or 0

	return self:enemyOf(battle, subject, function(enemy)
		local kings = enemy:getBonuses({type = "KING"})

		for i = 1, kings:size() do
			if kings:getBonus(i):getVal() <= level then return true end
		end

		return false
	end) ~= nil
end

function Script:enemyHasSpellbook(battle, caster, subject)
	local enemySide = ENUM.BattleSide.defender

	if subject:getSide() == ENUM.BattleSide.defender then
		enemySide = ENUM.BattleSide.attacker
	end

	local hero = battle:getHero(enemySide)

	return hero ~= nil and hero:hasArtifact("spellBook")
end

function Script:subjectIsHurt(battle, caster, subject)
	return subject:getFirstHPleft() < subject:getMaxHealth()
end

--- Prayer lasts the round, so it is worth something only to a unit whose turn is still to come -
--- which a unit that waited still has.
function Script:subjectHasTurnLeft(battle, caster, subject)
	return subject:willMove(0)
end

function Script:subjectCanShoot(battle, caster, subject)
	return battle:canShoot(subject)
end

function Script:subjectFightsInMelee(battle, caster, subject)
	return not battle:canShoot(subject)
end

--- Json keys of the spells already in effect on the unit. Read in one go, because it rules a spell
--- out more cheaply than anything else here and is asked about once per candidate.
function Script:spellsAffecting(unit)
	local present = {}
	local bonuses = unit:getBonuses({ sourceType = ENUM.BonusSource.spellEffect })

	for i = 1, bonuses:size() do
		present[bonuses:getBonus(i):getSourceID()] = true
	end

	return present
end

--- Spells the caster could usefully put on the subject right now, in registration order.
function Script:getUsefulSpells(battle, caster, subject)
	local affecting = self:spellsAffecting(subject)
	local level = self.val or 0
	local useful = {}

	for _, entry in ipairs(self.candidateSpells) do
		local spell = LIBRARY:getSpellByName(entry.spell)

		if spell == nil then
			error("genie is given the spell " .. entry.spell .. ", which no mod provides")
		end

		-- cheapest test first: a table read, then a handful of queries, then the full cast check
		if not affecting[spell:getJsonKey()]
			and (entry.condition == nil or self[entry.condition](self, battle, caster, subject))
			and spell:canBeCastAt(battle, caster, subject, level)
		then
			table.insert(useful, spell)
		end
	end

	return useful
end

function Script:isSubject(unit, other)
	return other:isAlive() and other:getSide() == unit:getSide() and other:unitID() ~= unit:unitID()
end

--- The unit the aim names, or nil when the action can do nothing there. Answering nil is what makes
--- a hex unselectable, so the roll never has an empty list to draw from.
function Script:getSubject(battle, unit, hex)
	if not unit:canCast() then return nil end

	local subject = battle:getUnitByPos(hex, true)

	if not subject or not self:isSubject(unit, subject) then return nil end
	if #self:getUsefulSpells(battle, unit, subject) == 0 then return nil end

	return subject
end

function Script:getSelectableHexes(battle, unit, hexes)
	if not unit:canCast() then return hexes end

	for _, other in ipairs(battle:getUnitsIf(function(candidate) return self:isSubject(unit, candidate) end)) do
		if #self:getUsefulSpells(battle, unit, other) > 0 then
			local occupied = other:getHexes()
			for i = 1, occupied:size() do
				hexes:insert(occupied:at(i))
			end
		end
	end

	return hexes
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

	-- "Cast a spell on %s"
	return {
		append = { "core.genrltxt.301" },
		replaceStrings = { subject:getCreature():getNameTextID(subject:getCount()) }
	}
end

function Script:execute(server, battle, unit, targets)
	local subject = self:getSubject(battle, unit, targets:at(1))
	if not subject then return end

	local useful = self:getUsefulSpells(battle, unit, subject)
	if #useful == 0 then return end

	server:castSpellAsAction(battle, unit, useful[server:rngInt(1, #useful)], { subject }, self.val or 0)
end

--- The repertoire of the H3 master genie. A spell is left out while it would change nothing, which
--- for most of them the spell's own rules already say - only the ones named below need more.
Script:addSpell("airShield", "enemyCanShoot")
Script:addSpell("antiMagic", "enemyHasSpellbook")
Script:addSpell("bless")
Script:addSpell("bloodlust", "subjectFightsInMelee")
Script:addSpell("counterstrike")
Script:addSpell("cure", "subjectIsHurt")
Script:addSpell("fireShield", "enemyFightsInMelee")
Script:addSpell("fortune")
Script:addSpell("frenzy")
Script:addSpell("haste")
Script:addSpell("magicMirror", "enemyHasSpellbook")
Script:addSpell("mirth")
Script:addSpell("prayer", "subjectHasTurnLeft")
Script:addSpell("precision", "subjectCanShoot")
Script:addSpell("protectAir", "enemyHasSpellbook")
Script:addSpell("protectEarth", "enemyHasSpellbook")
Script:addSpell("protectFire", "enemyHasSpellbook")
Script:addSpell("protectWater", "enemyHasSpellbook")
Script:addSpell("shield", "enemyFightsInMelee")
Script:addSpell("slayer", "enemyHasKing")
Script:addSpell("stoneSkin")

return Script
