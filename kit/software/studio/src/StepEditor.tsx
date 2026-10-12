import { X } from 'lucide-react';
import { keys, media, type Step } from './model';
export function StepEditor({
  step,
  onChange,
  onRemove,
  index,
}: {
  step: Step;
  onChange: (s: Step) => void;
  onRemove?: () => void;
  index: number;
}) {
  return (
    <div className="step">
      <div className="step-title">
        <span>PASSAGGIO {String(index + 1).padStart(2, '0')}</span>
        {onRemove && (
          <button className="icon" aria-label={`Rimuovi passaggio ${index + 1}`} onClick={onRemove}>
            <X size={15} />
          </button>
        )}
      </div>
      <label>
        Tipo
        <select
          value={step.kind}
          onChange={(e) =>
            onChange(
              e.target.value === 'key'
                ? { kind: 'key', usage: 6, modifiers: 8 }
                : e.target.value === 'media'
                  ? { kind: 'media', usage: 205 }
                  : { kind: 'pause', ms: 500 },
            )
          }
        >
          <option value="key">Scorciatoia da tastiera</option>
          <option value="media">Multimedia</option>
          <option value="pause">Pausa</option>
        </select>
      </label>
      {step.kind === 'key' ? (
        <>
          <div className="modifiers">
            {(
              [
                [8, '⌘', 'Command'],
                [4, '⌥', 'Option'],
                [1, '⌃', 'Control'],
                [2, '⇧', 'Shift'],
              ] as const
            ).map(([bit, symbol, name]) => (
              <button
                key={bit}
                aria-label={name}
                aria-pressed={!!(step.modifiers & bit)}
                className={step.modifiers & bit ? 'selected' : ''}
                onClick={() => onChange({ ...step, modifiers: step.modifiers ^ bit })}
              >
                {symbol}
                <small>{name}</small>
              </button>
            ))}
          </div>
          <label>
            Tasto
            <select
              value={step.usage}
              onChange={(e) => onChange({ ...step, usage: +e.target.value })}
            >
              {!keys.some((k) => k[0] === step.usage) && (
                <option value={step.usage}>HID {step.usage}</option>
              )}
              {keys.map(([n, l]) => (
                <option key={n} value={n}>
                  {l}
                </option>
              ))}
            </select>
          </label>
        </>
      ) : step.kind === 'media' ? (
        <label>
          Comando
          <select
            value={step.usage}
            onChange={(e) => onChange({ ...step, usage: +e.target.value })}
          >
            {media.map(([n, l]) => (
              <option key={n} value={n}>
                {l}
              </option>
            ))}
          </select>
        </label>
      ) : (
        <label>
          Durata (ms)
          <input
            type="number"
            min="50"
            max="5000"
            step="50"
            value={step.ms}
            onChange={(e) => onChange({ ...step, ms: +e.target.value })}
          />
        </label>
      )}
    </div>
  );
}
