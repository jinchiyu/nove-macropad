import { afterEach, expect, it, vi } from 'vitest';
import { Device, UncertainSaveError } from './serial';
import initial from '../public/profilo-iniziale.json';
import { validate } from './model';
validate(initial);
const profile = initial;
const raw = JSON.stringify(profile);
afterEach(() => {
  vi.useRealTimers();
  vi.unstubAllGlobals();
});
function portFor(handle: (r: any) => unknown) {
  let controller: ReadableStreamDefaultController<Uint8Array>;
  const calls: any[] = [];
  const port = {
    open: vi.fn(),
    close: vi.fn(),
    readable: new ReadableStream<Uint8Array>({
      start(c) {
        controller = c;
      },
    }),
    writable: new WritableStream<Uint8Array>({
      write(v) {
        const r = JSON.parse(new TextDecoder().decode(v));
        calls.push(r);
        const data = r.op === 'hello' ? { device: 'NOVE MacroPad', protocol: 1 } : handle(r);
        if (data === undefined) return;
        controller.enqueue(
          new TextEncoder().encode(JSON.stringify({ id: r.id, ok: true, data }) + '\n'),
        );
      },
    }),
  };
  vi.stubGlobal('navigator', { serial: { requestPort: async () => port } });
  return { calls, disconnect: () => controller.close() };
}
function block(offset: number, revision = 18) {
  return { revision, total: raw.length, chunk: raw.slice(offset, offset + 512) };
}
it.each([
  ['missing total', { revision: 18, chunk: 'x' }],
  ['negative total', { revision: 18, total: -1, chunk: 'x' }],
  ['oversized total', { revision: 18, total: 32769, chunk: 'x' }],
  ['invalid revision', { revision: '18', total: 1, chunk: 'x' }],
  ['empty block', { revision: 18, total: 10, chunk: '' }],
  ['overrun', { revision: 18, total: 1, chunk: 'xx' }],
  ['oversized block', { revision: 18, total: 600, chunk: 'x'.repeat(513) }],
])('rejects %s', async (_, reply) => {
  portFor(() => reply);
  const d = new Device();
  await d.connect();
  try {
    await expect(d.profile()).rejects.toThrow();
  } finally {
    await d.close();
  }
});
it.each(['total', 'revision'])('rejects changed %s mid-transfer', async (field) => {
  portFor((r) => ({
    ...block(r.offset),
    ...(r.offset ? { [field]: field === 'total' ? raw.length + 1 : 19 } : {}),
  }));
  const d = new Device();
  await d.connect();
  try {
    await expect(d.profile()).rejects.toThrow('cambiat');
  } finally {
    await d.close();
  }
});
it('reads complete multi-block profile', async () => {
  portFor((r) => block(r.offset));
  const d = new Device();
  await d.connect();
  try {
    expect(await d.profile()).toEqual({ profile, revision: 18 });
  } finally {
    await d.close();
  }
});
it('reports disconnect during read', async () => {
  const mock = portFor(() => undefined);
  const d = new Device();
  await d.connect();
  const pending = d.profile();
  mock.disconnect();
  await expect(pending).rejects.toThrow('scollegata');
  await d.close();
});
it('never commits after an invalid chunk acknowledgement', async () => {
  const mock = portFor((r) => (r.op === 'begin' ? {} : { offset: 0 }));
  const d = new Device();
  await d.connect();
  try {
    await expect(d.save(profile, 18)).rejects.toThrow('blocco');
    expect(mock.calls.some((r) => r.op === 'commit')).toBe(false);
    expect(d.requiresReadback).toBe(false);
  } finally {
    await d.close();
  }
});
it('locks saving after commit timeout until a valid readback', async () => {
  let revision = 18;
  const mock = portFor((r) => {
    if (r.op === 'get') return block(r.offset, revision);
    if (r.op === 'chunk') return { offset: r.offset + new TextEncoder().encode(r.chunk).length };
    if (r.op === 'commit') {
      revision = 19;
      return undefined;
    }
    return {};
  });
  const d = new Device();
  await d.connect();
  vi.useFakeTimers();
  const result = expect(d.save(profile, 18)).rejects.toBeInstanceOf(UncertainSaveError);
  await vi.advanceTimersByTimeAsync(5001);
  await result;
  expect(d.requiresReadback).toBe(true);
  const n = mock.calls.length;
  await expect(d.save(profile, 18)).rejects.toBeInstanceOf(UncertainSaveError);
  expect(mock.calls).toHaveLength(n);
  vi.useRealTimers();
  expect((await d.profile()).revision).toBe(19);
  expect(d.requiresReadback).toBe(false);
  await d.close();
});
it('disconnect during commit leaves uncertain outcome', async () => {
  const mock = portFor((r) => {
    if (r.op === 'chunk') return { offset: r.offset + r.chunk.length };
    if (r.op === 'commit') {
      queueMicrotask(() => mock.disconnect());
      return undefined;
    }
    return {};
  });
  const d = new Device();
  await d.connect();
  await expect(d.save(profile, 18)).rejects.toBeInstanceOf(UncertainSaveError);
  expect(d.requiresReadback).toBe(true);
  await d.close();
});
it('times out a stalled read', async () => {
  portFor(() => undefined);
  const d = new Device();
  await d.connect();
  vi.useFakeTimers();
  const p = expect(d.profile()).rejects.toThrow('non risponde');
  await vi.advanceTimersByTimeAsync(5001);
  await p;
  await d.close();
});
