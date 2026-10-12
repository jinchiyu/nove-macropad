import { Usb } from 'lucide-react';
import { type Profile } from './model';
import { times } from './options';
export function SettingsPanel({
  draft,
  update,
}: {
  draft: Profile;
  update: (fn: (p: Profile) => void) => void;
}) {
  return (
    <section className="settings-panel">
      <h2>Comportamento del dispositivo</h2>
      <div className="setting-row">
        <div>
          <strong>Inverti la manopola</strong>
          <p>Scambia le azioni della rotazione oraria e antioraria.</p>
        </div>
        <input
          aria-label="Inverti la manopola"
          type="checkbox"
          checked={draft.settings.encoderReverse}
          onChange={(e) =>
            update((p) => {
              p.settings.encoderReverse = e.target.checked;
            })
          }
        />
      </div>
      <label>
        Spegni il display dopo
        <select
          value={draft.settings.displayTimeoutMs}
          onChange={(e) =>
            update((p) => {
              p.settings.displayTimeoutMs = +e.target.value;
            })
          }
        >
          {times.map(([v, t]) => (
            <option key={v} value={v}>
              {t}
            </option>
          ))}
        </select>
      </label>
      <p className="hint">
        Il primo gesto riaccende il display ed esegue il comando. La pressione lunga della manopola
        cambia layer dopo 700 ms.
      </p>
      <div className="info-box">
        <Usb size={20} />
        <div>
          <strong>USB per configurare. Bluetooth per lavorare.</strong>
          <p>
            Puoi chiudere Studio dopo aver salvato. Il MacroPad mantiene le impostazioni e necessita
            soltanto di alimentazione e collegamento Bluetooth.
          </p>
        </div>
      </div>
    </section>
  );
}
