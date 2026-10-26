# Windows hook notes

## Streaming design

The Windows hook does not retain a whole word anymore.

The composer stores only an ambiguous suffix that could still match a longer transliteration rule. All unambiguous mappings are inserted immediately.

Examples:

- `a` -> inserts `а`, no state retained.
- `s` -> inserts `с`, retains raw `s` because `sh`/`sht` are possible.
- `sh` -> replaces only the pending `с` with `ш`, retains raw `sh` because `sht` is possible.
- `sht` -> replaces only the pending `ш` with `щ`, then clears state.
- `sd` -> `с` remains untouched; only `д` is appended and state is cleared.

## Mouse/focus reset

A `WH_MOUSE_LL` hook clears composition state on left/right/middle/X-button clicks.

The reset is intentionally state-only. It must never send Backspace on a click because the click may move the caret to a different location.

Foreground-window changes and cursor/navigation keys also commit the pending state in place.
