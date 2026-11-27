# BattleActionEnvironment

Server operations available to a battle action while it is applied, bound to the battle of the action. Passed to the `apply` function of a battle action script.

### castSpell

Casts the spell of the unit at the target, with its animation, its effects and the battle log line.

- param `caster`: [`Unit`](Unit.md) — Unit whose ability casts the spell.
- param `spell`: [`Spell`](Spell.md) — Spell to cast.
- param `mode`: [`SpellMode`](SpellMode.md) — Way the spell is cast, from `ENUM.SpellMode`.
- param `level`: `integer` — Mastery level of the spell.
- param `target`: `Destination[]` — Destinations the spell is aimed at.

### fireCombatEvent

Runs the combat event scripts of the unit and of every other unit reacting to the event.

- param `event`: [`CombatEvent`](CombatEvent.md) — Event to fire, from `ENUM.CombatEvent`.
- param `unit`: [`Unit`](Unit.md) — Unit the event happens to.
- param `other`: [`Unit`](Unit.md) — Other unit of the event, or nil.
- param `spell`: [`Spell`](Spell.md) — Spell the event is about, or nil.

### triggerObstaclesUnder

Triggers the obstacles on the hexes the unit occupies.

- param `unit`: [`Unit`](Unit.md) — Unit whose hexes are checked.

### rngInt

Returns a random integer from the random generator of the server.

- param `low`: `integer` — Inclusive lower bound.
- param `high`: `integer` — Inclusive upper bound.

- returns `integer` — Random integer in [low, high].
