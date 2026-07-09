# Windows Hook MVP

This version is a temporary Windows global keyboard hook before the real TSF IME.

Run `BulgarianIME.exe` or `BulgarianIME_DebugConsole.exe`.

Do **not** run `bgime-cli.exe` if you expect system-wide typing conversion. The CLI only converts text you pass to it.

## Test

1. Start `BulgarianIME_DebugConsole.exe`.
2. Leave the console open.
3. Open Notepad.
4. Type `zdravey az sım sh ch zh sht`.
5. Expected: `здравей аз съм ш ч ж щ`.

## If it does not work

- Make sure you are running `BulgarianIME_DebugConsole.exe` or `BulgarianIME.exe`, not `bgime-cli.exe`.
- If your target app runs as Administrator, run BulgarianIME as Administrator too.
- Do not test in password fields, games, or apps with anti-cheat.
- Start with Notepad.
