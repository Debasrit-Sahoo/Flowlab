# Flowlab



> Instant, per-port network control for Windows



Bind keyboard shortcuts to rules that block or rate-limit traffic on specific ports, and toggle them instantly at runtime. No GUI, no background services, no system-wide configuration changes.



Built on [WinDivert](https://reqrypt.org/windivert.html).



---



## Why This Exists



Modern tools for controlling network traffic are either:



- **Too heavy** — full GUI apps running constantly

- **Too static** — firewall rules that require setup and teardown

- **Too coarse** — system-wide throttling with no precision



Flowlab is designed for fast, targeted control. You define rules once, then toggle them on and off with a keypress. That's it.



---



## Use Cases



- Simulate poor network conditions for testing

- Throttle specific game or service ports on demand

- Temporarily block traffic without touching firewall settings

- Debug bandwidth-sensitive applications

- Reproduce edge cases like packet drops or slow links



---



## Features



- Packet interception using WinDivert (kernel-level)

- Per-rule keybind toggling (instant enable/disable)

- Supports TCP, UDP, or both

- Direction-aware rules (upload, download, or both)

- Hard blocking or rate limiting (14 configurable levels)

- Simple plaintext configuration

- Minimal overhead from inactive rules

- No GUI, no background service



---



## How It Works



1\. A config file defines rules (port range, protocol, direction, action)

2\. Each rule is mapped to a keyboard shortcut

3\. A background thread intercepts packets using WinDivert

4\. A cache-friendly global state table (1 byte per port)

5\. Matching packets are either forwarded, dropped, or rate-limited



Keybinds toggle rules by flipping a single byte in a shared state table, allowing lock-free interaction between threads.



---



## Requirements



- Windows 10 or later (x64)

- WinDivert 2.x

- MSVC (Visual Studio Build Tools or full IDE)

- Administrator privileges (required by WinDivert)



---



## Build



1\. Download and extract WinDivert (e.g. `D:\\WinDivert\\`)



2\. From a **Visual Studio Developer Command Prompt**:



```bat

cl /std:c11 /W4 /O2 /permissive- ^

&#x20;  /I include ^

&#x20;  /I D:\\WinDivert\\include ^

&#x20;  /Fo:build\\obj\\ ^

&#x20;  /Fe:build\\app.exe ^

&#x20;  src\\main.c src\\hook.c src\\keybinds.c src\\config\_loader.c ^

&#x20;  src\\parser.c src\\dispatcher.c src\\statetable.c src\\divert.c src\\limiter.c ^

&#x20;  user32.lib ^

&#x20;  /link /LIBPATH:D:\\WinDivert\\x64 WinDivert.lib ws2\_32.lib

```



3\. Copy `WinDivert.dll` and `WinDivert.sys` into the output directory.



---



## Installation



1\. Grab a build of `app.exe` (or build it yourself — see [Build](#build))

2\. Download [WinDivert 2.x](https://reqrypt.org/windivert.html) and extract it

3\. Copy `WinDivert.dll` and `WinDivert.sys` into the same folder as `app.exe`

4\. Place your `config.txt` in that same folder



Your directory should look like this:



```

flowlab/

├── app.exe

├── config.txt

├── WinDivert.dll

└── WinDivert.sys

```



---



## Run



Flowlab uses WinDivert to intercept packets at the kernel level, which **requires Administrator privileges**. It will not function without elevation.



**Option 1 — Right-click → Run as administrator** on `app.exe`



**Option 2 — From an elevated command prompt:**



```bat

app.exe

```



> **Tip:** To always run elevated, right-click `app.exe` → Properties → Compatibility → check *Run this program as an administrator*.



- Reads `config.txt` from the working directory

- Invalid config → prints error + line number, then exits

- Press `Ctrl+C` to shut down cleanly



---



## Troubleshooting



### `Access is denied` or `ERROR\_ACCESS\_DENIED`



Flowlab was not launched with Administrator privileges. Right-click `app.exe` and select **Run as administrator**, or launch it from an elevated command prompt.



---



### `The system cannot find the file specified` / `WinDivert.dll not found`



`WinDivert.dll` and `WinDivert.sys` are missing from the application directory. Copy both files from your WinDivert 2.x download into the same folder as `app.exe`.



---



### `This version of WinDivert requires...` / driver version mismatch



You have a mismatched WinDivert version. Make sure `WinDivert.dll` and `WinDivert.sys` are both from the **same WinDivert 2.x release** — do not mix files from different versions.



---



### `The driver is blocked from loading` / `Code integrity` error



Windows is blocking the WinDivert kernel driver due to driver signature enforcement. Try:



- Ensure you're using an **official WinDivert release** (signed driver)

- Check that Secure Boot settings are not blocking third-party drivers

- On Windows 11: some builds may require test signing mode — see WinDivert's documentation



---



### Packets are not being intercepted / rules have no effect



- Confirm Flowlab is running as Administrator

- Verify `WinDivert.sys` is present alongside `WinDivert.dll`

- Check that your `config.txt` rules match the correct port, protocol, and direction

- Make sure the target application traffic actually uses the port range specified in your rule



---



### Config error on startup



Flowlab prints the offending line number and exits. Open `config.txt`, go to the reported line, and check for typos in the keybind, port range, protocol, direction, or action fields.



---



## Configuration



All behavior is defined in a single file: `config.txt`



### Structure



```

[header lines]

[rule lines]

```



---



### Header Options



#### Policy



```ini

policy=remote

policy=local

```



| Value    | Behavior                                      |

|----------|-----------------------------------------------|

| `remote` | Match on destination/source port (typical use)|

| `local`  | Match on your machine's port                  |



**Default:** `remote`



#### Rate Limits



```ini

speed1=8192

speed2=65536

...

```



- Values are in **bytes per second**

- Used by `LIMIT1` through `LIMIT14`

- Unspecified levels default to **1 byte/sec** (effectively blocked)



---



### Rule Format



```

KEYBIND, PORT\_START, PORT\_END, PROTOCOL, DIRECTION, ACTION

```



**Example:**



```

ALT+1, 1400, 1400, ALL, DL, LIMIT1

```



#### Fields



| Field       | Options / Format                                         |

|-------------|----------------------------------------------------------|

| **Keybind** | `[MODIFIER+]KEY` — Modifiers: `CTRL`, `ALT`, `SHIFT`, `WIN`. Final key must be a single printable character. |

| **Port range** | `8000, 8000` (single) or `2000, 3000` (range)        |

| **Protocol** | `TCP`, `UDP`, `ALL`                                    |

| **Direction** | `UL` (upload), `DL` (download), `ALL`                 |

| **Action**  | `BLOCK` or `LIMIT1` … `LIMIT14`                         |



Rate limiting uses a **token bucket**. Excess packets are dropped.



---



### Example Config



```ini

policy=remote

speed1=8192

speed2=65536

speed3=1048576



ALT+1, 1400, 1400, ALL, DL, LIMIT1

ALT+2, 2800, 2800, ALL, UL, LIMIT2

ALT+3, 430,  450,  TCP, ALL, LIMIT3

ALT+4, 9000, 9090, ALL, DL, BLOCK

```



---



## Design Notes



- **State table:** `uint8\_t g\_state\_table[65536]` — 1 byte per port

- Lock-free access between threads (atomic on x86-64)

- Token bucket limiter per rule

- Packet parsing supports IPv4 and IPv6



---



## Limitations



- Max **64 rules** (compile-time constant)

- Max **16 keybinds** per modifier combination

- Only **single-character keys** supported

- Overlapping port ranges share state (last toggle wins)

- No runtime config reload (restart required)

- Rate limits apply per port range, not per connection

- Fragmented packets (IPv4/IPv6) beyond first fragment are not inspected

- Requires administrator privileges



---



## Why Not Just Use a Firewall or NetLimiter?



| Tool | Problem |

|------|---------|

| Firewalls | Static — not designed for rapid toggling |

| GUI tools (e.g. NetLimiter) | Heavier, always running |

| System-wide throttling | Lacks per-port precision |



Flowlab is built for **fast, targeted, reversible control**.



---



## License



Uses [WinDivert](https://reqrypt.org/windivert.html) (LGPL v3).

