# Guess the Country

A country guessing game with two modes:

- **Flag quiz**: 10 flags with 4 choices each, one point per correct answer.
- **Clue hunt**: 5 countries. Clues go from vague (region) to obvious (flag). A correct guess on the first clue earns 8 points and on the last earns 1. A wrong guess reveals the next clue.

Each mode has its own leaderboard, stored in SQLite.

| Layer    | Tech |
|----------|------|
| Frontend | Svelte 5 + Vite, built as a static export (`frontend/`) |
| Backend  | C, [Mongoose](https://github.com/cesanta/mongoose) 7.23 embedded web server (`backend/`) |
| Database | SQLite (`/app/data/game.db` in the container) |

## How it is served

There is one process and one port. The C server answers `/api/*` and serves the Svelte static export (`vite build` output) for everything else:

```
browser ──► Mongoose (C) :8000
              ├── /api/*       → JSON handlers → SQLite
              └── everything   → frontend/dist (static export)
```

- `/assets/*` files are content-hashed by Vite and sent with `Cache-Control: immutable` (1 year). A missing asset returns a real 404.
- `index.html` is sent with `Cache-Control: no-cache`, so a new deploy shows up immediately.
- Unknown non-asset paths fall back to `index.html` (SPA fallback).
- `npm run build` also writes `.gz` copies of each file. Mongoose sends them to clients that accept gzip, so nothing is compressed per request.

The server uses fixed paths relative to its working directory. No file path comes from an environment variable or a request:

| Path | Contents |
|------|----------|
| `./public/` | Svelte static export. Without an `index.html`, the server runs API-only. |
| `./data/game.db` | SQLite database. `./data` is created if missing. |

The only setting is `PORT` (1–65535, default 8000). Country data is compiled into the binary (`backend/src/countries.h`) and inserted with prepared statements on first start. The server never runs SQL text read from disk.

## Run with Docker / Podman

```sh
docker compose up --build        # or: podman compose up --build
```

Then open http://localhost:8000. The leaderboard is stored in the `game-data` volume. `docker compose down -v` resets it.

The image runs as a non-root user (UID 10001), and the C binary is built with hardening flags (FORTIFY_SOURCE, stack protector, PIE, full RELRO).

## Local development

Backend (Linux/macOS/WSL, needs `gcc`, `make` and `libsqlite3-dev`):

```sh
cd backend
make run                          # hardened build, links ../frontend/dist as ./public, serves :8000
```

Frontend, with hot reload. Vite proxies `/api` to the backend on port 8000:

```sh
cd frontend
npm install
npm run dev                       # http://localhost:5173
npm run build                     # static export -> dist/
```

## CI

[`.github/workflows/ci.yml`](.github/workflows/ci.yml) runs on every push and pull request. Actions are pinned to commit SHAs, and scanner images are pinned to exact versions.

| Job | What it checks |
|-----|----------------|
| Frontend lint | ESLint, `svelte-check` (JSDoc types), production build |
| Dependency audit | `npm audit --audit-level=high`, `npm audit signatures` |
| Backend scan | `-Werror` hardened build, `gcc -fanalyzer`, cppcheck, clang-tidy (clang static analyzer, bugprone, CERT), flawfinder |
| Backend sanitizers | ASan + UBSan build, then the [smoke test](tests/smoke.sh) against it |
| CodeQL | `security-extended` queries for C and JavaScript |
| hadolint | Dockerfile lint ([config](.hadolint.yaml)) |
| Trivy fs | Vulnerabilities (including dev dependencies), secrets, and misconfigurations in the repo |
| Container | Image build, smoke test of the running container (API and static export), non-root check, Trivy image scan |

To run the same checks locally:

```sh
cd frontend && npm run lint && npm run check && npm audit
cd backend  && make scan && make sanitize
BASE_URL=http://localhost:8000 sh tests/smoke.sh
docker run --rm -v "$PWD:/w" -w /w hadolint/hadolint:v2.15.1-alpine hadolint --config .hadolint.yaml Dockerfile
docker run --rm -v "$PWD:/repo" aquasec/trivy:0.76.1 fs --include-dev-deps --skip-dirs /repo/frontend/node_modules /repo
```

## API

| Method | Path | Description |
|--------|------|-------------|
| GET  | `/api/health` | Liveness check |
| GET  | `/api/countries` | All country names (for autocomplete) |
| GET  | `/api/questions/flag` | `{qid, flag, options[4]}` |
| GET  | `/api/questions/clue` | `{qid, total}` |
| GET  | `/api/clue?qid=&n=` | Clue *n* (0 = hardest) for a question |
| POST | `/api/answer` | `{qid, guess, reveal}` → `{correct, answer?}` |
| GET  | `/api/scores?mode=flag\|clue` | Top 10 |
| POST | `/api/scores` | `{player, mode, score}` → `{rank}` |

## Data

Country data comes from [mledoze/countries](https://github.com/mledoze/countries) 5.1.0 (ODbL-1.0). It covers 194 independent countries. Flags come from [flagcdn.com](https://flagcdn.com). To regenerate the compiled-in data:

```sh
curl -L -o countries.json https://raw.githubusercontent.com/mledoze/countries/5.1.0/countries.json
node tools/make-seed.mjs countries.json > backend/src/countries.h
```

## License note

Mongoose is dual-licensed under GPLv2 or a commercial license from Cesanta (see `backend/vendor/LICENSE`).

The original v1 country search app (`index.html`, `js/`, `css/`) is kept in the repo root, and the game reuses its design. Its data source, the restcountries.com v3.1 API, has been shut down, so the v1 app no longer loads data.
