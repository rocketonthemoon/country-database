// Thin wrapper around the C backend's JSON API.

/** @typedef {'flag' | 'clue'} Mode */
/** @typedef {{ qid: number, flag: string, options: string[] }} FlagQuestion */
/** @typedef {{ qid: number, total: number }} ClueQuestion */
/** @typedef {{ n: number, label: string, value: string, type: 'text' | 'flag' }} Clue */
/** @typedef {{ correct: boolean, answer?: string }} AnswerResult */
/** @typedef {{ player: string, score: number, created_at: string }} ScoreRow */

/**
 * @template T
 * @param {string} path
 * @param {RequestInit} [options]
 * @returns {Promise<T>}
 */
async function request(path, options = {}) {
  const res = await fetch(`/api${path}`, {
    headers: { 'Content-Type': 'application/json' },
    ...options,
  })
  const body = await res.json().catch(() => ({}))
  if (!res.ok) throw new Error(body.error ?? `Request failed (${res.status})`)
  return body
}

/** @param {unknown} e */
export const errorMessage = (e) => (e instanceof Error ? e.message : String(e))

export const FLAG_ROUNDS = 10
export const CLUE_ROUNDS = 5

export const api = {
  /** @returns {Promise<string[]>} */
  countries: () => request('/countries'),
  /** @returns {Promise<FlagQuestion>} */
  flagQuestion: () => request('/questions/flag'),
  /** @returns {Promise<ClueQuestion>} */
  clueQuestion: () => request('/questions/clue'),
  /**
   * @param {number} qid
   * @param {number} n
   * @returns {Promise<Clue>}
   */
  clue: (qid, n) => request(`/clue?qid=${qid}&n=${n}`),
  /**
   * @param {number} qid
   * @param {string} guess
   * @param {boolean} [reveal]
   * @returns {Promise<AnswerResult>}
   */
  answer: (qid, guess, reveal = false) =>
    request('/answer', {
      method: 'POST',
      body: JSON.stringify({ qid, guess, reveal }),
    }),
  /**
   * @param {Mode} mode
   * @returns {Promise<ScoreRow[]>}
   */
  scores: (mode) => request(`/scores?mode=${mode}`),
  /**
   * @param {string} player
   * @param {Mode} mode
   * @param {number} score
   * @returns {Promise<{ rank: number }>}
   */
  submitScore: (player, mode, score) =>
    request('/scores', {
      method: 'POST',
      body: JSON.stringify({ player, mode, score }),
    }),
}
