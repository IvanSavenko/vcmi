# ActionContext

Acting unit and hovered hex of a query about an option of a battle action.

### actor

Unit that would make the action.

- type: [`Unit`](Unit.md)

### hoveredHex

Hex under the cursor.

- type: [`BattleHex`](BattleHex.md)

### partialTarget

Destinations chosen with earlier clicks.

- type: `Destination[]`

### side

Side that would make the action.

- type: [`BattleSide`](BattleSide.md)

### legal

True if clicking the hovered hex makes an action that passes `validate`. Set only for `preview`.

- type: `boolean`
