# ActionPreview

Cursor and status bar text that the client shows for an option of a battle action on the hovered hex. Returned by the `preview` function of a battle action script.

### cursor

Identifier of the cursor from `config/cursors.json`.

- type: `string`

### text

Status bar text.

- type: [`MetaString`](MetaString.md)

### healthGain

If above zero, appended to `text` in brackets with the plural forms of `healthGainTextID`.

- type: `integer`

### healthGainTextID

Base text ID of the health gain, such as `vcmi.battleWindow.healValuePreview`.

- type: `string`
