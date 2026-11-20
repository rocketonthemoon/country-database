#!/bin/sh
# End-to-end smoke test against a running server (local binary or container).
#   BASE_URL=http://localhost:8000 sh tests/smoke.sh
set -eu

BASE_URL="${BASE_URL:-http://localhost:8000}"
API="$BASE_URL/api"
fails=0

check() { # name, expected substring, actual
  case "$3" in
    *"$2"*) echo "ok   $1" ;;
    *) echo "FAIL $1: expected '$2' in: $3"; fails=$((fails + 1)) ;;
  esac
}

status() { curl -s -o /dev/null -w '%{http_code}' "$@"; }
header() { curl -s -D - -o /dev/null "$2" | tr -d '\r' | grep -i "^$1:" || true; }

# Wait for the server to come up.
i=0
until curl -sf "$API/health" >/dev/null 2>&1; do
  i=$((i + 1)); [ "$i" -gt 30 ] && { echo "server not reachable at $BASE_URL"; exit 1; }
  sleep 1
done

# ---- API
check "health" '"ok":true' "$(curl -s "$API/health")"
check "countries list" '"Afghanistan"' "$(curl -s "$API/countries")"

q=$(curl -s "$API/questions/flag")
check "flag question" '"options":[' "$q"
qid=$(echo "$q" | sed -E 's/.*"qid":([0-9]+).*/\1/')

check "clue 0" '"label":"Region"' "$(curl -s "$API/clue?qid=$qid&n=0")"
check "clue 7 is flag" '"type":"flag"' "$(curl -s "$API/clue?qid=$qid&n=7")"
check "clue out of range" '400' "$(status "$API/clue?qid=$qid&n=8")"
check "unknown question" '404' "$(status "$API/clue?qid=999999&n=0")"

check "wrong answer hides name" '{"correct":false}' \
  "$(curl -s -X POST "$API/answer" -d "{\"qid\":$qid,\"guess\":\"Narnia\"}")"
check "reveal shows name" '"answer":' \
  "$(curl -s -X POST "$API/answer" -d "{\"qid\":$qid,\"guess\":\"x\",\"reveal\":true}")"
check "case-insensitive answer" '"correct":true' \
  "$(curl -s -X POST "$API/answer" -d '{"qid":1,"guess":" afghanistan "}')"
check "malformed json" '400' "$(status -X POST "$API/answer" -d '{bad')"

check "save score" '"rank":' \
  "$(curl -s -X POST "$API/scores" -d '{"player":"smoke","mode":"flag","score":5}')"
check "reject score > max" '400' \
  "$(status -X POST "$API/scores" -d '{"player":"smoke","mode":"flag","score":11}')"
check "reject bad mode" '400' \
  "$(status -X POST "$API/scores" -d '{"player":"smoke","mode":"x","score":1}')"
check "reject empty name" '400' \
  "$(status -X POST "$API/scores" -d '{"player":"  ","mode":"clue","score":1}')"
sqli="{\"player\":\"x'); DROP TABLE scores;--\",\"mode\":\"clue\",\"score\":1}"
check "sql stays data" '"rank":' "$(curl -s -X POST "$API/scores" -d "$sqli")"
check "leaderboard" '"player":"smoke"' "$(curl -s "$API/scores?mode=flag")"
check "unknown api route" '404' "$(status "$API/nope")"

# ---- Svelte static export
check "index.html" '200' "$(status "$BASE_URL/")"
check "index is html" '<div id="app">' "$(curl -s "$BASE_URL/")"
check "index revalidates" 'no-cache' "$(header Cache-Control "$BASE_URL/")"
asset=$(curl -s "$BASE_URL/" | grep -oE '/assets/[^"]+\.js' | head -1)
check "hashed asset" '200' "$(status "$BASE_URL$asset")"
check "asset immutable" 'immutable' "$(header Cache-Control "$BASE_URL$asset")"
check "asset gzip" 'gzip' "$(curl -s -D - -o /dev/null -H 'Accept-Encoding: gzip' "$BASE_URL$asset" | tr -d '\r' | grep -i '^content-encoding:' || true)"
check "missing asset is 404" '404' "$(status "$BASE_URL/assets/missing.js")"
check "spa fallback" '<div id="app">' "$(curl -s "$BASE_URL/some/client/route")"
check "path traversal blocked" '40' "$(status --path-as-is "$BASE_URL/../../etc/passwd")"

if [ "$fails" -gt 0 ]; then
  echo "$fails check(s) failed"
  exit 1
fi
echo "all checks passed"
