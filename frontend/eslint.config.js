import js from '@eslint/js'
import svelte from 'eslint-plugin-svelte'
import globals from 'globals'

export default [
  { ignores: ['dist/', 'node_modules/'] },
  js.configs.recommended,
  ...svelte.configs.recommended,
  {
    languageOptions: {
      globals: { ...globals.browser },
    },
  },
  {
    files: ['vite.config.js', 'svelte.config.js', 'eslint.config.js'],
    languageOptions: {
      globals: { ...globals.node },
    },
  },
]
