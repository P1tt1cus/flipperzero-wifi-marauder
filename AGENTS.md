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
  `python3 tests/test_terminal.py` and `ufbt` for relevant app changes.
- Preserve GPL licensing, upstream attribution, shared data formats, and the
  side-by-side `marauder_studio` app ID unless a task calls for migration.
- Fetch upstream changes and review them on a separate integration branch.
  Do not reset `studio` to upstream or automatically merge upstream releases.
- Never commit secrets, Wi-Fi credentials, captures, device dumps, or local build
  environments. Build artifacts belong in CI artifacts/releases.
- App launch alone does not verify UART communication. Use Check link and record
  actual tests. Keep software-test results separate from hardware verification.
