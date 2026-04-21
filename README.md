# FLowlab

A lightweight Windows network traffic controller. Bind keyboard shortcuts to rules that block or rate-limit traffic on specific ports, toggleable at runtime with no GUI.

Built on [WinDivert](https://reqrypt.org/windivert.html).

---

## What it does

- Intercepts packets at the network layer using WinDivert
- Each rule covers a port range, a protocol (TCP/UDP/both), and a direction (upload/download/both)
- Rules are toggled on/off with a keybind — pressing the same key again toggles it back off
- Actions: hard block or rate-limit at one of 14 configurable speeds
- Everything is configured in a single plaintext file loaded at startup — no changes take effect at runtime

---

## Requirements

- Windows 10 or later (x64)
- [WinDivert 2.x](https://reqrypt.org/windivert.html) — download the release package
- MSVC (Visual Studio build tools or full IDE, C11)
- Administrator privileges at runtime (WinDivert requires it)

---

## Building

1. Download WinDivert and extract it somewhere, e.g. `D:\WinDivert\`

2. From a Visual Studio developer command prompt, run:

```
cl /std:c11 /W4 /O2 /permissive- ^
   /I include ^
   /I D:\WinDivert\include ^
   /Fo:build\obj\ ^
   /Fe:build\app.exe ^
   src\main.c src\hook.c src\keybinds.c src\config_loader.c ^
   src\parser.c src\dispatcher.c src\statetable.c src\divert.c src\limiter.c ^
   user32.lib ^
   /link /LIBPATH:D:\WinDivert\x64 WinDivert.lib ws2_32.lib
```

Adjust the WinDivert path to match your setup. The `build\obj\` directory must exist before compiling.

3. Copy `WinDivert.dll` and `WinDivert.sys` from the WinDivert package into the same directory as `app.exe`.

---

## Running

Must be run as Administrator. WinDivert installs a kernel driver to intercept packets and will fail without elevated privileges.

```
app.exe
```

The program looks for `config.txt` in the working directory. If it cannot open the file or finds a parse error, it prints the offending line number and exits.

Press `Ctrl+C` once to shut down cleanly.

---

## Config file

`config.txt` is loaded once at startup. Changes require a restart.

### Structure

```
[header lines]
[rule lines]
```

Header lines must appear before any rule lines. Their order relative to each other does not matter. Rule lines follow after.

### Header lines

#### `policy=`

Controls which port is used to match packets to rules.

```
policy=remote
policy=local
```

- `remote` — match on the remote port (the server's port). Use this for client-side traffic shaping, e.g. limiting a game server's port.
- `local` — match on the local port (your machine's port).

Default if absent: `remote`.

#### `speed1=` through `speed14=`

Set the bandwidth limit in bytes per second for each of the 14 limit levels. Used by `LIMIT1`–`LIMIT14` actions in rule lines.

```
speed1=8192
speed2=65536
speed3=524288
```

- Values are in bytes per second (not bits)
- Any level not specified defaults to `1` byte/sec (effectively a block)
- Speeds are independent — each LIMIT level can have a different rate

### Rule lines

Each rule is one line with exactly 6 comma-separated fields:

```
KEYBIND, PORT_START, PORT_END, PROTOCOL, DIRECTION, ACTION
```

Whitespace around commas is ignored. Fields are case-insensitive.

#### Field 1 — KEYBIND

The keyboard shortcut that toggles this rule. Format:

```
[MODIFIER+]KEY
```

Supported modifiers: `CTRL`, `ALT`, `SHIFT`, `WIN`

The final character must be a single printable ASCII character (letters, digits, symbols). Letters are case-insensitive — `A` and `a` are the same key.

Examples:
```
ALT+1
CTRL+SHIFT+G
WIN+F
Q
```

Modifier order does not matter. Multiple modifiers are supported.

**Limitation:** only single character final keys are supported. F-keys, arrow keys, numpad, and other special keys are not supported.

#### Field 2 & 3 — PORT_START and PORT_END

The port range this rule applies to, inclusive. Both must be valid port numbers (0–65535) and `PORT_START` must be ≤ `PORT_END`.

For a single port: set both to the same value.

```
8000, 8000      # single port
2015, 3015      # range
```

#### Field 4 — PROTOCOL

```
TCP
UDP
ALL
```

`ALL` matches both TCP and UDP.

#### Field 5 — DIRECTION

```
UL       # upload (outbound from your machine)
DL       # download (inbound to your machine)
ALL      # both directions
```

#### Field 6 — ACTION

```
BLOCK          # drop all matching packets
LIMIT1         # rate-limit at speed1 bytes/sec
LIMIT2         # rate-limit at speed2 bytes/sec
...
LIMIT14        # rate-limit at speed14 bytes/sec
```

Rate limiting uses a token bucket. Packets that exceed the budget are dropped. TCP will naturally retransmit dropped packets, producing the effect of a bandwidth cap. UDP packets that are dropped are gone.

---

## Full example config

```
policy=remote
speed1=8192
speed2=65536
speed3=1048576

ALT+1, 1400, 1400, ALL,  DL,    LIMIT1
ALT+2, 2800, 2800, ALL,  UL,    LIMIT2
ALT+3, 430, 450, TCP,  ALL,   LIMIT3
ALT+4, 9000, 9090, ALL, DL,   BLOCK
ALT+5, 5460, 8009, ALL, UL,   LIMIT1
```

- `ALT+1` toggles download throttle on port 1400 at 8 KB/s
- `ALT+2` toggles upload throttle on port 2800 at 64 KB/s
- `ALT+3` toggles TCP throttle both directions on ports 430–450 at 1 MB/s
- `ALT+4` toggles a hard download block on ports 9000–9090
- `ALT+5` toggles upload throttle on ports 5460–8009 at 8 KB/s

---

## Limitations

**Max 64 rules.** Defined by `MAX_RULES` in `keybinds.h`. Recompile to increase.

**Max 16 keybinds per modifier combination.** Each modifier bitmask gets a bucket of 16 entries. If you put more than 16 rules under the same modifier (e.g. all under `ALT+`), the extras are silently ignored.

**Single character final keys only.** F-keys and special keys are not supported as keybind targets.

**Shared port conflict.** If two rules cover the same port, the state table entry for that port is a single byte. Toggling one rule overwrites the other's state. Avoid overlapping port ranges across rules.

**IPv6 fragmented packets are forwarded without inspection.** Non-first IPv6 fragments (fragment extension header with a non-zero offset) are passed through unconditionally, as they do not carry port headers.

**IPv4 fragmented packets are forwarded without inspection.** Only the first fragment carries port headers; subsequent fragments (non-zero fragment offset) are passed through unconditionally.

**No runtime config reload.** Restart the process to apply config changes.

**Rate limits apply per port range, not per connection.** All connections on the matched port range share the same token bucket.

**Requires administrator.** WinDivert operates at kernel level and cannot run without elevation.

---

## Architecture overview

```
config.txt
    │
    ▼
config_loader      — parses header directives and rule lines
    │
    ├─► keybinds   — builds a hash-bucketed lookup table (mod → vk → rule)
    ├─► statetable — allocates g_state_table[65536], one byte per port
    └─► divert     — builds WinDivert filter string, opens handle, spawns thread
            │
            ├─► limiter    — initialises token buckets for LIMIT rules
            └─► divert_loop (thread)
                    │
                    ├── WinDivertRecv
                    ├── IP/TCP/UDP parsing (IPv4 and IPv6)
                    ├── state table lookup
                    ├── action dispatch (forward / block / limit)
                    └── WinDivertSend

hook (main thread)
    └── LowLevelKeyboardProc
            └── keybind_lookup → dispatch_keybind → toggle state table entry
```

The divert thread and the keyboard hook both access `g_state_table`. The hook writes a single byte per toggle; the divert thread reads a single byte per packet. On x86-64 byte-sized reads and writes are atomic at the hardware level, so no explicit locking is used. This is a documented assumption — if ported to other architectures, a mutex or atomic should be added.

---

## License

This project uses WinDivert which is licensed under LGPL v3. Your use of WinDivert is subject to its license terms. See [https://reqrypt.org/windivert.html](https://reqrypt.org/windivert.html).