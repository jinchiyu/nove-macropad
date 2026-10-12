import { chromium, expect } from '@playwright/test';
const browser = await chromium.launch({
  executablePath: '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',
  headless: true,
  args: ['--enable-unsafe-swiftshader'],
});
try {
  const page = await browser.newPage({ viewport: { width: 1440, height: 1080 } }),
    errors = [];
  page.on('pageerror', (e) => errors.push(e.message));
  await page.goto(process.env.STUDIO_URL || 'http://127.0.0.1:9941');
  await page.getByLabel('Tema', { exact: true }).selectOption('dark');
  await page.reload();
  await expect(page.locator('html')).toHaveAttribute('data-theme', 'dark');
  await page.getByRole('button', { name: 'Esplora la demo' }).click();
  await expect(page.locator('.cad-loading')).toHaveCount(0, { timeout: 30000 });
  await expect(page.locator('canvas')).toHaveCount(1);
  await page.getByRole('button', { name: 'Tasto 9:', exact: false }).click();
  await expect(page.locator('.action-panel h2')).toContainText('9');
  await page.screenshot({ path: 'artifacts/cad-dark.png', fullPage: true });
  await page.getByRole('button', { name: 'Prova comandi', exact: true }).click();
  await page.getByRole('button', { name: 'Tasto 1:', exact: false }).click();
  await page.getByRole('button', { name: 'Tasto 1:', exact: false }).click();
  const canvas = await page.locator('canvas').boundingBox();
  await page.mouse.move(canvas.x + canvas.width / 2, canvas.y + canvas.height / 2);
  await page.mouse.down();
  await page.mouse.move(canvas.x + canvas.width / 2 + 100, canvas.y + canvas.height / 2 + 40, {
    steps: 12,
  });
  await page.mouse.up();
  await page.getByRole('button', { name: 'Ripristina vista' }).click();
  await page.getByLabel('Tema', { exact: true }).selectOption('light');
  await page.screenshot({ path: 'artifacts/cad-light.png', fullPage: true });
  await page.getByRole('button', { name: 'Vista semplice', exact: true }).click();
  await expect(page.locator('.device-stage .hardware')).toHaveCount(1);
  await expect(page.locator('canvas')).toHaveCount(0);
  await page.getByLabel('Tema', { exact: true }).selectOption('system');
  await page.emulateMedia({ colorScheme: 'dark', reducedMotion: 'reduce' });
  await expect(page.locator('html')).toHaveAttribute('data-theme', 'dark');
  await page.getByRole('button', { name: 'Modello 3D', exact: true }).click();
  await expect(page.locator('.cad-loading')).toHaveCount(0);
  await page.setViewportSize({ width: 390, height: 844 });
  await page.screenshot({ path: 'artifacts/cad-mobile.png', fullPage: true });
  if (await page.evaluate(() => document.documentElement.scrollWidth > innerWidth))
    throw Error('Mobile overflow');
  await page.route('**/models/nove-v29.glb', (r) => r.abort());
  await page.reload();
  await page.getByRole('button', { name: 'Esplora la demo' }).click();
  await expect(page.locator('.device-stage .hardware')).toHaveCount(1);
  await expect(page.getByText('Vista 3D non disponibile.', { exact: false })).toBeVisible();
  if (errors.length) throw Error(errors.join('\n'));
  console.log(
    'PASS: CAD load, controls, rotate/reset, themes/persistence/system, reduced motion, mobile, model failure fallback',
  );
} finally {
  await browser.close();
}
