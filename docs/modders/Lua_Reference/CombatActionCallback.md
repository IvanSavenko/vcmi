# CombatActionCallback

Moving and attacking, as the engine does them when a unit acts. Handed only to a combat action script, and deliberately absent from the server callback every other kind of script holds: an attack wakes combat event scripts, and one of those asking for another attack from inside it would never end.

### walkUnit

Walks the unit along a path to the destination the way a move action does, triggering whatever it crosses, and answers how far it got. That may be short of the destination when something stopped it, so a script that cares has to check where the unit ended up. Unlike server:moveUnit, which places the unit on a hex without it travelling there.

- param `battle`: [`Battle`](Battle.md) — Battle in which the unit walks.
- param `unit`: [`Unit`](Unit.md) — Unit to walk.
- param `destination`: [`BattleHex`](BattleHex.md) — Hex to walk to.

- returns `integer`

### performAttack

Runs one melee attack to its end - first strike, every blow the attacker is entitled to, the retaliation - exactly as the engine runs the attack of a unit. The attack rules stay with the engine, so a script asking for one does not have to know any of them.

- param `battle`: [`Battle`](Battle.md) — Battle the attack happens in.
- param `attacker`: [`Unit`](Unit.md) — Unit making the attack.
- param `defender`: [`Unit`](Unit.md) — Unit being attacked.
- param `targetHex`: [`BattleHex`](BattleHex.md) — Hex the blow lands on.
- param `distance`: `integer` — Hexes the attacker travelled to reach it, which a charge scales with.
