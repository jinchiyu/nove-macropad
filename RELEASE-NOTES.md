# NOVE MacroPad — V29 / release candidate 1.1.0-rc.2

Release candidate package: NOVE Studio 1.1.0-rc.2, firmware 1.1.0-rc.1, mechanical revision V29.

- Studio: faithful CAD 3D view, simple-view fallback and Light/Dark/System themes.
- Validated USB transfers and draft preservation when saving has an uncertain outcome; explicit readback required before retrying.
- K1C PETG project with six plates, seven 3MF files including the combined project, STL files, bilingual build guide, firmware, Studio and dependency notices.
- Hardware recheck: all nine keys, encoder in both directions, click and hold; OLED/LEDs, a real macro and layer/colour change confirmed. Three layers preserved. No new firmware flashed during the recheck.
- Software checks: 26 tests, TypeScript/Studio build, native and sanitizer tests, firmware build and browser resilience checks. Packaging verifies checksums and unchanged print profiles.

The firmware binary remains the hardware-tested build from commit e1a2e88 (SHA-256 a03a094de48fb378dcc650ed16936d7c68064aa329e21caecd73ea616ae693b3). Formatting-only C++ changes are included as sources; the newly compiled binary is not distributed.

macOS validated. Windows/Linux and first installation on a blank board are not validated. Personal profiles, pairing data and editable original CAD are excluded from the archive.

## Licences / Licenze

Original software: PolyForm Noncommercial 1.0.0 with an additional NOVE permission for internal productivity, including daily professional use of your own MacroPad. Selling, commercially distributing or incorporating the software in commercial products/services requires written permission. Third-party licences remain unchanged.

Models, original documentation and gallery: CC BY-NC 4.0. Noncommercial sharing and remixing with attribution are allowed; selling files or printed copies requires permission. See LICENSE.txt in the repository and kit/licenses/NOVE-LICENSE.txt in the download for the applicable terms.

## Downloads

Download **NOVE-V29-1.1.0-rc.2-kit.zip**, verify it using **SHA256SUMS.txt**, then open `kit/START-HERE.txt`. The archive includes printable files, firmware, Studio, sources, dependency notices and the bilingual guide. This is a prerelease; no stable v1.0.0 release is claimed.

IT: scarica lo ZIP, controlla SHA256SUMS.txt e apri kit/START-HERE.txt. Uso non commerciale e produttività interna nei termini indicati; vendita e sfruttamento commerciale richiedono autorizzazione.
