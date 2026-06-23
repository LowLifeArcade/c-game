# Pilgrim of the Thorn Web

Cloudflare Worker + Vite landing page for the macOS build of Pilgrim of the Thorn.

## Commands

```sh
npm install
npm run dev
npm run build
npm run deploy
```

`npm run deploy` works here and from the repository root. It rebuilds and packages the game with `make package-macos`, copies the current versioned DMG and game art into `public/`, builds the Vite app, and deploys it with Wrangler.

The game version comes from the repository's root `VERSION` file. The hero download and `/api/download` Worker redirect both point to the matching versioned DMG.
