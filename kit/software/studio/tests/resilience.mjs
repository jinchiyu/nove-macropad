import { chromium, expect } from '@playwright/test';
import { readFileSync } from 'node:fs';
const base = process.env.STUDIO_URL || 'http://127.0.0.1:9941';
const profile = JSON.parse(
  readFileSync(new URL('../public/profilo-iniziale.json', import.meta.url)),
);
const browser = await chromium.launch({
  executablePath: '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',
  headless: true,
  args: ['--enable-unsafe-swiftshader'],
});
try {
  for (const failure of ['module', 'webgl', 'context']) {
    const page = await browser.newPage();
    if (failure === 'module') await page.route('**/assets/CadView-*.js', (r) => r.abort());
    if (failure === 'webgl')
      await page.addInitScript(() => {
        const original = HTMLCanvasElement.prototype.getContext;
        HTMLCanvasElement.prototype.getContext = function (type, ...args) {
          if (type.includes('webgl')) return null;
          return original.call(this, type, ...args);
        };
      });
    await page.goto(base);
    await page.getByRole('button', { name: 'Esplora la demo' }).click();
    if (failure === 'context') {
      await expect(page.locator('.cad-loading')).toHaveCount(0);
      await page
        .locator('canvas')
        .evaluate((c) => c.dispatchEvent(new Event('webglcontextlost', { cancelable: true })));
    }
    await expect(page.locator('.device-stage .hardware')).toHaveCount(1);
    await page.getByLabel('Etichetta', { exact: true }).fill('Bozza conservata');
    await expect(page.getByLabel('Etichetta', { exact: true })).toHaveValue('Bozza conservata');
    await page.close();
  }
  const page = await browser.newPage();
  await page.addInitScript((initial) => {
    let controller;
    let incoming = '';
    let commits = 0;
    const port = {
      open: async () => {},
      close: async () => {},
      readable: new ReadableStream({
        start(c) {
          controller = c;
        },
      }),
      writable: new WritableStream({
        write(bytes) {
          const r = JSON.parse(new TextDecoder().decode(bytes));
          let data = {};
          if (r.op === 'hello') data = { device: 'NOVE MacroPad', protocol: 1 };
          if (r.op === 'status')
            data = { connected: true, ready: true, testing: false, layer: 0, revision: 18 };
          if (r.op === 'get') {
            const raw = JSON.stringify(initial);
            data = { revision: 18, total: raw.length, chunk: raw.slice(r.offset, r.offset + 512) };
          }
          if (r.op === 'begin') incoming = '';
          if (r.op === 'chunk') {
            incoming += r.chunk;
            data = { offset: new TextEncoder().encode(incoming).length };
          }
          if (r.op === 'commit') {
            commits++;
            window.testCommits = commits;
            return;
          }
          controller.enqueue(
            new TextEncoder().encode(JSON.stringify({ id: r.id, ok: true, data }) + '\n'),
          );
        },
      }),
    };
    Object.defineProperty(navigator, 'serial', {
      value: { requestPort: async () => port },
      configurable: true,
    });
  }, profile);
  await page.goto(base);
  await page.getByRole('button', { name: 'Connetti MacroPad', exact: true }).first().click();
  await expect(page.getByLabel('Etichetta', { exact: true })).toBeVisible();
  await page.getByLabel('Etichetta', { exact: true }).fill('Bozza dopo timeout');
  await page.getByRole('button', { name: 'Salva sul MacroPad', exact: true }).click();
  await expect(page.getByRole('button', { name: 'Rileggi dispositivo' })).toBeVisible({
    timeout: 10000,
  });
  await expect(
    page.getByRole('button', { name: 'Salva sul MacroPad', exact: true }),
  ).toBeDisabled();
  await expect(page.getByLabel('Etichetta', { exact: true })).toHaveValue('Bozza dopo timeout');
  await page.getByRole('button', { name: 'Rileggi dispositivo' }).click();
  await expect(page.getByRole('button', { name: 'Salva sul MacroPad', exact: true })).toBeEnabled();
  await expect(page.getByLabel('Etichetta', { exact: true })).toHaveValue('Bozza dopo timeout');
  expect(await page.evaluate(() => window.testCommits)).toBe(1);
  await page.close();
  console.log(
    'PASS: module/WebGL/context fallback; commit timeout locks saving; readback preserves draft and never retries automatically',
  );
} finally {
  await browser.close();
}
