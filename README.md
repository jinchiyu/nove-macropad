# NOVE MacroPad by 99BIT

DIY Bluetooth macropad: nine keys, rotary encoder, OLED and RGB lighting. Mechanical revision V29; NOVE Studio 1.1.0-rc.2; hardware-tested firmware 1.1.0-rc.1.

**[Download the complete kit](https://github.com/jinchiyu/nove-macropad/releases/tag/v1.1.0-rc.2)** — print files, bilingual guide, wiring, firmware and local Studio with CAD 3D view and light/dark/system themes.

Read [START-HERE](kit/START-HERE.txt) and the [build guide](kit/guides/99BIT-build-guide-EN-IT.pdf). This is a soldered DIY build. USB-C supplies power/configuration; keyboard commands use Bluetooth. macOS is validated; Windows/Linux and installation on a blank board are not yet validated. Print profiles target K1C, 0.4 mm nozzle, PETG.

## Source and build

Original firmware source: `kit/software/source` (`pio run -e xiao`). Studio source: `kit/software/studio` (`npm ci`, `npm test`, `npm run build`). Included Studio build starts with `kit/software/Start NOVE Studio.command`; Python 3 and Chrome required. Dependency sources/notices are included. The distributed firmware binary is the hardware-tested build; included C++ source differs only in formatting from its original build source.

## Licence

Original software: [PolyForm Noncommercial 1.0.0](licenses/PolyForm-Noncommercial-1.0.0.md), with the [NOVE internal-productivity permission](LICENSE.txt). You may use your own MacroPad for daily professional work. Commercial distribution, sale and incorporation into commercial products/services require written permission. Source-available, not OSI open source.

Models, original documentation and images: CC BY-NC 4.0, allowing attribution-based noncommercial sharing/remixing. Third-party components retain their own licences; their rights are not restricted by these original-work terms.

Italiano: scarica il kit e consulta la guida bilingue. Sono consentiti condivisione e remix non commerciali con attribuzione. Per il software è consentito anche usare il proprio MacroPad nel lavoro quotidiano. Vendita e sfruttamento commerciale richiedono autorizzazione, secondo i termini completi in LICENSE.txt.
