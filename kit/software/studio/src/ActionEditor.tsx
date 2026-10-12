import { Settings2, Plus } from 'lucide-react';
import { controlName, type Action } from './model';
import { StepEditor } from './StepEditor';
export function ActionEditor({
  a,
  selected,
  sequence,
  setSequence,
  action,
}: {
  a: Action;
  selected: number;
  sequence: boolean;
  setSequence: (v: boolean) => void;
  action: (a: Action) => void;
}) {
  return (
    <section className="action-panel">
      <div className="panel-heading">
        <div>
          <span className="eyebrow">CONFIGURAZIONE</span>
          <h2>{controlName(selected)}</h2>
        </div>
        <Settings2 size={20} />
      </div>
      <label>
        Etichetta
        <input
          value={a.label}
          placeholder="Es. Copia"
          onChange={(e) => action({ ...a, label: e.target.value })}
        />
      </label>
      <label>
        Azione
        <select
          aria-label="Azione"
          value={
            sequence || a.steps.length > 1
              ? 'sequence'
              : a.steps.length
                ? a.steps[0].kind === 'pause'
                  ? 'sequence'
                  : a.steps[0].kind
                : 'none'
          }
          onChange={(e) => {
            const type = e.target.value;
            setSequence(type === 'sequence');
            action({
              ...a,
              steps:
                type === 'none'
                  ? []
                  : type === 'sequence'
                    ? a.steps.length
                      ? a.steps
                      : [{ kind: 'key', usage: 6, modifiers: 8 }]
                    : type === 'key'
                      ? [{ kind: 'key', usage: 6, modifiers: 8 }]
                      : [{ kind: 'media', usage: 205 }],
            });
          }}
        >
          <option value="none">Non assegnato</option>
          <option value="key">Scorciatoia</option>
          <option value="media">Multimedia</option>
          <option value="sequence">Sequenza</option>
        </select>
      </label>
      <div className="steps">
        {a.steps.map((s, i) => (
          <StepEditor
            key={i}
            step={s}
            index={i}
            onChange={(st) =>
              action({ ...a, steps: a.steps.map((old, j) => (i === j ? st : old)) })
            }
            onRemove={
              sequence || a.steps.length > 1
                ? () => action({ ...a, steps: a.steps.filter((_, j) => i !== j) })
                : undefined
            }
          />
        ))}
      </div>
      {(sequence || a.steps.length > 1) && (
        <button
          className="button full"
          disabled={a.steps.length >= 16}
          onClick={() => action({ ...a, steps: [...a.steps, { kind: 'pause', ms: 500 }] })}
        >
          <Plus size={15} />
          Aggiungi passaggio · {a.steps.length}/16
        </button>
      )}
      <p className="hint">
        Le scorciatoie seguono il layout della tastiera del Mac. Le modifiche si applicano solo dopo
        il salvataggio.
      </p>
    </section>
  );
}
