// Writes a .gz sibling for every text file in dist/. The Mongoose server
// sends these automatically to clients that accept gzip, so the static
// export is served compressed without compressing on every request.
import { readdirSync, readFileSync, writeFileSync, statSync } from 'node:fs'
import { join } from 'node:path'
import { fileURLToPath } from 'node:url'
import { gzipSync, constants } from 'node:zlib'

const DIST = fileURLToPath(new URL('../dist/', import.meta.url))
const TEXT = /\.(html|js|css|svg|json|txt|map)$/

/** @param {string} dir @returns {string[]} */
const walk = (dir) =>
  readdirSync(dir).flatMap((name) => {
    const path = join(dir, name)
    return statSync(path).isDirectory() ? walk(path) : [path]
  })

let count = 0
for (const file of walk(DIST)) {
  if (!TEXT.test(file)) continue
  const gz = gzipSync(readFileSync(file), { level: constants.Z_BEST_COMPRESSION })
  writeFileSync(`${file}.gz`, gz)
  count++
}
console.log(`precompressed ${count} files in dist/`)
