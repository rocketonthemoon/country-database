# syntax=docker/dockerfile:1

# ---- 1. Svelte static export ------------------------------------------------
# `npm run build` = vite build + precompress (.gz siblings for Mongoose).
FROM docker.io/library/node:24.11-alpine3.22 AS frontend
WORKDIR /src
COPY frontend/package.json frontend/package-lock.json ./
RUN npm ci --no-audit --no-fund
COPY frontend/ ./
RUN npm run build

# ---- 2. C backend (Mongoose + SQLite), hardened build ------------------------
FROM docker.io/library/alpine:3.22 AS backend
RUN apk add --no-cache build-base sqlite-dev
WORKDIR /src
COPY backend/Makefile ./
COPY backend/src ./src
COPY backend/vendor ./vendor
RUN make server

# ---- 3. Runtime: the C server serves the API and the static export ------------
FROM docker.io/library/alpine:3.22
RUN apk upgrade --no-cache \
 && apk add --no-cache sqlite-libs \
 && addgroup -S -g 10001 app \
 && adduser -S -u 10001 -G app -H -s /sbin/nologin app \
 && mkdir -p /data && chown 10001:10001 /data
WORKDIR /app
COPY --from=backend /src/server ./server
COPY backend/data/seed.sql ./data/seed.sql
COPY --from=frontend /src/dist ./public

ENV PORT=8000 \
    DB_PATH=/data/game.db \
    SEED_PATH=/app/data/seed.sql \
    PUBLIC_DIR=/app/public

USER 10001:10001
VOLUME ["/data"]
EXPOSE 8000
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s \
  CMD ["wget", "-qO", "/dev/null", "http://127.0.0.1:8000/api/health"]
CMD ["./server"]
