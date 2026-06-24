import { spawnSync } from 'node:child_process';
import { readFile, stat } from 'node:fs/promises';
import { basename, dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));
const webRoot = resolve(here, '..');
const gameRoot = resolve(webRoot, '..');
const target = process.argv[2];
const targets = {
    local: { bucket: 'pilgrim-of-the-thorn-downloads-prod', mode: '--local' },
    dev: { bucket: 'pilgrim-of-the-thorn-downloads-dev', mode: '--remote' },
    prod: { bucket: 'pilgrim-of-the-thorn-downloads-prod', mode: '--remote' },
};

if (!(target in targets)) {
    throw new Error('Usage: node scripts/upload-download.mjs <local|dev|prod>');
}

const version = (await readFile(resolve(gameRoot, 'VERSION'), 'utf8')).trim();
const filename = `Pilgrim-of-the-Thorn-macOS-v${version}.dmg`;
const source = resolve(gameRoot, 'dist/macos', filename);

try {
    await stat(source);
} catch {
    throw new Error(`Missing ${source}. Run "npm run build:game" first.`);
}

const { bucket, mode } = targets[target];
const wrangler = resolve(webRoot, 'node_modules/wrangler/bin/wrangler.js');
const result = spawnSync(
    process.execPath,
    [
        wrangler,
        'r2',
        'object',
        'put',
        `${bucket}/${filename}`,
        `--file=${source}`,
        '--content-type=application/x-apple-diskimage',
        `--content-disposition=attachment; filename="${basename(source)}"`,
        '--cache-control=private, max-age=0, must-revalidate',
        mode,
    ],
    { cwd: webRoot, stdio: 'inherit' },
);

if (result.status !== 0) {
    process.exit(result.status ?? 1);
}
