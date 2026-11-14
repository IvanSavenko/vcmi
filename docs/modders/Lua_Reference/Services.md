# Services

The static game-content catalogue, bound to the global `LIBRARY`. Looks up artifacts, creatures, factions, hero classes/types, resources, secondary skills and spells by their config name (the same string used in JSON definitions).

### getArtifactByName

Looks up an artifact by its JSON key. Returns nil if not found.

- param `name`: `string` — JSON key of the artifact (e.g. `core:goldenBow`).
- param `scope`: `string?` — Mod from which configuration the name was taken, see `scopeOf`. Defaults to the mod of the calling script.

- returns [`Artifact`](Artifact.md)

### getCreatureByName

Looks up a creature by its JSON key. Returns nil if not found.

- param `name`: `string` — JSON key of the creature (e.g. `core:pikeman`).
- param `scope`: `string?` — Mod from which configuration the name was taken, see `scopeOf`. Defaults to the mod of the calling script.

- returns [`Creature`](Creature.md)

### getFactionByName

Looks up a faction by its JSON key. Returns nil if not found.

- param `name`: `string` — JSON key of the faction (e.g. `core:castle`).
- param `scope`: `string?` — Mod from which configuration the name was taken, see `scopeOf`. Defaults to the mod of the calling script.

- returns [`Faction`](Faction.md)

### getHeroClassByName

Looks up a hero class by its JSON key. Returns nil if not found.

- param `name`: `string` — JSON key of the hero class (e.g. `core:knight`).
- param `scope`: `string?` — Mod from which configuration the name was taken, see `scopeOf`. Defaults to the mod of the calling script.

- returns [`HeroClass`](HeroClass.md)

### getHeroTypeByName

Looks up a hero type by its JSON key. Returns nil if not found.

- param `name`: `string` — JSON key of the hero type (e.g. `core:orrin`).
- param `scope`: `string?` — Mod from which configuration the name was taken, see `scopeOf`. Defaults to the mod of the calling script.

- returns [`HeroType`](HeroType.md)

### getResourceByName

Looks up a resource by its JSON key. Returns nil if not found.

- param `name`: `string` — JSON key of the resource (e.g. `core:gold`).
- param `scope`: `string?` — Mod from which configuration the name was taken, see `scopeOf`. Defaults to the mod of the calling script.

- returns [`ResourceType`](ResourceType.md)

### getSpellByName

Looks up a spell by its JSON key. Returns nil if not found.

- param `name`: `string` — JSON key of the spell (e.g. `core:magicArrow`).
- param `scope`: `string?` — Mod from which configuration the name was taken, see `scopeOf`. Defaults to the mod of the calling script.

- returns [`Spell`](Spell.md)

### getSecondarySkillByName

Looks up a secondary skill by its JSON key. Returns nil if not found.

- param `name`: `string` — JSON key of the secondary skill (e.g. `core:wisdom`).
- param `scope`: `string?` — Mod from which configuration the name was taken, see `scopeOf`. Defaults to the mod of the calling script.

- returns [`Skill`](Skill.md)

### getSpellSchoolByName

Looks up a spell school by its JSON key. Returns nil if not found.

- param `name`: `string` — JSON key of the spell school (e.g. `core:fire`).
- param `scope`: `string?` — Mod from which configuration the name was taken, see `scopeOf`. Defaults to the mod of the calling script.

- returns [`SpellSchool`](SpellSchool.md)
