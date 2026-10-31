# Marauder Studio companion maintenance

This is P1tt1cus's maintained companion fork. Read ReadMe.md before making changes.

- `origin`: https://github.com/P1tt1cus/flipperzero-wifi-marauder
- `upstream`: https://github.com/0xchocolate/flipperzero-wifi-marauder
- Maintained/default branch: `studio`. Feature branches start here; target PRs here.
- Initial upstream base: `13e93181f0d515fcb858cdec555cedc5364c45ea` (companion v0.8.0).
- Paired ESP32 fork: https://github.com/P1tt1cus/ESP32Marauder
- Paired Flipper firmware fork: https://github.com/P1tt1cus/Momentum-Firmware
- Hardware profile: Flipper f7/API 87.1 and ZY-ESP32 (ESP32-D0WD-V3, 4 MB,
  STUDIO_ZY_ESP32, UART0 at 115200). No GPS, ESP SD, RGB LED,
  attached screen, battery monitor or PSRAM. Diagnostics requires a successful
  profile/capability reply; legacy LDDB firmware keeps it hidden.
- Build using the pinned SDK/tooling in .github/workflows/build.yml. Run
  `python3 tests/test_terminal.py`, `python3 tests/test_studio_view.py` and `ufbt` for relevant app changes.
- Preserve GPL licensing, upstream attribution, shared data formats, and the
  side-by-side `marauder_studio` app ID unless a task calls for migration.
- Fetch upstream changes and review them on a separate integration branch.
  Do not reset `studio` to upstream or automatically merge upstream releases.
- Never commit secrets, Wi-Fi credentials, captures, device dumps, or local build
  environments. Build artifacts belong in CI artifacts/releases.
- App launch alone does not verify UART communication. Use Check link and record
  actual tests. Keep software-test results separate from hardware verification.

## Connection feedback (Studio 1.2)

Automatic startup Check link uses the existing protocol query. Home/console show
last meaningful reply age; echoes and bare prompts never establish link health.
Read `studio/command_status.c` and the README before changing deadlines or retry
policy. Hold-OK recovery is available only after timeout/quiet state; it replays
an exact allowlist of read-only queries or switches to Check link. It never
replays scripts or mutations. Silence is not proof of completion/disconnection.
The query prompt heuristic applies only to synchronous allowlisted commands.
The installed v1.20.0 firmware lacks UART completion messages for some scans.

## Australian radio status (Studio 1.3)

Device → Radio profile requires the `radio-au` capability from our profile.
Read-only machine reports validate actual country, channel range/current channel,
TX maximum and errors; an inactive radio is not AU-confirmed. Run
`python3 tests/test_radio_report.py` along with existing checks. See ReadMe.md for
installed hashes and hardware evidence. Paired ESP32 `studio.2-au` is installed;
direct USB AU checks and GPIO Radio profile/Diagnostics checks pass.

## Session browser (Studio 1.4)

Sessions and the log viewer use the app-owned FileBrowser view, with selection
queued to the scene manager. Keep browser worker start/stop balanced and avoid
blocking dialog calls in these paths. Back from a viewer's browser must preserve
the previously opened file/page; selecting a new log resets to page one and
returns to the existing viewer scene. Run `python3 tests/test_session_browser.py`
alongside the other host checks and API 87.1 build. Read the README for the
reported freeze, physical recovery and scope of on-device verification.
