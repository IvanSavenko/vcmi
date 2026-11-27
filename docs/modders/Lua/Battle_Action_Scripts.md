# Battle Action Scripts

Declared with `"implements" : "battleAction"` in the [scripts](Script_Types.md) section of a mod. This script type holds the rules of a battle action of a unit, such as the built-in `core:heal`. The action itself is declared in the `battleActions` section of a mod and names its script.

## Declaring an action

An action entry in the `battleActions` section:

- `script` - identifier of the `battleAction` script with the rules of the action
- `trigger` - bonus types that grant the action, as `{ "type" : "HEALER" }` or `{ "type" : [ "TYPE_A", "TYPE_B" ] }`. A unit gets one option of the action per distinct subtype of these bonuses; bonuses with the same subtype, or with none, give one option. Without a trigger, the action is offered to every unit
- `priority` - order in which the client tries the actions of a unit for a hovered hex, lower first. Used when the script returns no priority of its own
- `button` - optional button of the unit action panel that selects the action, with `order` (position in the panel, lower first), `icon` and `tooltip`. A tooltip starting with `@` is the identifier of an existing text
- `turnStatus` - turn status of the unit once the action starts: `acted` (default), `waiting`, `defended`, or `keep` to leave it as it is
- `usesUnitSpellcast` - the action uses up the spellcasting of the unit for the turn
- `tacticsAction` - the action is allowed during the tactics phase

```json
"battleActions" : {
	"heal" : {
		"script" : "healAction",
		"trigger" : { "type" : "HEALER" },
		"priority" : 12
	}
},
"scripts" : {
	"healAction" : {
		"implements" : "battleAction",
		"script" : "battleAction/heal",
		"patches" : [ ],
		"schema" : { "properties" : {}, "additionalProperties" : false }
	}
}
```

Action and script identifiers are separate: `core:heal` is the action, `core:healAction` its script, and `core:heal` among scripts is a spell effect.

A new action of a mod usually declares its own bonus type as its trigger, in the `bonuses` section of the mod, so that the creature window shows the ability with its description and icon.

## Writing a script

Scripts are kept in `scripts/battleAction/`. A script derives from the base script `battleAction/unitAction`, which provides a default for every function, and overrides the functions it needs:

```lua
local Base = require("battleAction/unitAction")
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

function Script:isAvailable(battle, unit, option)
	return battle:getTacticDistance() == 0
end

return Script
```

Call other functions of the script through `self`, never through `Base` or a local function, so that a script derived from yours can override them. A script may derive from another action script in the same way, for example `require("battleAction/heal")`.

The functions receive the battle as [Battle](../Lua_Reference/Battle.md), units as [Unit](../Lua_Reference/Unit.md), and the other data as tables: [ActionOption](../Lua_Reference/ActionOption.md), [ActionContext](../Lua_Reference/ActionContext.md) and [BattleAction](../Lua_Reference/BattleAction.md). Changing a table changes nothing in the game.

## Trigger bonuses

`option.bonuses` and `action.bonuses` are the trigger bonuses of the unit that grant the option or the action, all with the same subtype, as a [BonusList](../Lua_Reference/BonusList.md). A script reads the parameters of its ability from them: for example, first aid casts the spell named by the subtype. Only the action type and the subtype are sent over the network, and the server collects the bonuses again.

## Assumptions and guarantees

- the game calls the functions in any order and any number of times: on the client for every hovered hex, on the server for every action, and in AI for estimates
- `validate` is called only for a unit that exists, is alive, may act now and has the trigger bonuses of the action
- `apply` is called only for an action that passed `validate`
- the global state of the script is not kept between calls

## Functions

### isAvailable

Signature: `function Script:isAvailable(battle, unit, option)`

Returns true if the action offers the option to the unit on its turn. Called only for the options that the trigger gives the unit. Default: true.

### getPriority

Signature: `function Script:getPriority(battle, option, unit, target)`

Returns the order in which the client tries the options of the unit for a hovered hex, lower first. `target` is the unit on the hovered hex, or nil. Default: nil, which uses `priority` of the action.

### preview

Signature: `function Script:preview(battle, option, context)`

Returns the [ActionPreview](../Lua_Reference/ActionPreview.md) that the client shows for the option on the hovered hex. `context.legal` is true if clicking the hex makes an action that passes `validate`. Default: the `combatBlocked` cursor for a hex that is not legal, and `combatPointer` otherwise.

### buildTarget

Signature: `function Script:buildTarget(battle, option, context)`

Returns the destinations of the action made by clicking the hovered hex, as a `Destination[]`, or nil if the option has no action for the hex. Default: nil.

### validate

Signature: `function Script:validate(battle, action, problem)`

Returns true if the side may make the action. Otherwise, adds the reason to `problem`, a [SpellProblem](../Lua_Reference/SpellProblem.md). Runs on the server for every action, and on the client to check hovered hexes. Default: false.

### apply

Signature: `function Script:apply(env, battle, action)`

Applies the action through `env`, a [BattleActionEnvironment](../Lua_Reference/BattleActionEnvironment.md). Runs only on the server. Default: does nothing.
