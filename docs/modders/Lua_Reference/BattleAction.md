# BattleAction

Battle action made by a unit, as the script of its type receives it in `validate` and `apply`.

### side

Side making the action.

- type: [`BattleSide`](BattleSide.md)

### actor

Unit making the action.

- type: [`Unit`](Unit.md)

### spell

Spell of the action, or nil.

- type: [`Spell`](Spell.md)

### target

Destinations of the action.

- type: `Destination[]`

### bonuses

Trigger bonuses of the unit that grant the action, all with the same subtype. Empty for an action without a trigger.

- type: [`BonusList`](BonusList.md)
