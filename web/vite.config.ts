import { cloudflare } from "@cloudflare/vite-plugin";
import { readFileSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { defineConfig } from "vite";

const here = dirname(fileURLToPath(import.meta.url));
const gameVersion = readFileSync(resolve(here, "../VERSION"), "utf8").trim();

export default defineConfig({
  plugins: [cloudflare()],
  define: {
    __PILGRIM_VERSION__: JSON.stringify(gameVersion)
  },
  server: {
    host: "127.0.0.1",
    port: 5173
  }
});
