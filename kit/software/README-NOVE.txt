NOVE Studio 1.1.0-rc.2 / firmware 1.1.0-rc.1 / V29
EN: Start NOVE Studio.command opens the included local build; no Node required. Python 3 and Chrome required. Drag the CAD model to rotate, click controls to configure, or choose Simple view. Theme: Light / Dark / System (Italian UI: Chiaro / Scuro / Sistema), saved locally. No idle auto-rotation; reduced-motion preference respected.
IT: Start NOVE Studio.command apre la build locale inclusa; non serve Node. Servono Python 3 e Chrome. Trascina il CAD per ruotarlo, seleziona i comandi o scegli Vista semplice. Tema Chiaro / Scuro / Sistema salvato localmente. Nessuna rotazione automatica; rispettata la preferenza movimento ridotto.

EN: Pair NOVE MacroPad. Studio also accepts the old 99BIT MacroPad firmware identity. Before updating an existing device, export its profile and disconnect Studio; flash.py --mode update writes only the application and preserves data with the supplied partition layout. Never erase the whole flash. First mode is only for new boards.
IT: Associa NOVE MacroPad. Studio accetta anche il precedente nome 99BIT MacroPad. Prima di aggiornare un dispositivo esistente esporta il profilo e disconnetti Studio; flash.py --mode update scrive solo l'applicazione e conserva i dati con la mappa partizioni fornita. Non cancellare tutta la flash. First serve solo per schede nuove.

EN/IT: Software: PolyForm Noncommercial 1.0.0 + NOVE internal-productivity permission; see licenses/NOVE-LICENSE.txt. Candidate version / versione candidata. macOS validated; Windows/Linux not validated. Personal profiles and pairing backups are NOT included.

Firmware binary retained from validated commit e1a2e88. Packaged C++ sources have formatting-only changes since that build.

EN: If saving has an uncertain outcome, Studio keeps your draft and blocks further saves. Use Rileggi dispositivo to read the device again; compare the retained draft before explicitly saving. Reading never automatically retries a save.
IT: Se il salvataggio ha esito incerto, Studio conserva la bozza e blocca altri salvataggi. Usa Rileggi dispositivo: aggiorna il riferimento salvato senza sostituire la bozza. Controlla le modifiche prima di salvare esplicitamente; nessun tentativo automatico.
