# BattleAction

Battle action made by a unit, as the script of its type receives it in `validate` and `apply`.

### side

Side making the action.

- type: [`BattleSide`](BattleSide.md)

### actor

Unit making the action, or nil if the battle has no such unit.

- type: [`Unit`](Unit.md)

### spell

Spell of the action, or nil.

- type: [`Spell`](Spell.md)

### target

Destinations of the action.

- type: `Destination[]`
