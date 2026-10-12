import { describe, it, expect } from 'vitest';
import defaults from '../public/profilo-iniziale.json';
import { validate, chunks, bytes } from './model';
describe('profili', () => {
  it('conserva le assegnazioni iniziali', () => expect(() => validate(defaults)).not.toThrow());
  it.each([
    null,
    {},
    { ...defaults, schema: 2 },
    { ...defaults, layers: [] },
    { ...defaults, settings: { ...defaults.settings, brightness: 11 } },
  ])('rifiuta profilo invalido %s', (p) => expect(() => validate(p)).toThrow());
  it('rifiuta pause fuori limite e sequenze lunghe', () => {
    const p = structuredClone(defaults);
    p.layers[0].actions[0].steps = [{ kind: 'pause', ms: 1 }] as any;
    expect(() => validate(p)).toThrow();
    p.layers[0].actions[0].steps = Array(17).fill({ kind: 'key', modifiers: 8, usage: 6 });
    expect(() => validate(p)).toThrow();
  });
  it('frammenta senza rompere Unicode', () => {
    const s = 'à✓🟦'.repeat(200);
    const c = chunks(s);
    expect(c.join('')).toBe(s);
    expect(c.every((x) => bytes(x) <= 512)).toBe(true);
  });
});
