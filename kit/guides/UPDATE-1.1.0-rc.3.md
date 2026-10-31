# NOVE v1.1.0-rc.3 — BLE recovery after Mac sleep

Mechanical V29 · Studio 1.1.0-rc.2 · firmware studio-1.1.0-ble-recovery.8.

## Changes
Fixes the observed WAIT condition where the Mac stays connected and encrypted but HID suspension remains active. When encryption and both HID subscriptions are valid, the first physical key/encoder gesture clears the stale suspension. That gesture is consumed; subsequent gestures execute macros. Pending macros are cleared. No manual Bluetooth disconnection, bond reset or power cycle is required.

Recovery.8 retains the same runtime logic as recovery.7, with a new version identity and compiler path mapping to remove the owner's personal build paths. BLE/OLED diagnostic instrumentation and 50 kHz I²C remain included. The untested OLED recovery candidate is excluded. The earlier intermittent OLED initialization issue is not claimed permanently fixed.

## Validation
On 24 October, recovery.7's trace captured HID suspend, local-input recovery and ready=true without reboot or reconnection; the owner confirmed LEDs and macros and subsequently reported several days of successful use.
On recovery.8 the owner confirmed both local OLED/LED standby/wake and Mac sleep/wake with working macros. The subsequent trace shows over 10 hours of uptime, unchanged boot ID, one connection, zero disconnects, zero BLE notification errors and zero OLED I²C errors; profiles match exactly. No HID suspend command occurred in that recovery.8 session, so the fallback itself was not retriggered in this cycle. The implementation matches the previously tested recovery.7 logic.
Firmware sources build successfully; native tests and Studio's 26 tests, TypeScript and production build pass. Studio files, STL, seven 3MF profiles and gallery remain unchanged. Windows/Linux and installation on a blank board remain unvalidated.

## Update / Aggiornamento
Export your profile in Studio, then disconnect its serial connection. From kit/software/firmware run `python3 flash.py --port YOUR_PORT --mode update` to preview; add `--write` to install. Install esptool==4.5.1 if needed. Update writes only application at 0x10000. Do not erase flash or use first-install mode on an existing device. Profile and pairing preservation was verified with the supplied partition layout.
IT: Esporta il profilo e disconnetti Studio dalla seriale. Usa `--mode update`: scrive solo l’applicazione e conserva profili e abbinamenti con la mappa partizioni fornita. Non cancellare la flash. Il primo gesto dopo una sospensione BLE bloccata serve soltanto al risveglio; i successivi eseguono le macro.

Firmware SHA-256: 37de048f909fc1d9898f450e8b6a5f3fa59078806963955a824e5ce6980dfd33.

## Licence
Original software: PolyForm Noncommercial 1.0.0 plus the NOVE internal-productivity permission. You may use your own MacroPad for professional work. Sale, commercial distribution and exploitation require written permission. Models and original documentation/images: CC BY-NC 4.0. Third-party licences remain intact. Source-available software, not OSI open source.
IT: Vendita e sfruttamento commerciale richiedono autorizzazione scritta. È consentito usare il proprio dispositivo nel lavoro quotidiano secondo il permesso aggiuntivo NOVE; modelli condivisibili e modificabili per usi non commerciali con attribuzione.
