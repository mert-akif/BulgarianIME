# BulgarianIME

C++20 Bulgarian Latin/Turkish-Q to Cyrillic input helper.

The current Windows version uses a global keyboard hook so you can type Bulgarian Cyrillic directly in normal text fields (Notepad, WhatsApp, browser text boxes, Discord, etc.).

## v0.2 - streaming composer

The hook no longer buffers and rewrites the whole word.

Normal letters are converted immediately and forgotten. Only the tiny suffix that can still become a multi-key Bulgarian mapping is kept temporarily, for example:

- `s` -> may become `sh` or `sht`
- `sh` -> may become `sht`
- `z` -> may become `zh`
- `c` -> may become `ch`
- `t` -> may become `ts`
- `y` -> may become `ya` or `yu`

This means the pending buffer is normally empty and is at most a couple of Latin characters for the current mapping.

Mouse clicks also clear the pending state **without deleting or rewriting visible text**. This prevents an old partial sequence from being emitted after clicking somewhere else to correct a character.

## Executables

- `BulgarianIME.exe` - background Windows hook app.
- `BulgarianIME_DebugConsole.exe` - same hook with diagnostic console output.
- `bgime-cli.exe` - command-line converter only.

## Windows build

```powershell
cmake -S . -B out/build/x64-Debug
cmake --build out/build/x64-Debug --config Debug
```

Then run:

```powershell
.\out\build\x64-Debug\Debug\BulgarianIME.exe
```

For diagnostics:

```powershell
.\out\build\x64-Debug\Debug\BulgarianIME_DebugConsole.exe
```

## Shortcuts

- `Ctrl + Alt + B` - enable / disable
- `Ctrl + Alt + Q` - quit

## Example

Input:

```text
zdravey az sım sh ch zh sht
```

Output:

```text
здравей аз съм ш ч ж щ
```

## Safety / limitations

This is still a Windows hook MVP, not the final TSF IME. Avoid using it in password fields, games, anti-cheat protected applications, or other security-sensitive software.

## Roadmap

- [x] C++ transliteration core
- [x] CLI converter
- [x] Windows global hook MVP
- [x] Streaming / minimal pending buffer
- [x] Clear composition state on mouse click
- [x] Clear composition state on focus/window changes
- [ ] System tray icon
- [ ] Configurable mappings
- [ ] Windows startup option
- [x] Linux support
- [x] Fcitx5 Linux input method
- [ ] Full Windows TSF IME

## Linux / Ubuntu (Fcitx5)

The Linux version is a real Fcitx5 input-method addon, not a global keyboard
hook. Stable characters are committed immediately; only a tiny ambiguous
suffix such as `s`, `sh`, or `ş` is kept as Fcitx preedit until the next key
makes the transliteration unambiguous.

### One-command build/install

```bash
chmod +x linux-fcitx5/install-ubuntu.sh
./linux-fcitx5/install-ubuntu.sh
```

Then, for the first Fcitx5 setup on Ubuntu:

1. Run `im-config` and select **fcitx5**.
2. Log out and log back in.
3. Run `fcitx5-configtool`.
4. Add **BulgarianIME** to the active input methods.
5. Switch to BulgarianIME and type normally.

Example:

```text
zdravey az sım şt
```

becomes:

```text
здравей аз съм щ
```

The addon is designed for both X11 and Wayland through Fcitx5.
