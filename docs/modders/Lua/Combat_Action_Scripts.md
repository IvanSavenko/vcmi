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
  actions from a row of buttons, and one without an icon would be blank
- `description` - text shown to the player, used as the tooltip of that button

## Functions

```lua
local Base = require("combat/combatAction")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

function Script:getSelectableHexes(battle, unit, hexes) ... return hexes end
function Script:getAffectedHexes(battle, unit, targets, hexes) ... return hexes end
function Script:getCursor(battle, unit, targets) ... end
function Script:getStatusMessage(battle, unit, targets) ... end
function Script:execute(server, battle, unit, targets) ... end

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

It is returned unresolved on purpose, so that each client renders it in its own language and the
numbers and names inside it decline correctly. Do not build the line by concatenating a translated
string with a number: the word order and grammatical agreement of other languages are not the ones
English has. Answer with an empty table to leave the bar blank.

Register the identifiers the script names in a translation file of the mod, the same way every other
piece of text is. A message whose text comes from the bonus rather than from the script belongs in a
parameter listed in `stringRegistrations`.

### execute(server, battle, unit, targets)

Carries the action out. The only function of the five that runs on the server, and so the only one
that may change anything. It is reached only after the server has confirmed that the unit really
offers this action and that the target is one `getSelectableHexes` answered with.

## An empty list is not the same as no button

Whether a unit offers the action at all is decided by whether it carries the bonus, not by the
script. `getSelectableHexes` decides only whether the action can be used *right now*. An ability that
should disappear entirely under some condition belongs behind a bonus limiter instead.

## Nothing may re-enter an action

`execute` may damage, move, heal and cast, and those may in turn set off combat event scripts. What
it may not do is start another action - there is no binding for that, deliberately, so that the chain
stays at most one step deep.
