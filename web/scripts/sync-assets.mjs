import { copyFile, mkdir, readFile, rm, stat } from 'node:fs/promises';
import { createReadStream, createWriteStream } from 'node:fs';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { PNG } from 'pngjs';

const here = dirname(fileURLToPath(import.meta.url));
const webRoot = resolve(here, '..');
const gameRoot = resolve(webRoot, '..');
const version = (await readFile(resolve(gameRoot, 'VERSION'), 'utf8')).trim();
const animationManifest = await readFile(resolve(gameRoot, 'assets/pilgrim_animation_clips.txt'), 'utf8');

if (!/^\d+\.\d+\.\d+(?:[+-][0-9A-Za-z.-]+)?$/.test(version)) {
    throw new Error(`Invalid semantic version in VERSION: ${version}`);
}

const copies = [
    ['assets/cathedral_concept.png', 'public/art/cathedral-concept.png'],
    ['assets/cathedral_background.png', 'public/art/cathedral-background.png'],
    ['assets/pilgrim_idle_breath_source.png', 'public/art/pilgrim-idle.png'],
];

await rm(resolve(webRoot, 'public/downloads'), { recursive: true, force: true });

for (const [from, to] of copies) {
    const source = resolve(gameRoot, from);
    const target = resolve(webRoot, to);

    try {
        await stat(source);
    } catch {
        throw new Error(`Missing ${from}. Run "npm run build:game" from web/ first.`);
    }

    await mkdir(dirname(target), { recursive: true });
    await copyFile(source, target);
}

await chromaKeyGreen(
    resolve(gameRoot, activeClipSheet(animationManifest, 'walk')),
    resolve(webRoot, 'public/art/pilgrim-walk-keyed.png'),
);

console.log(`Synced game art v${version} into web/public.`);

function activeClipSheet(manifest, animation) {
    const clips = new Map();
    const active = new Map();

    for (const raw of manifest.split('\n')) {
        const parts = raw.trim().split(/\s+/);
        if (!parts[0] || parts[0].startsWith('#')) continue;
        if (parts[0] === 'clip') clips.set(parts[2], { animation: parts[1], sheet: parts[3] });
        if (parts[0] === 'active') active.set(parts[1], parts[2]);
    }

    const clip = clips.get(active.get(animation));
    if (!clip || clip.animation !== animation) throw new Error(`No active ${animation} clip in animation manifest.`);
    return clip.sheet;
}

async function chromaKeyGreen(source, target) {
    await mkdir(dirname(target), { recursive: true });

    return new Promise((resolvePromise, reject) => {
        createReadStream(source)
            .pipe(new PNG())
            .on('parsed', function parsed() {
                for (let y = 0; y < this.height; y += 1) {
                    for (let x = 0; x < this.width; x += 1) {
                        const index = (this.width * y + x) << 2;
                        const red = this.data[index];
                        const green = this.data[index + 1];
                        const blue = this.data[index + 2];

                        if (green > 160 && red < 90 && blue < 90) {
                            this.data[index + 3] = 0;
                        }
                    }
                }

                this.pack().pipe(createWriteStream(target)).on('finish', resolvePromise).on('error', reject);
            })
            .on('error', reject);
    });
}
