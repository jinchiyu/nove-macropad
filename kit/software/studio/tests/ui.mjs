import { chromium } from '@playwright/test';
const browser = await chromium.launch({
  executablePath: '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',
  headless: true,
});
const page = await browser.newPage({ viewport: { width: 1440, height: 1040 } });
let errors = [];
page.on('pageerror', (e) => errors.push(e.message));
page.on('dialog', (d) => d.accept());
await page.goto(process.env.STUDIO_URL || 'http://127.0.0.1:9939');
await page.getByRole('button', { name: 'Esplora la demo' }).click();
await page.getByRole('button', { name: 'Tasto 1: Copy', exact: true }).click();
await page.screenshot({ path: 'artifacts/macropad.png', fullPage: true });
await page.getByLabel('Etichetta', { exact: true }).fill('Copia prova');
await page.getByRole('button', { name: 'Salva nella demo' }).click();
await page.getByLabel('Azione', { exact: true }).selectOption('sequence');
await page.getByRole('button', { name: /Aggiungi passaggio/ }).click();
await page.getByLabel('Durata (ms)').fill('750');
await page.getByRole('button', { name: 'Nuovo layer' }).click();
await page.getByLabel('Nome del layer').fill('CUSTOM');
await page.getByRole('button', { name: 'Duplica', exact: true }).click();
await page.getByRole('button', { name: 'Elimina', exact: true }).click();
await page.getByRole('button', { name: 'Illuminazione', exact: true }).click();
await page.getByLabel('Colore MAC').fill('#ff8800');
await page.screenshot({ path: 'artifacts/illuminazione.png', fullPage: true });
await page.getByRole('button', { name: 'Impostazioni', exact: true }).click();
await page.getByLabel('Inverti la manopola').check();
await page.getByLabel('Spegni il display dopo').selectOption('60000');
await page.getByRole('button', { name: 'Backup', exact: true }).click();
await page
  .locator('input[type=file]')
  .setInputFiles({ name: 'bad.json', mimeType: 'application/json', buffer: Buffer.from('{}') });
await page.getByRole('alert').waitFor();
await page.setViewportSize({ width: 900, height: 900 });
await page.getByRole('button', { name: 'MacroPad', exact: true }).click();
await page.screenshot({ path: 'artifacts/compact.png', fullPage: true });
if (await page.evaluate(() => document.documentElement.scrollWidth > innerWidth))
  throw Error('Overflow horizontal');
if (errors.length) throw Error(errors.join('\n'));
console.log(
  'PASS UI: demo, edit, sequence, layers, colours, settings, invalid import, compact layout; no JS errors',
);
await browser.close();
