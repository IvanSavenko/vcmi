# Combat Action Scripts

`"implements" : "combatAction"`

A combat action script is one more action a unit offers its owner, alongside the ones the engine
provides - attack, shoot, move. The script decides where the action may be aimed, what it does, and
everything the player sees while choosing: the shaded tiles, the cursor and the status bar line.

A unit offers one action per [COMBAT_ACTION](../Bonus/Bonus_Types.md#combat_action) bonus it carries,
so a creature offering two variants of an ability carries the same script twice with different
parameters, rather than the script remembering which variant is active.

Unlike a [combat event script](Combat_Event_Scripts.md), which only ever runs on the server, most of
a combat action script runs on the player's machine as well - that is how the client can shade the
right tiles before anything is sent. Everything except `execute` must therefore answer from the
battle alone, and must not look at anything the owner of the unit cannot see.

## Declaring one

```json
"devour" : {
    "implements" : "combatAction",
    "script" : "combat/devour",
    "patches" : [ ],
    "icon" : "battle/actionDevour",
    "schema" : {
        "properties" : {
            "healPercentage" : { "type" : "number" }
        },
        "additionalProperties" : false
    },
    "description" : "{Devour}\nMoves onto a corpse and consumes it, healing this unit."
}
```

On top of the [shared fields](Script_Types.md#shared-format):

- `icon` - image of the button that offers this action to the player. Required: the player picks
  actions from a row of buttons, and one without an icon would be blank. A script whose artwork
  lives in another mod may leave it out and let that mod contribute it as a patch
  (`{ "otherMod:myAction" : { "icon" : "..." } }`), since patches are merged before the entry is
  validated
- `description` - text shown to the player, used as the tooltip of that button

## Functions

```lua
local Base = require("combat/combatAction")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

function Script:getSelectableHexes(battle, unit, hexes) ... return hexes end
function Script:validateTargets(battle, unit, targets) ... end
function Script:getAffectedHexes(battle, unit, targets, hexes) ... return hexes end
function Script:getCursor(battle, unit, targets) ... end
function Script:getStatusMessage(battle, unit, targets) ... end
function Script:execute(server, actions, battle, unit, targets) ... end

return Script
```

`unit` is the bearer of the bonus. `targets` holds the hexes the owner aimed at, the first of them
being the one the action was aimed at.

### getSelectableHexes(battle, unit, hexes)

Fills and returns `hexes` with the tiles the action may be aimed at. Answer with an empty list when
the action cannot be used this turn - the button is then still shown, but no tile accepts it.

The server validates an incoming action against this answer before running anything, so a target the
script does not offer is refused. That makes this the one function that has to be written as a rule
rather than as a hint.

An action with nothing to aim at - affecting only its bearer, or the whole battlefield - answers with
the bearer's own position, and ignores `targets` in `execute`.

### validateTargets(battle, unit, targets)

Whether aiming the action this way is legal. The first target is already checked against
`getSelectableHexes`; anything past it comes from the owner's client and nothing else looks at it, so
the default answers false unless there is exactly one target.

An action that wants more overrides this and checks them itself. A melee action letting the owner
pick which side to approach from is the case this exists for - the side comes from where the mouse
sits inside the target hex, which only the client knows, so it has to be sent and therefore has to be
checked:

```lua
function Script:validateTargets(battle, unit, targets)
    if targets:size() == 1 then return true end
    if targets:size() ~= 2 then return false end
    -- targets[2] has to be a hex this unit could really strike targets[1] from
end
```

The client offers the extra target rather than assuming it: it builds the longer list, asks this
function, and sends the aim alone if the answer is no. Both sides therefore decide with the same
function and cannot disagree.

### getAffectedHexes(battle, unit, targets, hexes)

Fills and returns `hexes` with the tiles the action would affect if used as aimed. This is what the
player sees shaded while hovering, so it is what tells a special attack apart from an ordinary one.
The default answers with `targets`, which is right for an action that affects only what it hits.

### getCursor(battle, unit, targets)

The name of a cursor declared in `config/cursors.json`, such as `"combatHeal"`, or `""` to keep the
cursor the engine chose. A mod adds cursors of its own by shipping a `config/cursors.json` of its
own, holding only the entries it adds - see the header of that file for the format.

```lua
function Script:getCursor(battle, unit, targets)
    return battle:getUnitByPos(targets:at(1), true) and "myMod.devour" or "combatBlocked"
end
```

### getStatusMessage(battle, unit, targets)

The line written in the status bar while hovering, as a `MetaString` - the same shape the battle log
takes:

```lua
return { append = { "myMod.action.devour" }, replaceNumbers = { healed } }
```

The script is asked about hexes it refuses as well as ones it accepts, so answer with why the aim is
no good rather than leaving the bar blank - the targeting rules that decide it are the script's own.

It is returned unresolved on purpose, so that each client renders it in its own language and the
numbers and names inside it decline correctly. Do not build the line by concatenating a translated
string with a number: the word order and grammatical agreement of other languages are not the ones
English has. Answer with an empty table to leave the bar blank.

Register the identifiers the script names in a translation file of the mod, the same way every other
piece of text is. A message whose text comes from the bonus rather than from the script belongs in a
parameter listed in `stringRegistrations`.

### execute(server, actions, battle, unit, targets)

Carries the action out. The only function of the five that runs on the server, and so the only one
that may change anything. It is reached only after the server has confirmed that the unit really
offers this action and that the target is one `getSelectableHexes` answered with.

Two callbacks rather than one. `server` is the one every kind of script holds - damaging, healing,
casting, adding bonuses. `actions` carries the two things only an action may ask for:

```lua
local distance = actions:walkUnit(battle, unit, hex)
actions:performAttack(battle, unit, victim, hex, distance)
```

`walkUnit` walks a path, triggering whatever the unit crosses, and answers how far it got - which may
be short of where it was headed, so check where the unit ended up if that matters. `performAttack`
runs a whole melee attack the way the engine does: first strike, every blow the attacker is entitled
to, and the retaliation. A script asking for one owns none of those rules.

### Helpers of the base class

Two methods every action inherits, for the common case of an action that has to stand next to what it
is aimed at:

- `hexToReach(battle, unit, target)` - hex the unit has to stand on to touch `target`, preferring the
  one it already stands on, or nil when it can reach none
- `isHexToReachFrom(battle, unit, target, hex)` - whether `hex` is one such hex, which is what
  `validateTargets` checks a second target against

## An empty list is not the same as no button

Whether a unit offers the action at all is decided by whether it carries the bonus, not by the
script. `getSelectableHexes` decides only whether the action can be used *right now*. An ability that
should disappear entirely under some condition belongs behind a bonus limiter instead.

## Built-in scripts

### genieSpell

Casts a beneficial spell on another allied unit, picked at random among those that would actually
help it, the way a master genie does. Replaces the `RANDOM_SPELLCASTER` bonus, and ships with the
repertoire of the H3 master genie.

Which spell is rolled is settled in `execute`, on the server, so that every client is told the same
one. Only hexes where at least one spell is left to gain are offered, so the roll never draws from an
empty list. A spell already in effect on the subject is skipped, and so is one the spell's own rules
refuse - `canBeCastAt` answers most of it, and only the rules below need more than that.

Parameters:

- `val` - mastery level the rolled spell is cast at

#### Changing the repertoire

The list is built by `addSpell` calls at the bottom of the script, the same way the damage calculator
builds its factors, so a patch may add to it, take from it, or change when one of its spells counts:

```lua
--- Mod feature: the genie may also lay a shield of thorns, but only on somebody being hit
function Script:subjectIsSurrounded(battle, caster, subject)
    return #battle:getUnitsIf(function(other)
        return other:getOwner() ~= subject:getOwner() and subject:getSurroundingHexes():contains(other:getPosition())
    end) > 0
end

Script:addSpell("myMod:thornShield", "subjectIsSurrounded")
Script:removeSpell("slayer")

return Script
```

`addSpell(spell, condition)` takes the *name* of the condition method rather than the method itself,
so that a patch stacked later can override it and be the one that runs. The condition is called as
`self:condition(battle, caster, subject)` and answers whether the spell is worth casting; leave it
out for a spell that always is. The conditions of the shipped list - `enemyCanShoot`,
`enemyFightsInMelee`, `enemyHasKing`, `enemyHasSpellbook`, `subjectIsHurt`, `subjectHasTurnLeft`,
`subjectCanShoot`, `subjectFightsInMelee` - are ordinary methods and can be overridden the same way,
as is `threatensInMelee(battle, unit)`, which decides what counts as a melee threat.

A patch never changes the list of the script it extends: the first `addSpell` or `removeSpell` copies
it, so a mod extending `genieSpell` does not alter the genie of the base game.


### adjacentSpellcast

Walks up to a unit and casts a spell on it from there, the way a HotA engineer repairs a machine.
Replaces the `ADJACENT_SPELLCASTER` bonus, which is converted to this script on load.

Parameters:

- `spell` - the spell to cast
- `val` - mastery level it is cast at

Only hexes where the spell would actually do something are offered, and only while the caster can
still cast at all, so the action disappears once its `CASTS` are spent. The owner may pick which side
to walk up from, which the action takes as a second target.

It declares no icon, so it gets no button in the unit action panel - the spell is what differs between
the abilities using this script, and one shared icon would name none of them. The status bar names the
spell and the unit but not what the spell would do, which the engine's version estimated.

### attackAndReturn

Walks up to an enemy, strikes it and flies back to where it started, the way a harpy does. Replaces
the `RETURN_AFTER_STRIKE` bonus, which is converted to this script on load.

The attack itself is the engine's, so the script owns none of the rules about first strike, multiple
blows or retaliation. What it owns is the return, and the one subtlety in it: a unit slowed while
attacking does not make it all the way home, so the flight back is shortened by however much movement
it lost.

The owner may pick which side to approach the victim from, which the action takes as a second target.

## Why walking and attacking are kept apart

An attack sets off [combat event scripts](Combat_Event_Scripts.md) - that is what fire shield and
death stare are. Those hold a `server` of their own, and if `performAttack` lived on it, a reaction
could ask for another attack from inside the attack that woke it, and that one would wake it again,
without end.

So the two live on `actions`, which only a combat action is ever handed. The chain is therefore at
most action -> attack -> reaction, and a reaction has no way to extend it. An action cannot start
another action either, for the same reason: there is no binding for it.
