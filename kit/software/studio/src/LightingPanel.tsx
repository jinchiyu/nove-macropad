import { type Profile } from './model';
import { times } from './options';
export function LightingPanel({
  draft,
  update,
}: {
  draft: Profile;
  update: (fn: (p: Profile) => void) => void;
}) {
  return (
    <section className="settings-panel">
      <h2>Illuminazione perimetrale</h2>
      <p>24 LED, un colore per layer. Anteprima locale: salva per applicare al dispositivo.</p>
      <div className="colour-list">
        {draft.layers.map((l, i) => (
          <div className="colour-row" key={i}>
            <span
              className="colour-swatch"
              style={{ background: l.color, boxShadow: `0 0 20px ${l.color}30` }}
            />
            <strong>{l.name}</strong>
            <label>
              Colore
              <input
                aria-label={`Colore ${l.name}`}
                type="color"
                value={l.color}
                onChange={(e) =>
                  update((p) => {
                    p.layers[i].color = e.target.value;
                  })
                }
              />
            </label>
            <code>{l.color.toUpperCase()}</code>
          </div>
        ))}
      </div>
      <div className="brightness-control">
        <label htmlFor="led-brightness">Luminosità LED</label>
        <div className="brightness-value">
          <input
            aria-label="Luminosità LED in percentuale"
            type="number"
            min="0"
            max="10"
            step="1"
            value={draft.settings.brightness}
            onChange={(e) => {
              const n = e.target.valueAsNumber;
              if (Number.isFinite(n))
                update((p) => {
                  p.settings.brightness = Math.max(0, Math.min(10, Math.round(n)));
                });
            }}
          />
          <span>%</span>
        </div>
        <input
          id="led-brightness"
          type="range"
          min="0"
          max="10"
          step="1"
          aria-valuetext={
            draft.settings.brightness === 0 ? 'Spenti' : `${draft.settings.brightness}%`
          }
          value={draft.settings.brightness}
          onChange={(e) =>
            update((p) => {
              p.settings.brightness = +e.target.value;
            })
          }
        />
        <div className="brightness-scale">
          <span>0% · Spenti</span>
          <span>10% · Massimo</span>
        </div>
      </div>
      <p className="hint">
        Regola da 0 a 10% e premi Salva sul MacroPad per applicare. I LED restano spenti quando il
        Bluetooth non è pronto.
      </p>
      <label>
        Spegni i LED dopo
        <select
          value={draft.settings.ledTimeoutMs}
          onChange={(e) =>
            update((p) => {
              p.settings.ledTimeoutMs = +e.target.value;
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
    </section>
  );
}
