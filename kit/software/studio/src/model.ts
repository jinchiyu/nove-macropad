export type Step =
  | { kind: 'key'; usage: number; modifiers: number }
  | { kind: 'media'; usage: number }
  | { kind: 'pause'; ms: number };
export type Action = { label: string; steps: Step[] };
export type Layer = { name: string; color: string; actions: Action[] };
export type Profile = {
  schema: 1;
  settings: {
    brightness: number;
    ledTimeoutMs: number;
    displayTimeoutMs: number;
    encoderReverse: boolean;
  };
  layers: Layer[];
};
export const bytes = (s: string) => new TextEncoder().encode(s).length;
export const media: [[number, string], ...[number, string][]] = [
  [205, 'Riproduci / pausa'],
  [181, 'Traccia successiva'],
  [182, 'Traccia precedente'],
  [183, 'Stop'],
  [226, 'Muto'],
  [233, 'Volume +'],
  [234, 'Volume −'],
];
export const keys: [number, string][] = [
  ...Array.from({ length: 26 }, (_, i): [number, string] => [i + 4, String.fromCharCode(i + 65)]),
  ...Array.from({ length: 9 }, (_, i): [number, string] => [i + 30, String(i + 1)]),
  [39, '0'],
  [40, 'Invio'],
  [41, 'Esc'],
  [42, 'Backspace'],
  [43, 'Tab'],
  [44, 'Spazio'],
  [45, '− (fisico)'],
  [46, '= (fisico)'],
  [47, '[ (fisico)'],
  [48, '] (fisico)'],
  [49, '\\ (fisico)'],
  [51, '; (fisico)'],
  [52, "' (fisico)"],
  [53, '` (fisico)'],
  [54, ','],
  [55, '.'],
  [56, '/ (fisico)'],
  ...Array.from({ length: 12 }, (_, i): [number, string] => [58 + i, `F${i + 1}`]),
  [73, 'Insert'],
  [74, 'Home'],
  [75, 'Pagina su'],
  [76, 'Canc'],
  [77, 'Fine'],
  [78, 'Pagina giù'],
  [79, '→'],
  [80, '←'],
  [81, '↓'],
  [82, '↑'],
];
export const emptyAction = (): Action => ({ label: '', steps: [] });
export const controlName = (i: number) =>
  i < 9 ? `Tasto ${i + 1}` : ['Rotazione oraria', 'Rotazione antioraria', 'Clic manopola'][i - 9];
export function stepLabel(s: Step): string {
  if (s.kind === 'pause') return `Pausa ${s.ms} ms`;
  if (s.kind === 'media') return media.find((x) => x[0] === s.usage)?.[1] || 'Media';
  return (
    [
      [8, '⌘'],
      [4, '⌥'],
      [1, '⌃'],
      [2, '⇧'],
    ]
      .filter((x) => s.modifiers & Number(x[0]))
      .map((x) => x[1])
      .join('') + (keys.find((x) => x[0] === s.usage)?.[1] || `HID ${s.usage}`)
  );
}
export function actionLabel(a: Action) {
  return (
    a.label ||
    (a.steps.length === 1 ? stepLabel(a.steps[0]) : a.steps.length ? 'Sequenza' : 'Non assegnato')
  );
}
export function validate(value: unknown): asserts value is Profile {
  const p = value as Profile;
  const int = (v: unknown, l: number, h: number) =>
    Number.isInteger(v) && Number(v) >= l && Number(v) <= h;
  const text = (v: unknown, n: number, required = false) =>
    typeof v === 'string' &&
    (!required || v.length > 0) &&
    bytes(v) <= n &&
    !/[\x00-\x1f\x7f]/.test(v);
  const fail = (s: string): never => {
    throw new Error(s);
  };
  if (
    !p ||
    p.schema !== 1 ||
    !p.settings ||
    !Array.isArray(p.layers) ||
    p.layers.length < 1 ||
    p.layers.length > 8
  )
    fail('Profilo non valido: servono da 1 a 8 layer (schema 1).');
  if (!int(p.settings.brightness, 0, 10) || typeof p.settings.encoderReverse !== 'boolean')
    fail('Luminosità o direzione encoder non valida.');
  for (const t of [p.settings.ledTimeoutMs, p.settings.displayTimeoutMs])
    if (t !== 0 && !int(t, 15000, 600000))
      fail('Tempo di spegnimento non valido (15–600 secondi).');
  for (const [i, l] of p.layers.entries()) {
    if (
      !l ||
      !text(l.name, 24, true) ||
      !/^#[0-9a-f]{6}$/i.test(l.color) ||
      !Array.isArray(l.actions) ||
      l.actions.length !== 12
    )
      fail(`Layer ${i + 1}: nome, colore o numero di controlli non valido.`);
    for (const a of l.actions) {
      if (!a || !text(a.label, 48) || !Array.isArray(a.steps) || a.steps.length > 16)
        fail('Azione non valida: massimo 16 passaggi, etichetta 48 byte.');
      for (const s of a.steps) {
        if (!s) fail('Passaggio mancante.');
        if (s.kind === 'key') {
          if (!int(s.usage, 4, 115) || !int(s.modifiers, 0, 15)) fail('Scorciatoia non valida.');
        } else if (s.kind === 'media') {
          if (!media.some((x) => x[0] === s.usage)) fail('Comando multimediale non valido.');
        } else if (s.kind === 'pause') {
          if (!int(s.ms, 50, 5000)) fail('La pausa deve essere tra 50 e 5.000 ms.');
        } else fail('Tipo di passaggio non supportato.');
      }
    }
  }
  if (bytes(JSON.stringify(p)) > 32768)
    fail('Il profilo supera 32 KiB. Riduci il numero di passaggi.');
}
export function chunks(s: string, max = 512) {
  const result: string[] = [];
  let part = '';
  for (const c of s) {
    if (bytes(part + c) > max) {
      result.push(part);
      part = '';
    }
    part += c;
  }
  if (part) result.push(part);
  return result;
}
