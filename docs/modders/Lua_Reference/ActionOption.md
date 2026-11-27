# ActionOption

One way for a unit to use a battle action, offered to the player on the unit's turn. Passed to the functions of a battle action script.

### spell

Spell cast by the option, or nil.

- type: [`Spell`](Spell.md)

### bonuses

Trigger bonuses of the unit that grant the option, all with the same subtype. Empty for an action without a trigger.

- type: [`BonusList`](BonusList.md)
