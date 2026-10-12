import { useEffect, useRef, useState } from 'react';
import { Device, type DeviceStatus, type InputEvent } from './serial';
export function useDeviceConnection({
  busy,
  onInput,
  onError,
}: {
  busy: boolean;
  onInput: (e: InputEvent) => void;
  onError: (message: string) => void;
}) {
  const device = useRef<Device | null>(null);
  const [connected, setConnected] = useState(false),
    [testing, setTesting] = useState(false),
    [status, setStatus] = useState<DeviceStatus | null>(null);
  const callbacks = useRef({ onInput, onError });
  callbacks.current = { onInput, onError };
  useEffect(
    () => () => {
      void device.current?.close();
    },
    [],
  );
  useEffect(() => {
    if (!connected) return;
    let cancelled = false,
      polling = false;
    const id = setInterval(async () => {
      const d = device.current;
      if (!d || busy || polling) return;
      polling = true;
      try {
        if (testing) await d.request('heartbeat');
        const next = await d.status();
        if (!cancelled) {
          setStatus(next);
          if (!next.testing) setTesting(false);
        }
      } catch (e) {
        if (!cancelled) callbacks.current.onError((e as Error).message);
      } finally {
        polling = false;
      }
    }, 800);
    return () => {
      cancelled = true;
      clearInterval(id);
    };
  }, [connected, testing, busy]);
  async function open() {
    await device.current?.close();
    const d = new Device();
    device.current = d;
    d.onEvent = (e) => callbacks.current.onInput(e);
    d.onDisconnect = () => {
      if (device.current === d) {
        setConnected(false);
        setTesting(false);
        setStatus(null);
      }
    };
    try {
      await d.connect();
      const result = await d.profile();
      const next = await d.status();
      setStatus(next);
      setTesting(next.testing);
      setConnected(true);
      return result;
    } catch (e) {
      await d.close();
      throw e;
    }
  }
  async function reread() {
    const d = device.current;
    if (!d || !connected) throw new Error('Ricollega il MacroPad prima di rileggere il profilo.');
    const result = await d.profile();
    setStatus(await d.status());
    return result;
  }
  async function disconnect() {
    await device.current?.close();
  }
  return { device, connected, testing, setTesting, status, setStatus, open, reread, disconnect };
}
