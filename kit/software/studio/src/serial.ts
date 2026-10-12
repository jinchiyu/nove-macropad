import { bytes, chunks, validate, type Profile } from './model';
export type DeviceStatus = {
  connected: boolean;
  ready: boolean;
  layer: number;
  testing: boolean;
  revision: number;
};
export type InputEvent = {
  event: 'input';
  control: 'key' | 'encoder' | 'click' | 'hold';
  value: number;
};
type Data = Record<string, unknown>;
const MAX_BYTES = 32768;
const object = (v: unknown): v is Data => typeof v === 'object' && v !== null && !Array.isArray(v);
const integer = (v: unknown, max = 0xffffffff): v is number =>
  Number.isInteger(v) && Number(v) >= 0 && Number(v) <= max;
export class UncertainSaveError extends Error {
  constructor() {
    super(
      'Esito del salvataggio incerto. La bozza è conservata: rileggi il dispositivo prima di salvare ancora.',
    );
  }
}
class DeviceResponseError extends Error {}
export class Device {
  port?: SerialPort;
  reader?: ReadableStreamDefaultReader<Uint8Array>;
  writer?: WritableStreamDefaultWriter<Uint8Array>;
  private next = 1;
  private readTask?: Promise<void>;
  private pending = new Map<
    number,
    { resolve: (v: Data) => void; reject: (e: Error) => void; timer: ReturnType<typeof setTimeout> }
  >();
  requiresReadback = false;
  onEvent: (e: InputEvent) => void = () => {};
  onDisconnect: () => void = () => {};
  async connect() {
    if (!('serial' in navigator))
      throw new Error('Apri NOVE Studio in Chrome per usare la connessione USB.');
    this.port = await navigator.serial.requestPort();
    try {
      await this.port.open({ baudRate: 115200 });
      this.writer = this.port.writable!.getWriter();
      this.reader = this.port.readable!.getReader();
      this.readTask = this.read(this.reader);
      let hello: Data | undefined;
      for (let attempt = 0; attempt < 3; attempt++) {
        try {
          hello = await this.request('hello');
          break;
        } catch (e) {
          if (attempt === 2) throw e;
        }
      }
      if (
        !hello ||
        !['99BIT MacroPad', 'NOVE MacroPad'].includes(String(hello.device)) ||
        hello.protocol !== 1
      )
        throw new Error('Dispositivo o firmware non compatibile con Studio.');
      return hello;
    } catch (e) {
      await this.close();
      throw e;
    }
  }
  private async read(reader: ReadableStreamDefaultReader<Uint8Array>) {
    const decoder = new TextDecoder();
    let buffer = '';
    try {
      for (;;) {
        const { value, done } = await reader.read();
        if (done) break;
        buffer += decoder.decode(value, { stream: true });
        let end: number;
        while ((end = buffer.indexOf('\n')) >= 0) {
          if (end > 65536) throw new Error('Risposta troppo grande');
          const line = buffer.slice(0, end).trim();
          buffer = buffer.slice(end + 1);
          let msg: unknown;
          try {
            msg = JSON.parse(line);
          } catch {
            continue;
          } // Firmware diagnostic lines are not JSON.
          if (!object(msg)) continue;
          if (msg.event === 'input') {
            const { control, value } = msg;
            if (
              (control === 'key' && integer(value, 8)) ||
              (control === 'encoder' &&
                Number.isInteger(value) &&
                value !== 0 &&
                Math.abs(Number(value)) <= 127) ||
              ((control === 'click' || control === 'hold') && value === 1)
            )
              this.onEvent(msg as InputEvent);
            continue;
          }
          if (!integer(msg.id)) continue;
          const p = this.pending.get(msg.id);
          if (!p) continue;
          clearTimeout(p.timer);
          this.pending.delete(msg.id);
          if (msg.ok === false && typeof msg.error === 'string')
            p.reject(new DeviceResponseError(msg.error));
          else if (msg.ok === true && object(msg.data)) p.resolve(msg.data);
          else p.reject(new Error('Risposta USB non valida.'));
        }
        if (buffer.length > 65536) throw new Error('Risposta troppo grande');
      }
    } catch {
      /* Pending operations receive the disconnect error below. */
    } finally {
      for (const p of this.pending.values()) {
        clearTimeout(p.timer);
        p.reject(new Error('USB scollegata. La bozza resta disponibile.'));
      }
      this.pending.clear();
      this.onDisconnect();
    }
  }
  async request(op: string, fields: Data = {}): Promise<Data> {
    if (!this.writer) throw new Error('Collega il MacroPad via USB.');
    const id = this.next++;
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        this.pending.delete(id);
        reject(new Error('Il dispositivo non risponde. Verifica firmware Studio e porta USB.'));
      }, 5000);
      this.pending.set(id, { resolve, reject, timer });
      this.writer!.write(
        new TextEncoder().encode(JSON.stringify({ v: 1, id, op, ...fields }) + '\n'),
      ).catch((e) => {
        clearTimeout(timer);
        this.pending.delete(id);
        reject(e);
      });
    });
  }
  async status(): Promise<DeviceStatus> {
    const r = await this.request('status');
    if (
      typeof r.connected !== 'boolean' ||
      typeof r.ready !== 'boolean' ||
      typeof r.testing !== 'boolean' ||
      !integer(r.layer, 7) ||
      !integer(r.revision)
    )
      throw new Error('Stato USB non valido.');
    return r as DeviceStatus;
  }
  async test(enabled: boolean) {
    const r = await this.request('test', { enabled });
    if (r.testing !== enabled) throw new Error('Stato prova non valido.');
    return enabled;
  }
  async profile(): Promise<{ profile: Profile; revision: number }> {
    let raw = '',
      offset = 0,
      total: number | undefined,
      revision: number | undefined;
    for (;;) {
      const r = await this.request('get', { offset });
      if (
        !integer(r.total, MAX_BYTES) ||
        r.total === 0 ||
        !integer(r.revision) ||
        typeof r.chunk !== 'string'
      )
        throw new Error('Risposta profilo non valida.');
      if (total !== undefined && r.total !== total)
        throw new Error('Dimensione profilo cambiata durante la lettura.');
      if (revision !== undefined && r.revision !== revision)
        throw new Error('Profilo cambiato durante la lettura. Riprova.');
      total = r.total;
      revision = r.revision;
      const length = bytes(r.chunk);
      if (length === 0 || length > 512 || offset + length > total || offset + length > MAX_BYTES)
        throw new Error('Trasferimento profilo incompleto o non valido.');
      raw += r.chunk;
      offset += length;
      if (offset === total) break;
    }
    const profile: unknown = JSON.parse(raw);
    validate(profile);
    this.requiresReadback = false;
    return { profile, revision: revision! };
  }
  async save(profile: Profile, revision: number): Promise<{ revision: number }> {
    if (this.requiresReadback) throw new UncertainSaveError();
    validate(profile);
    await this.request('begin', { revision });
    let offset = 0;
    for (const chunk of chunks(JSON.stringify(profile))) {
      const r = await this.request('chunk', { offset, chunk });
      offset += bytes(chunk);
      if (r.offset !== offset)
        throw new Error('Conferma del blocco USB non valida. Nessun commit inviato.');
    }
    try {
      const r = await this.request('commit');
      if (!integer(r.revision) || r.revision !== (revision + 1) >>> 0)
        throw new Error('Revisione di salvataggio non valida.');
      return { revision: r.revision };
    } catch (e) {
      if (e instanceof DeviceResponseError) throw e;
      this.requiresReadback = true;
      throw new UncertainSaveError();
    }
  }
  async close() {
    const reader = this.reader;
    this.reader = undefined;
    try {
      await reader?.cancel();
    } catch {}
    await this.readTask;
    this.readTask = undefined;
    try {
      reader?.releaseLock();
    } catch {}
    try {
      this.writer?.releaseLock();
    } catch {}
    this.writer = undefined;
    try {
      await this.port?.close();
    } catch {}
    this.port = undefined;
  }
}
