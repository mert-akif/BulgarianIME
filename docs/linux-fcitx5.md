# Linux support: Fcitx5

BulgarianIME uses a native Fcitx5 input-method addon on Linux.

This is deliberately different from the Windows hook implementation:

- normal converted characters are committed directly to the application;
- only an ambiguous suffix is kept in Fcitx preedit (`s`, `sh`, `ş`, etc.);
- when the sequence becomes final, the preedit is committed;
- Fcitx owns focus/preedit lifetime, so clicking or changing applications does
  not leave a process-wide stale keyboard buffer behind;
- the same engine is usable through Fcitx on X11 and Wayland.

## Ubuntu dependencies

```bash
sudo apt update
sudo apt install -y \
  build-essential cmake ninja-build \
  fcitx5 fcitx5-config-qt fcitx5-frontend-all \
  libfcitx5core-dev
```

Ubuntu 22.04 and 24.04 both provide `libfcitx5core-dev`.

## Build and install

From the repository root:

```bash
cmake -S . -B out/build/linux-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DBGIME_BUILD_FCITX5=ON

cmake --build out/build/linux-release
sudo cmake --install out/build/linux-release
fcitx5 -rd
```

Or use:

```bash
./linux-fcitx5/install-ubuntu.sh
```

## First Fcitx5 setup on Ubuntu

Run:

```bash
im-config
```

Select **fcitx5**, then log out and log back in.

After logging in:

```bash
fcitx5-configtool
```

Add **BulgarianIME** to the active input methods.

## Test

Open a normal text field and type:

```text
zdravey az sım şt
```

Expected output:

```text
здравей аз съм щ
```

## Useful diagnostics

Session type:

```bash
echo $XDG_SESSION_TYPE
```

Fcitx diagnostic report:

```bash
fcitx5-diagnose
```

Check that the addon was installed:

```bash
find /usr/lib -path '*/fcitx5/bulgarianime.so' -print
cat /usr/share/fcitx5/inputmethod/bulgarianime.conf
```

If BulgarianIME does not appear in `fcitx5-configtool`, restart Fcitx:

```bash
fcitx5 -rd
```

or log out and back in.
