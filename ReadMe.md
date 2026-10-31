# Marauder Studio

The maintained [P1tt1cus companion fork](https://github.com/P1tt1cus/flipperzero-wifi-marauder), based on upstream
commit `13e93181f0d515fcb858cdec555cedc5364c45ea` (v0.8.0). GPL-3.0;
upstream notices and the command/script engine are retained below.

## Maintained repositories

Our default development branch is `studio`. `origin` points to this fork and
`upstream` points to the original project. Changes to the UI are maintained
alongside [ESP32 firmware](https://github.com/P1tt1cus/ESP32Marauder) and
[Flipper firmware](https://github.com/P1tt1cus/Momentum-Firmware).
Review upstream changes on an integration branch before merging. The build
workflow targets our API 87.1 profile and publishes the FAP as a CI artifact.

## Use

Open **Apps → GPIO → Marauder Studio**. This is a separate FAP; the original
`ESP/esp32_wifi_marauder.fap` remains installed.

- **Discover:** nearby-device discovery, result lists, inspection, and channel controls.
- **Sessions:** browse saved logs, PCAP sizes, text dumps, or the last console buffer.
- **Tools:** additional upstream commands.
- **Scripts:** the existing workflow editor and runner.
- **Device → Check link:** requests a version/profile reply and shows a readable summary.
- **Device → Diagnostics:** appears after Check link confirms support in Studio firmware;
  reports chip, flash, memory, reset and scan state. Scroll for the full report.
- **Settings:** immediate log/capture saving toggles and control/board notes.

In the console, **OK** holds the display or follows new output. **Up/Down** scroll
one line; **Left/Right** scroll four. **Back** returns and sends `stopscan` for
continuous commands. Holding the display does not stop recording. `Top` means
the initial page of a result; `Hold` means you paused it. The footer shows bytes
received and `LOG`/`PCAP` when writing those files. An SD write failure is shown
explicitly. The screen retains 160 wrapped lines; the last-output viewer retains
up to 4 KiB. Saved logs can be browsed in 4 KiB pages.

Files and the two save preferences are shared with the original companion under
`/ext/apps_data/marauder/{logs,pcaps,dumps,scripts}`. Browse captures to inspect
size; export through qFlipper for analysis in Wireshark. Flipper SD storage and
an ESP32-attached SD card are separate.

Version **1.1** targets the ZY-ESP32 (original ESP32, 4 MB), with paired firmware
profile `STUDIO_ZY_ESP32` / `studio-zy-esp32`. GPS/wardrive/geofences, ESP SD
list/save/load/backup/update, RGB LED and SD HTML commands are removed from the
menus. Script LED controls are removed; old `enableLed` metadata is preserved
when saving a script but is not sent to this board. Custom script commands for
absent hardware are rejected by the paired firmware.

Wi-Fi 2.4 GHz, BLE, CSI and UART-to-Flipper capture remain available. **HTML from
Flipper** uses serial transfer, not an ESP SD card. `SavePCAP` still controls
capture output and must remain enabled when recording. Existing internal
SPIFFS settings remain supported.

Legacy v1.20.0 replies still work with Check link but do not enable Diagnostics.
Unknown profiles, malformed capability arrays and unsupported protocol versions
cannot enable it. Check link reports a reply at that moment, not continuous
connection status. Hardware filtering is specific to our board; this build is
not a generic auto-configuring companion for other ESP32 boards.

## Build and checks

Target: Flipper f7, API **87.1**, matching Qwerty's Momentum build `d3f89dfe`.
Use the Momentum `mntm-012` SDK (same API); do not update device firmware to build.

```sh
uv tool run --from ufbt ufbt update --hw-target f7 --url https://github.com/Next-Flip/Momentum-Firmware/releases/download/mntm-012/flipper-z-f7-sdk-mntm-012.zip
uv tool run --from ufbt ufbt
python3 tests/test_terminal.py
```

Output: `dist/marauder_studio.fap`. Host tests use AddressSanitizer and
UndefinedBehaviorSanitizer for fragmented terminal escapes, wrapping, history
eviction, binary noise, and validated/malformed version replies.

Previous v1.0 on-device checks on Qwerty (2026-10-31): app launch, home-screen fit,
command navigation and Back, console hold/scroll controls, and a successful
Marauder v1.20.0 protocol reply. The same reply was read back from
`logs/protocolinfo_3.log` (246 bytes). The installed FAP MD5 matches the local
build (`988ffc34993c64bde36af43c32c7fdb3`); the original companion still matches
its pre-change MD5 (`1186772e48c9b86b1356e8e22e169235`). New RF capture runs,
all upstream commands, and SD failure conditions were not exercised.

The UI uses the device's two-colour canvas (#000000/#FFFFFF), native primary
and secondary labels, and a fixed-width console. The design review favoured
six short task names over a miniature desktop dashboard: at 128×64, navigation
and four readable output rows matter more than decoration. UART callbacks use
a locking view model, callback replacement waits for in-flight work, and IRQs
stop before RX buffers are freed. Preferences write exactly one byte.

---

## Upstream project documentation

[![FAP Build](https://github.com/0xchocolate/flipperzero-wifi-marauder/actions/workflows/build.yml/badge.svg)](https://github.com/0xchocolate/flipperzero-wifi-marauder/actions/workflows/build.yml)

# WiFi Marauder companion app for Flipper Zero

Requires a connected dev board running Marauder FW. [See install instructions from UberGuidoZ here.](https://github.com/UberGuidoZ/Flipper/tree/main/Wifi_DevBoard#marauder-install-information)

<img src="https://github.com/0xchocolate/flipperzero-wifi-marauder/blob/feature_wifi_marauder_app/screenshots/marauder-topmenu.png?raw=true" width=20% height=20% /> <img src="https://github.com/0xchocolate/flipperzero-wifi-marauder/blob/feature_wifi_marauder_app/screenshots/marauder-script-demo.png?raw=true" width=20% height=20% /> <img src="https://github.com/0xchocolate/flipperzero-wifi-marauder/blob/feature_wifi_marauder_app/screenshots/marauder-save-pcaps.png?raw=true" width=20% height=20% />

## Get the app
1. Make sure you're logged in with a github account (otherwise the downloads in step 2 won't work)
2. Navigate to the [FAP Build](https://github.com/0xchocolate/flipperzero-wifi-marauder/actions/workflows/build.yml)
   GitHub action workflow, and select the most recent run, scroll down to artifacts.
3. The FAP is built for the `dev` and `release` channels of both official and unleashed
   firmware. Download the artifact corresponding to your firmware version.
4. (Optional step to avoid confusion) Go to "Apps/GPIO" on the Flipper SD Card, delete any existing Marauder app, on some firmwares there will be a `ESP32CAM_Marauder.fap` or similar.
5. Extract `esp32_wifi_marauder.fap` from the ZIP file downloaded in step 3 to your Flipper Zero SD card, preferably under Apps/GPIO along with the rest of the GPIO apps. (If you're using qFlipper to transfer files you need to extract the content of the ZIP file to your computer before you drag it to qFlipper, as qFlipper does not support direct dragging from a ZIP file (at least on Windows)).

From a local clone of this repo, you can also build the app yourself using ufbt.

### FYI - the ESP flasher is now its own app: https://github.com/0xchocolate/flipperzero-esp-flasher


## Support

For app feedback, bugs, and feature requests, please [create an issue here](https://github.com/0xchocolate/flipperzero-firmware-with-wifi-marauder-companion/issues).

You can find me (0xchocolate) on discord as @cococode.

If you'd like to donate to the app development effort:

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/O4O1R7X6K)

**ETH**: `0xf32A1F0CD6122C97d8953183E53cB889cc087C9b`  
**BTC**: `bc1qtw7s25cwdkuaups22yna8sttfxn0usm2f35wc3`

Find more info about Marauder and support its developer (justcallmekoko aka WillStunForFood) here: https://github.com/justcallmekoko/ESP32Marauder

If you found the app preinstalled in a firmware release, consider supporting the maintainers!
