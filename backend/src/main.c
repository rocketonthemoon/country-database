// Country guessing game - HTTP API server.
// Mongoose (embedded web server) + SQLite.
//
// Paths are fixed and relative to the working directory, so no file path
// ever comes from the environment or a request:
//   ./public/        Svelte static export (vite build output); API only
//                    if it has no index.html
//   ./data/game.db   SQLite database (./data is created if missing)
// Country data is compiled in (countries.h) and inserted with prepared
// statements; no SQL text is read from disk.
//
// Environment:
//   PORT        listen port, 1-65535       (default 8000)

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <sqlite3.h>

#include "countries.h"
#include "mongoose.h"

#define PUBLIC_DIR "public"
#define INDEX_PATH PUBLIC_DIR "/index.html"
#define DATA_DIR "data"
#define DB_PATH DATA_DIR "/game.db"
#define DEFAULT_PORT 8000

#define FLAG_ROUNDS 10
#define CLUE_ROUNDS 5
#define CLUE_COUNT 8
#define FLAG_OPTIONS 4
#define PLAYER_MAX_BYTES 40
#define LEADERBOARD_SIZE 10

static const char *JSON_HDR =
    "Content-Type: application/json\r\nCache-Control: no-store\r\n";

static sqlite3 *s_db;
static bool s_serve_static;
static volatile sig_atomic_t s_stop;

static const char *SCHEMA =
    "PRAGMA journal_mode=WAL;"
    "CREATE TABLE IF NOT EXISTS countries ("
    "  id INTEGER PRIMARY KEY,"
    "  code TEXT NOT NULL UNIQUE,"
    "  name TEXT NOT NULL UNIQUE COLLATE NOCASE,"
    "  capital TEXT, region TEXT, subregion TEXT, area REAL,"
    "  languages TEXT, currencies TEXT,"
    "  landlocked INTEGER NOT NULL DEFAULT 0,"
    "  borders INTEGER NOT NULL DEFAULT 0);"
    "CREATE TABLE IF NOT EXISTS scores ("
    "  id INTEGER PRIMARY KEY,"
    "  player TEXT NOT NULL,"
    "  mode TEXT NOT NULL CHECK (mode IN ('flag', 'clue')),"
    "  score INTEGER NOT NULL,"
    "  created_at TEXT NOT NULL DEFAULT (datetime('now')));"
    "CREATE INDEX IF NOT EXISTS scores_mode_score ON scores (mode, score DESC);";

// ---------------------------------------------------------------- helpers

static void on_signal(int sig) {
  (void) sig;
  s_stop = 1;
}

// Parses a decimal int in [min, max]; returns dflt on any error.
static int parse_int(const char *s, int min, int max, int dflt) {
  char *end = NULL;
  long v;
  if (s == NULL || *s == '\0') return dflt;
  v = strtol(s, &end, 10);
  if (end == NULL || *end != '\0' || v < min || v > max) return dflt;
  return (int) v;
}

// Uniform random int in [0, bound) from the OS CSPRNG (via Mongoose).
static int random_below(int bound) {
  unsigned int r = 0;
  if (bound <= 1 || !mg_random(&r, sizeof(r))) return 0;
  return (int) (r % (unsigned int) bound);
}

static bool file_exists(const char *path) {
  FILE *fp = fopen(path, "rb");
  if (fp == NULL) return false;
  fclose(fp);
  return true;
}

static const char *col_text(sqlite3_stmt *st, int i) {
  const unsigned char *s = sqlite3_column_text(st, i);
  return s != NULL && *s != '\0' ? (const char *) s : "Unknown";
}

static void reply_error(struct mg_connection *c, int code, const char *msg) {
  mg_http_reply(c, code, JSON_HDR, "{%m:%m}\n", MG_ESC("error"), MG_ESC(msg));
}

static void reply_iobuf(struct mg_connection *c, struct mg_iobuf *io) {
  mg_http_reply(c, 200, JSON_HDR, "%.*s\n", (int) io->len, (char *) io->buf);
  mg_iobuf_free(io);
}

static bool is_valid_mode(const char *mode) {
  return mode != NULL && (strcmp(mode, "flag") == 0 || strcmp(mode, "clue") == 0);
}

static long max_score_for(const char *mode) {
  return strcmp(mode, "flag") == 0 ? FLAG_ROUNDS : CLUE_ROUNDS * CLUE_COUNT;
}

// Trims ASCII whitespace in place; rejects empty, too long, or control chars.
static char *clean_player_name(char *s) {
  char *end;
  if (s == NULL) return NULL;
  while (*s == ' ' || *s == '\t') s++;
  end = s + strlen(s);
  while (end > s && (end[-1] == ' ' || end[-1] == '\t')) *--end = '\0';
  if (*s == '\0' || strlen(s) > PLAYER_MAX_BYTES) return NULL;
  for (const unsigned char *p = (const unsigned char *) s; *p; p++) {
    if (*p < 0x20 || *p == 0x7f) return NULL;
  }
  return s;
}

// ---------------------------------------------------------------- database

// Inserts the compiled-in countries in one transaction, using a prepared
// statement with bound parameters.
static bool seed_countries(void) {
  const size_t n = sizeof(SEED_COUNTRIES) / sizeof(SEED_COUNTRIES[0]);
  sqlite3_stmt *st = NULL;
  bool ok = true;

  if (sqlite3_exec(s_db, "BEGIN", NULL, NULL, NULL) != SQLITE_OK) return false;
  if (sqlite3_prepare_v2(s_db,
                         "INSERT INTO countries (code, name, capital, region,"
                         " subregion, area, languages, currencies, landlocked,"
                         " borders) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                         -1, &st, NULL) != SQLITE_OK) {
    ok = false;
  }
  for (size_t i = 0; ok && i < n; i++) {
    const struct seed_country *r = &SEED_COUNTRIES[i];
    sqlite3_bind_text(st, 1, r->code, -1, SQLITE_STATIC);
    sqlite3_bind_text(st, 2, r->name, -1, SQLITE_STATIC);
    sqlite3_bind_text(st, 3, r->capital, -1, SQLITE_STATIC);
    sqlite3_bind_text(st, 4, r->region, -1, SQLITE_STATIC);
    sqlite3_bind_text(st, 5, r->subregion, -1, SQLITE_STATIC);
    sqlite3_bind_double(st, 6, r->area);
    sqlite3_bind_text(st, 7, r->languages, -1, SQLITE_STATIC);
    sqlite3_bind_text(st, 8, r->currencies, -1, SQLITE_STATIC);
    sqlite3_bind_int(st, 9, r->landlocked);
    sqlite3_bind_int(st, 10, r->borders);
    ok = sqlite3_step(st) == SQLITE_DONE;
    sqlite3_reset(st);
  }
  sqlite3_finalize(st);
  if (!ok) {
    MG_ERROR(("seed: %s", sqlite3_errmsg(s_db)));
    sqlite3_exec(s_db, "ROLLBACK", NULL, NULL, NULL);
    return false;
  }
  if (sqlite3_exec(s_db, "COMMIT", NULL, NULL, NULL) != SQLITE_OK) return false;
  MG_INFO(("seeded %d countries", (int) n));
  return true;
}

static bool db_init(void) {
  sqlite3_stmt *st;
  int count = 0;
  char *err = NULL;

  if (mkdir(DATA_DIR, 0750) != 0 && errno != EEXIST) {
    MG_ERROR(("cannot create %s/: %s", DATA_DIR, strerror(errno)));
    return false;
  }
  if (sqlite3_open_v2(DB_PATH, &s_db,
                      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                      NULL) != SQLITE_OK) {
    MG_ERROR(("open %s: %s", DB_PATH, sqlite3_errmsg(s_db)));
    return false;
  }
  sqlite3_busy_timeout(s_db, 2000);
  if (sqlite3_exec(s_db, SCHEMA, NULL, NULL, &err) != SQLITE_OK) {
    MG_ERROR(("schema: %s", err));
    sqlite3_free(err);
    return false;
  }

  if (sqlite3_prepare_v2(s_db, "SELECT COUNT(*) FROM countries", -1, &st,
                         NULL) == SQLITE_OK &&
      sqlite3_step(st) == SQLITE_ROW) {
    count = sqlite3_column_int(st, 0);
  }
  sqlite3_finalize(st);

  return count > 0 || seed_countries();
}

// ---------------------------------------------------------------- handlers

// GET /api/countries -> ["Afghanistan", ...]  (for autocomplete)
static void handle_countries(struct mg_connection *c) {
  struct mg_iobuf io = {NULL, 0, 0, 4096};
  sqlite3_stmt *st;
  bool first = true;

  if (sqlite3_prepare_v2(s_db, "SELECT name FROM countries ORDER BY name", -1,
                         &st, NULL) != SQLITE_OK) {
    reply_error(c, 500, "database error");
    return;
  }
  mg_xprintf(mg_pfn_iobuf, &io, "[");
  while (sqlite3_step(st) == SQLITE_ROW) {
    mg_xprintf(mg_pfn_iobuf, &io, "%s%m", first ? "" : ",",
               MG_ESC(col_text(st, 0)));
    first = false;
  }
  mg_xprintf(mg_pfn_iobuf, &io, "]");
  sqlite3_finalize(st);
  reply_iobuf(c, &io);
}

// GET /api/questions/flag -> {qid, flag, options: [4 names]}
static void handle_flag_question(struct mg_connection *c) {
  char names[FLAG_OPTIONS][128];
  char flag_url[64] = "";
  int n = 0, qid = 0;
  sqlite3_stmt *st;
  struct mg_iobuf io = {NULL, 0, 0, 256};

  if (sqlite3_prepare_v2(s_db,
                         "SELECT id, code, name FROM countries "
                         "ORDER BY RANDOM() LIMIT ?",
                         -1, &st, NULL) != SQLITE_OK) {
    reply_error(c, 500, "database error");
    return;
  }
  sqlite3_bind_int(st, 1, FLAG_OPTIONS);
  while (n < FLAG_OPTIONS && sqlite3_step(st) == SQLITE_ROW) {
    if (n == 0) {  // first random row is the answer
      qid = sqlite3_column_int(st, 0);
      mg_snprintf(flag_url, sizeof(flag_url), "https://flagcdn.com/w320/%s.png",
                  col_text(st, 1));
    }
    mg_snprintf(names[n], sizeof(names[n]), "%s", col_text(st, 2));
    n++;
  }
  sqlite3_finalize(st);
  if (n < FLAG_OPTIONS) {
    reply_error(c, 500, "not enough countries");
    return;
  }

  for (int i = n - 1; i > 0; i--) {  // Fisher-Yates shuffle
    int j = random_below(i + 1);
    char tmp[128];
    memcpy(tmp, names[i], sizeof(tmp));
    memcpy(names[i], names[j], sizeof(tmp));
    memcpy(names[j], tmp, sizeof(tmp));
  }

  mg_xprintf(mg_pfn_iobuf, &io, "{%m:%d,%m:%m,%m:[", MG_ESC("qid"), qid,
             MG_ESC("flag"), MG_ESC(flag_url), MG_ESC("options"));
  for (int i = 0; i < n; i++) {
    mg_xprintf(mg_pfn_iobuf, &io, "%s%m", i ? "," : "", MG_ESC(names[i]));
  }
  mg_xprintf(mg_pfn_iobuf, &io, "]}");
  reply_iobuf(c, &io);
}

// GET /api/questions/clue -> {qid, total}
static void handle_clue_question(struct mg_connection *c) {
  sqlite3_stmt *st;
  int qid = 0;
  if (sqlite3_prepare_v2(s_db,
                         "SELECT id FROM countries ORDER BY RANDOM() LIMIT 1",
                         -1, &st, NULL) == SQLITE_OK &&
      sqlite3_step(st) == SQLITE_ROW) {
    qid = sqlite3_column_int(st, 0);
  }
  sqlite3_finalize(st);
  if (qid == 0) {
    reply_error(c, 500, "database error");
    return;
  }
  mg_http_reply(c, 200, JSON_HDR, "{%m:%d,%m:%d}\n", MG_ESC("qid"), qid,
                MG_ESC("total"), CLUE_COUNT);
}

// GET /api/clue?qid=N&n=K -> {n, label, value, type}
// Clues go from vague to obvious; the client scores by how many it needed.
static void handle_clue(struct mg_connection *c, struct mg_http_message *hm) {
  char buf[16], value[256];
  const char *label, *type = "text";
  int qid, n;
  sqlite3_stmt *st;

  mg_http_get_var(&hm->query, "qid", buf, sizeof(buf));
  qid = parse_int(buf, 1, 0x7fffffff, -1);
  mg_http_get_var(&hm->query, "n", buf, sizeof(buf));
  n = parse_int(buf, 0, CLUE_COUNT - 1, -1);
  if (qid < 0 || n < 0) {
    reply_error(c, 400, "bad qid or clue number");
    return;
  }

  if (sqlite3_prepare_v2(
          s_db,
          "SELECT code, region, subregion,"
          "  CASE WHEN area < 10 THEN printf('%.2f', area)"
          "       ELSE format('%,d', CAST(area AS INTEGER)) END,"
          "  languages, currencies, capital, landlocked, borders "
          "FROM countries WHERE id = ?",
          -1, &st, NULL) != SQLITE_OK) {
    reply_error(c, 500, "database error");
    return;
  }
  sqlite3_bind_int(st, 1, qid);
  if (sqlite3_step(st) != SQLITE_ROW) {
    sqlite3_finalize(st);
    reply_error(c, 404, "unknown question");
    return;
  }

  switch (n) {
    case 0:
      label = "Region";
      mg_snprintf(value, sizeof(value), "%s", col_text(st, 1));
      break;
    case 1:
      label = "Area";
      mg_snprintf(value, sizeof(value), "%s km\xc2\xb2", col_text(st, 3));
      break;
    case 2: {
      int landlocked = sqlite3_column_int(st, 7);
      int borders = sqlite3_column_int(st, 8);
      const char *plural = borders == 1 ? "country" : "countries";
      label = "Geography";
      if (borders == 0) {
        mg_snprintf(value, sizeof(value), "No land borders");
      } else {
        mg_snprintf(value, sizeof(value), "%s, borders %d %s",
                    landlocked ? "Landlocked" : "Has a coastline", borders,
                    plural);
      }
      break;
    }
    case 3:
      label = "Languages";
      mg_snprintf(value, sizeof(value), "%s", col_text(st, 4));
      break;
    case 4:
      label = "Currency";
      mg_snprintf(value, sizeof(value), "%s", col_text(st, 5));
      break;
    case 5:
      label = "Subregion";
      mg_snprintf(value, sizeof(value), "%s", col_text(st, 2));
      break;
    case 6:
      label = "Capital";
      mg_snprintf(value, sizeof(value), "%s", col_text(st, 6));
      break;
    default:
      label = "Flag";
      type = "flag";
      mg_snprintf(value, sizeof(value), "https://flagcdn.com/w320/%s.png",
                  col_text(st, 0));
      break;
  }
  sqlite3_finalize(st);

  mg_http_reply(c, 200, JSON_HDR, "{%m:%d,%m:%m,%m:%m,%m:%m}\n", MG_ESC("n"),
                n, MG_ESC("label"), MG_ESC(label), MG_ESC("value"),
                MG_ESC(value), MG_ESC("type"), MG_ESC(type));
}

// POST /api/answer {qid, guess, reveal} -> {correct, answer?}
// The answer is only disclosed when correct or when reveal is true.
static void handle_answer(struct mg_connection *c, struct mg_http_message *hm) {
  long qid = mg_json_get_long(hm->body, "$.qid", 0);
  char *guess = mg_json_get_str(hm->body, "$.guess");
  bool reveal = false, correct;
  char answer[128];
  sqlite3_stmt *st;

  mg_json_get_bool(hm->body, "$.reveal", &reveal);
  if (qid <= 0) {
    free(guess);
    reply_error(c, 400, "missing qid");
    return;
  }
  if (sqlite3_prepare_v2(s_db,
                         "SELECT name, name = trim(?2) FROM countries "
                         "WHERE id = ?1",
                         -1, &st, NULL) != SQLITE_OK) {
    free(guess);
    reply_error(c, 500, "database error");
    return;
  }
  sqlite3_bind_int64(st, 1, qid);
  sqlite3_bind_text(st, 2, guess ? guess : "", -1, SQLITE_TRANSIENT);
  free(guess);
  if (sqlite3_step(st) != SQLITE_ROW) {
    sqlite3_finalize(st);
    reply_error(c, 404, "unknown question");
    return;
  }
  mg_snprintf(answer, sizeof(answer), "%s", col_text(st, 0));
  correct = sqlite3_column_int(st, 1) == 1;
  sqlite3_finalize(st);

  if (correct || reveal) {
    mg_http_reply(c, 200, JSON_HDR, "{%m:%s,%m:%m}\n", MG_ESC("correct"),
                  correct ? "true" : "false", MG_ESC("answer"), MG_ESC(answer));
  } else {
    mg_http_reply(c, 200, JSON_HDR, "{%m:false}\n", MG_ESC("correct"));
  }
}

// GET /api/scores?mode=flag|clue -> [{player, score, created_at}]
static void handle_get_scores(struct mg_connection *c,
                              struct mg_http_message *hm) {
  char mode[8];
  struct mg_iobuf io = {NULL, 0, 0, 512};
  sqlite3_stmt *st;
  bool first = true;

  mg_http_get_var(&hm->query, "mode", mode, sizeof(mode));
  if (!is_valid_mode(mode)) {
    reply_error(c, 400, "mode must be flag or clue");
    return;
  }
  if (sqlite3_prepare_v2(s_db,
                         "SELECT player, score, created_at FROM scores "
                         "WHERE mode = ? ORDER BY score DESC, id ASC LIMIT ?",
                         -1, &st, NULL) != SQLITE_OK) {
    reply_error(c, 500, "database error");
    return;
  }
  sqlite3_bind_text(st, 1, mode, -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(st, 2, LEADERBOARD_SIZE);
  mg_xprintf(mg_pfn_iobuf, &io, "[");
  while (sqlite3_step(st) == SQLITE_ROW) {
    mg_xprintf(mg_pfn_iobuf, &io, "%s{%m:%m,%m:%d,%m:%m}", first ? "" : ",",
               MG_ESC("player"), MG_ESC(col_text(st, 0)), MG_ESC("score"),
               sqlite3_column_int(st, 1), MG_ESC("created_at"),
               MG_ESC(col_text(st, 2)));
    first = false;
  }
  mg_xprintf(mg_pfn_iobuf, &io, "]");
  sqlite3_finalize(st);
  reply_iobuf(c, &io);
}

// POST /api/scores {player, mode, score} -> {rank}
static void handle_post_score(struct mg_connection *c,
                              struct mg_http_message *hm) {
  char *raw_player = mg_json_get_str(hm->body, "$.player");
  char *mode = mg_json_get_str(hm->body, "$.mode");
  long score = mg_json_get_long(hm->body, "$.score", -1);
  char *player = clean_player_name(raw_player);
  sqlite3_stmt *st;
  int rank = 0;

  if (player == NULL) {
    reply_error(c, 400, "invalid player name");
  } else if (!is_valid_mode(mode)) {
    reply_error(c, 400, "mode must be flag or clue");
  } else if (score < 0 || score > max_score_for(mode)) {
    reply_error(c, 400, "score out of range");
  } else if (sqlite3_prepare_v2(s_db,
                                "INSERT INTO scores (player, mode, score) "
                                "VALUES (?, ?, ?)",
                                -1, &st, NULL) != SQLITE_OK) {
    reply_error(c, 500, "database error");
  } else {
    sqlite3_bind_text(st, 1, player, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, mode, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st, 3, score);
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    if (rc != SQLITE_DONE) {
      reply_error(c, 500, "database error");
    } else {
      if (sqlite3_prepare_v2(s_db,
                             "SELECT COUNT(*) + 1 FROM scores "
                             "WHERE mode = ? AND score > ?",
                             -1, &st, NULL) == SQLITE_OK) {
        sqlite3_bind_text(st, 1, mode, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(st, 2, score);
        if (sqlite3_step(st) == SQLITE_ROW) rank = sqlite3_column_int(st, 0);
      }
      sqlite3_finalize(st);
      mg_http_reply(c, 201, JSON_HDR, "{%m:%d}\n", MG_ESC("rank"), rank);
    }
  }
  free(raw_player);
  free(mode);
}

// ---------------------------------------------------------------- routing

static bool is_method(struct mg_http_message *hm, const char *m) {
  return mg_strcmp(hm->method, mg_str(m)) == 0;
}

static bool is_route(struct mg_http_message *hm, const char *path) {
  return mg_match(hm->uri, mg_str(path), NULL);
}

// Serves the Svelte static export (vite build output) from ./public.
//  - /assets/* are content-hashed by Vite: cached for a year, real 404s.
//  - anything else that doesn't exist falls back to index.html (SPA).
//  - precompressed *.gz siblings are sent when the client accepts gzip.
static void serve_static(struct mg_connection *c, struct mg_http_message *hm) {
  struct mg_http_serve_opts opts = {.root_dir = PUBLIC_DIR};
  if (mg_match(hm->uri, mg_str("/assets/#"), NULL)) {
    opts.extra_headers = "Cache-Control: public, max-age=31536000, immutable\r\n";
  } else {
    opts.extra_headers = "Cache-Control: no-cache\r\n";
    opts.page404 = INDEX_PATH;
  }
  mg_http_serve_dir(c, hm, &opts);
}

static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
  struct mg_http_message *hm;
  if (ev != MG_EV_HTTP_MSG) return;
  hm = (struct mg_http_message *) ev_data;

  if (is_route(hm, "/api/health")) {
    mg_http_reply(c, 200, JSON_HDR, "{%m:true}\n", MG_ESC("ok"));
  } else if (is_route(hm, "/api/countries") && is_method(hm, "GET")) {
    handle_countries(c);
  } else if (is_route(hm, "/api/questions/flag") && is_method(hm, "GET")) {
    handle_flag_question(c);
  } else if (is_route(hm, "/api/questions/clue") && is_method(hm, "GET")) {
    handle_clue_question(c);
  } else if (is_route(hm, "/api/clue") && is_method(hm, "GET")) {
    handle_clue(c, hm);
  } else if (is_route(hm, "/api/answer") && is_method(hm, "POST")) {
    handle_answer(c, hm);
  } else if (is_route(hm, "/api/scores") && is_method(hm, "GET")) {
    handle_get_scores(c, hm);
  } else if (is_route(hm, "/api/scores") && is_method(hm, "POST")) {
    handle_post_score(c, hm);
  } else if (s_serve_static && !is_route(hm, "/api/#")) {
    serve_static(c, hm);
  } else {
    reply_error(c, 404, "not found");
  }
}

int main(void) {
  char url[64];
  struct mg_mgr mgr;
  // PORT is only used as a number; anything else falls back to the default.
  int port = parse_int(getenv("PORT"), 1, 65535, DEFAULT_PORT);  // flawfinder: ignore

  mg_log_set(MG_LL_INFO);
  signal(SIGINT, on_signal);
  signal(SIGTERM, on_signal);

  s_serve_static = file_exists(INDEX_PATH);
  if (!s_serve_static) MG_INFO(("%s not found - serving API only", INDEX_PATH));
  if (!db_init()) return EXIT_FAILURE;

  mg_snprintf(url, sizeof(url), "http://0.0.0.0:%d", port);
  mg_mgr_init(&mgr);
  if (mg_http_listen(&mgr, url, ev_handler, NULL) == NULL) {
    MG_ERROR(("cannot listen on %s", url));
    return EXIT_FAILURE;
  }
  MG_INFO(("listening on %s, static: %s", url, s_serve_static ? PUBLIC_DIR "/" : "off"));
  while (!s_stop) mg_mgr_poll(&mgr, 1000);

  MG_INFO(("shutting down"));
  mg_mgr_free(&mgr);
  sqlite3_close(s_db);
  return EXIT_SUCCESS;
}
