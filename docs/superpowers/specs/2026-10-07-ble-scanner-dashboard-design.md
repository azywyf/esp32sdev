# BLE Scanner + Web Dashboard — Design

Date: 2026-10-07
Status: Approved by user, ready for implementation plan.

## Purpose

Replace the current BLE GATT server demo (`src/main.cpp`) with a BLE
scanner that lists nearby BLE-advertising devices, highlights a
configured allowlist of "known" devices, and serves the results as a
live web dashboard over WiFi — with Start/Stop controls for the scan.

Inspired by gromeck/BLE-Scanner (ESP32 BLE presence scanner that
reports to MQTT), adapted to use a self-hosted web page instead of
MQTT, since there's no MQTT broker already running in this setup.

## Architecture / data flow

```
[Nearby BLE devices] --advertise--> [ESP32: BLE scan loop]
                                         |
                                         v
                              in-memory device list
                              (MAC, name, RSSI, last-seen,
                               known/unknown flag)
                                         |
                       +-----------------+------------------+
                       v                                    v
              [WiFi: WebServer.h]                   [Serial log (debug)]
                       |
                       v
         Browser polls http://<esp32-ip>/  (auto-refresh every ~3s)
         renders table: Name | MAC | RSSI | Last seen | Known?
```

- ESP32 connects to WiFi in STA mode using credentials from a
  gitignored `secrets.h`.
- A periodic BLE scan (Arduino `BLEDevice` scan API) updates an
  in-memory map of seen devices, keyed by MAC address, each with last
  RSSI and last-seen timestamp.
- A known-devices allowlist (MAC -> friendly label) is checked on each
  sighting; matches are flagged/labeled in the list.
- Devices not seen for a timeout (default 30s) age out of the list —
  but **only while scanning is active** (see Start/Stop below).
- The web server serves a single HTML page listing current devices,
  known ones highlighted, auto-refreshing via
  `<meta http-equiv="refresh">` (no JS required).

## Start/Stop control

```
Browser page has [Start] [Stop] buttons
        |
        v
GET /start  -> sets scanning = true  -> BLE scan loop runs, device list updates
GET /stop   -> sets scanning = false -> BLE scan loop paused, device list frozen
```

- Page shows current status ("Scanning: ON" / "Scanning: OFF") next
  to the device table.
- While stopped: scanning is paused AND the age-out timer is paused,
  so the list stays frozen at its last state rather than draining to
  empty.
- Buttons are plain links/forms to `/start` and `/stop` — no JS
  needed, consistent with keeping the project dependency-free.
- Scanning defaults to **ON** at boot (plug-in-and-it-works).

## Components (files)

- `src/main.cpp` — setup/loop: WiFi connect, start web server, BLE
  scan logic, start/stop state, route handlers (`/`, `/start`,
  `/stop`).
- `src/known_devices.h` — allowlist array, e.g.
  `{ "AA:BB:CC:DD:EE:FF", "My Phone" }`. Committed to git (MACs +
  labels only, not secret).
- `src/secrets.h` (gitignored) + `src/secrets.h.example` (committed
  template) — WiFi SSID/password.
- `src/webpage.h` — the HTML page as a raw string constant (table +
  Start/Stop buttons + meta-refresh), kept separate from `main.cpp`.

## Web server choice

Built-in Arduino-ESP32 `WebServer.h` (synchronous), not
`ESPAsyncWebServer`. Reasoning: single board, single viewer at a time,
short BLE scan bursts — no real concurrency pressure, and avoids
pulling in a third-party dependency for a personal/learning project.

## Error handling

- **WiFi connect fails/drops:** retry with backoff in
  `setup()`/`loop()`; log to Serial. No fallback AP mode — dashboard
  is simply unreachable until WiFi is back, which is acceptable for
  this personal project.
- **BLE scan errors:** scan API is callback-based; if a scan start
  fails, log to Serial and retry next loop iteration.
- **Device list overflow:** cap the in-memory list (default 30
  devices); when full, drop the oldest/weakest-RSSI entry before
  adding a new one.
- **Start/stop state across reboot:** not persisted — always boots
  with scanning ON.

## Testing (manual/hardware, no unit test framework in play)

1. `pio run` — build succeeds.
2. `pio run -t upload` — flashes to the board (COM3).
3. Confirm WiFi connects (Serial log prints assigned IP).
4. Visit `http://<ip>/` — table renders, auto-refreshes, a known
   device (e.g. phone with BLE on) shows labeled.
5. Click **Stop** — list freezes (RSSI/last-seen stop updating).
   Click **Start** — resumes updating.
6. Move a device out of BLE range while scanning is ON — confirm it
   ages out of the list after the timeout.

## Out of scope (explicitly not building)

- MQTT reporting (no broker available in this setup).
- Async web server / multiple simultaneous dashboard viewers.
- Persistent storage of scan history or start/stop state.
- Fallback WiFi AP mode if STA connection fails.
