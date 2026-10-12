# Aggiornamento dispositivo — NOVE Studio 1.1.0-rc.1

12 ottobre 2026. Firmware installato dal binario candidato associato al commit `e1a2e88`.

## Risultati automatici

- Prima: Studio 1.0.4, identità `99BIT MacroPad`, storage disponibile, tre layer, revisione 18, BLE connesso e pronto.
- Copia locale del profilo, dell'applicazione precedente, dell'area sistema/NVS e dell'intera partizione dati dei profili in `backups/pre-nove-1.1/` (esclusa da Git). Applicazione precedente confrontata byte per byte con il binario 1.0.4; tabella partizioni confrontata con quella del progetto.
- La lettura integrale iniziale si è fermata a 2 MiB. Il comando ROM di lettura non configurava la dimensione della flash. Risolto impostando 8 MiB nella RAM del bootloader e completando copie separate delle aree necessarie. Non è stata eseguita alcuna cancellazione completa.
- Scritta soltanto l'applicazione a `0x10000`, con esptool 4.5.1 in modalità `--no-stub`. Hash della scrittura verificato da esptool. Tabella partizioni e dati non sovrascritti.
- SHA-256 binario: `a03a094de48fb378dcc650ed16936d7c68064aa329e21caecd73ea616ae693b3`.
- Dopo riavvio: `NOVE MacroPad`, `studio-1.1.0-rc.1`, protocollo 1, storage disponibile. BLE connesso e pronto senza nuovo abbinamento.
- Profilo prima/dopo uguale integralmente (confronto JSON), tre layer e revisione 18 conservati. Nessuna operazione di salvataggio, ripristino o modifica dei profili eseguita.

- Modalità prova USB e heartbeat verificati; uscita automatica dopo 3 secondi senza heartbeat verificata. Profilo e revisione ricontrollati invariati.

## Collaudo fisico

L’utente conferma OLED con nome NOVE e LED regolari. La prova USB ha registrato 23 eventi: tutti i nove tasti (indici 0–8), rotazione positiva e negativa, clic e pressione lunga. Modalità prova disattivata al termine; BLE ancora pronto e profilo/revisione nuovamente verificati invariati.

L’utente conferma successivamente che una macro abituale nell’applicazione di destinazione e il cambio layer/colore funzionano come prima dell’aggiornamento. Questo collaudo non implica la prova esaustiva di ogni sequenza personalizzata. La verifica automatica della riconnessione BLE non certifica da sola l'esecuzione di ogni macro nell'applicazione di destinazione.

## Secondo collaudo prima della release — 12 ottobre 2026

Eseguito sul firmware già installato `studio-1.1.0-rc.1`, senza nuove scritture della flash. Non costituisce un collaudo della nuova build ottenuta dai sorgenti formattati: il kit conserva il binario originariamente installato e verificato, con SHA-256 riportato sopra.

- Nuova copia privata dei profili prima della prova, esclusa da Git e dal kit.
- USB: identità NOVE MacroPad, protocollo 1, storage disponibile; BLE connesso e pronto.
- Registrati 27 eventi: tutti i nove tasti, rotazione nei due sensi, clic e pressione lunga della manopola.
- Uscita dalla modalità prova confermata dal dispositivo (`testing: false`).
- Confronto integrale JSON prima/dopo: tre layer e revisione 18 invariati. Nessuna scrittura dei profili.
- Utente: OLED e LED regolari; successivamente confermati funzionamento di una macro abituale e cambio layer/colore.

Il kit include Studio 1.1.0-rc.2 e il firmware collaudato 1.1.0-rc.1. La prova non copre tutte le macro possibili, Windows/Linux o una prima installazione su scheda vergine. Licenza software e pubblicazione definitiva restano da completare.
