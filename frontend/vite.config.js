import { defineConfig } from 'vite'
import { svelte } from '@sveltejs/vite-plugin-svelte'

// In dev, Vite serves the UI and forwards /api to the C backend.
// In the container, the backend serves the built files itself.
export default defineConfig({
  plugins: [svelte()],
  server: {
    proxy: {
      '/api': process.env.BACKEND_URL ?? 'http://localhost:8000',
    },
  },
})
