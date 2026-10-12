import { it, expect, vi } from 'vitest';
import { Device } from './serial';
it.each(['99BIT MacroPad', 'NOVE MacroPad'])(
  'connette %s con risposte frammentate e log seriali',
  async (name) => {
    let controller: ReadableStreamDefaultController<Uint8Array>;
    const encoder = new TextEncoder();
    const readable = new ReadableStream<Uint8Array>({
      start(c) {
        controller = c;
      },
    });
    const writable = new WritableStream<Uint8Array>({
      write(bytes) {
        const r = JSON.parse(new TextDecoder().decode(bytes));
        const msg =
          JSON.stringify({ id: r.id, ok: true, data: { device: name, protocol: 1 } }) + '\n';
        controller.enqueue(encoder.encode('[BOOT] log\n' + msg.slice(0, 10)));
        controller.enqueue(encoder.encode(msg.slice(10)));
      },
    });
    const port = { open: vi.fn(), close: vi.fn(), readable, writable };
    vi.stubGlobal('navigator', { serial: { requestPort: async () => port } });
    const d = new Device();
    expect((await d.connect()).protocol).toBe(1);
    expect((await d.request('hello')).device).toBe(name);
    await d.close();
    vi.unstubAllGlobals();
  },
);
it('rifiuta il firmware incompatibile', async () => {
  let c: ReadableStreamDefaultController<Uint8Array>;
  const e = new TextEncoder();
  const port = {
    open: vi.fn(),
    close: vi.fn(),
    readable: new ReadableStream<Uint8Array>({
      start(v) {
        c = v;
      },
    }),
    writable: new WritableStream<Uint8Array>({
      write(v) {
        const r = JSON.parse(new TextDecoder().decode(v));
        c.enqueue(
          e.encode(
            JSON.stringify({ id: r.id, ok: true, data: { device: 'altro', protocol: 2 } }) + '\n',
          ),
        );
      },
    }),
  };
  vi.stubGlobal('navigator', { serial: { requestPort: async () => port } });
  await expect(new Device().connect()).rejects.toThrow('compatibile');
  vi.unstubAllGlobals();
});
