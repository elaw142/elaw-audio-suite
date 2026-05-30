import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));

export default defineConfig({
  base: "./",
  plugins: [react()],
  resolve: {
    alias: {
      "@elaw/ui": path.resolve(__dirname, "../../../shared/web")
    }
  },
  server: {
    fs: {
      allow: [path.resolve(__dirname, "../../..")]
    }
  },
  build: {
    outDir: "dist",
    emptyOutDir: true
  }
});
