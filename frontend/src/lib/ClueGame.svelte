<script>
  import { api, errorMessage, CLUE_ROUNDS } from './api.js'
  import GameOver from './GameOver.svelte'

  /** @type {{ onexit: () => void }} */
  let { onexit } = $props()

  /** @type {string[]} */
  let countries = $state([])
  let round = $state(1)
  let score = $state(0)
  let qid = $state(0)
  let total = $state(8)
  /** @type {import('./api.js').Clue[]} */
  let clues = $state([])
  let guess = $state('')
  /** @type {string[]} */
  let wrongGuesses = $state([])
  /** @type {(import('./api.js').AnswerResult & { points: number }) | null} */
  let outcome = $state(null)
  let busy = $state(false)
  let error = $state('')
  let finished = $state(false)
  let showSuggestions = $state(false)
  let activeIndex = $state(-1)
  /** @type {HTMLInputElement | undefined} */
  let input = $state()

  const points = $derived(total - clues.length + 1)
  const maxScore = $derived(CLUE_ROUNDS * total)
  const known = $derived(new Set(countries.map((c) => c.toLowerCase())))
  // Same matching as the v1 search: substring, case-insensitive.
  const suggestions = $derived.by(() => {
    const q = guess.trim().toLowerCase()
    if (!q) return []
    return countries.filter((c) => c.toLowerCase().includes(q)).slice(0, 50)
  })

  async function revealNext() {
    if (clues.length >= total) return
    const clue = await api.clue(qid, clues.length)
    clues = [...clues, clue]
  }

  async function startRound() {
    error = ''
    busy = true
    clues = []
    wrongGuesses = []
    outcome = null
    guess = ''
    try {
      const q = await api.clueQuestion()
      qid = q.qid
      total = q.total
      await revealNext()
    } catch (e) {
      error = errorMessage(e)
    } finally {
      busy = false
      input?.focus()
    }
  }

  /** @param {Event} [event] */
  async function submitGuess(event) {
    event?.preventDefault()
    showSuggestions = false
    const value = guess.trim()
    if (!value || busy || outcome) return
    if (!known.has(value.toLowerCase())) {
      error = 'Pick a country from the list.'
      return
    }
    error = ''
    busy = true
    try {
      const res = await api.answer(qid, value, clues.length >= total)
      if (res.correct) {
        outcome = { ...res, points }
        score += points
      } else if (clues.length >= total) {
        outcome = { ...res, points: 0 } // out of clues, answer revealed
      } else {
        wrongGuesses = [...wrongGuesses, value]
        guess = ''
        await revealNext() // a wrong guess costs a clue
      }
    } catch (e) {
      error = errorMessage(e)
    } finally {
      busy = false
      input?.focus()
    }
  }

  /** @param {string} name */
  function pick(name) {
    guess = name
    submitGuess()
  }

  /** @param {KeyboardEvent} event */
  function onKeydown(event) {
    if (!showSuggestions || suggestions.length === 0) return
    if (event.key === 'ArrowDown') {
      event.preventDefault()
      activeIndex = (activeIndex + 1) % suggestions.length
    } else if (event.key === 'ArrowUp') {
      event.preventDefault()
      activeIndex = (activeIndex - 1 + suggestions.length) % suggestions.length
    } else if (event.key === 'Enter' && activeIndex >= 0) {
      event.preventDefault()
      pick(suggestions[activeIndex])
    } else if (event.key === 'Escape') {
      showSuggestions = false
    }
  }

  async function nextClue() {
    busy = true
    try {
      await revealNext()
    } catch (e) {
      error = errorMessage(e)
    } finally {
      busy = false
      input?.focus()
    }
  }

  async function giveUp() {
    busy = true
    try {
      const res = await api.answer(qid, '', true)
      outcome = { ...res, points: 0 }
    } catch (e) {
      error = errorMessage(e)
    } finally {
      busy = false
    }
  }

  function next() {
    if (round >= CLUE_ROUNDS) {
      finished = true
    } else {
      round += 1
      startRound()
    }
  }

  function restart() {
    round = 1
    score = 0
    finished = false
    startRound()
  }

  api
    .countries()
    .then((list) => (countries = list))
    .catch((e) => (error = errorMessage(e)))
  startRound()
</script>

{#if finished}
  <GameOver mode="clue" {score} max={maxScore} onplayagain={restart} {onexit} />
{:else}
  <section>
    <h1 class="typewriter once">Guess from Clues.</h1>
  </section>

  <section class="section game">
    <div class="status">
      <span>Country {round} / {CLUE_ROUNDS}</span>
      <span>Score : {score}</span>
    </div>

    <ol class="box clues">
      {#each clues as clue (clue.n)}
        <li>
          <span class="label">{clue.label} :</span>
          {#if clue.type === 'flag'}
            <img src={clue.value} alt="Flag clue" />
          {:else}
            <span class="value">{clue.value}</span>
          {/if}
        </li>
      {/each}
      {#if clues.length === 0}
        <li class="muted">Loading…</li>
      {/if}
    </ol>

    {#if wrongGuesses.length}
      <p class="bad wrong"><i class="fa-solid fa-xmark"></i> Not : {wrongGuesses.join(', ')}</p>
    {/if}

    {#if outcome}
      <div class="feedback">
        <span class={outcome.correct ? 'good' : 'bad'}>
          {#if outcome.correct}
            <i class="fa-solid fa-check"></i> Correct, it's {outcome.answer}! +{outcome.points}
          {:else}
            <i class="fa-solid fa-xmark"></i> It was {outcome.answer}.
          {/if}
        </span>
        <button class="btn dark" onclick={next}>
          {round >= CLUE_ROUNDS ? 'See results' : 'Next country'}
        </button>
      </div>
    {:else}
      <article class="field">
        <fieldset>
          <legend>Enter Country Name :</legend>
          <form onsubmit={submitGuess}>
            <input
              bind:this={input}
              bind:value={guess}
              oninput={() => {
                showSuggestions = true
                activeIndex = -1
              }}
              onkeydown={onKeydown}
              onblur={() => setTimeout(() => (showSuggestions = false), 150)}
              autocomplete="off"
              aria-label="Enter country name"
              aria-autocomplete="list"
              aria-controls="clue-suggestions"
              disabled={busy}
            />
          </form>
        </fieldset>
        {#if showSuggestions && suggestions.length}
          <ul class="suggestions" id="clue-suggestions" role="listbox">
            {#each suggestions as name, i (name)}
              <li
                role="option"
                aria-selected={i === activeIndex}
                class:active={i === activeIndex}
                onmousedown={(e) => {
                  e.preventDefault()
                  pick(name)
                }}
              >
                {name}
              </li>
            {/each}
          </ul>
        {/if}
      </article>

      <div class="actions">
        <button class="btn dark" onclick={submitGuess} disabled={busy || !guess.trim()}>
          Guess for {points} pt{points === 1 ? '' : 's'}
        </button>
        <button class="btn" onclick={nextClue} disabled={busy || clues.length >= total}>
          Next clue ({clues.length}/{total})
        </button>
        <button class="btn" onclick={giveUp} disabled={busy}>Give up</button>
      </div>
    {/if}

    {#if error}<p class="error">{error}</p>{/if}

    <button class="btn quit" onclick={onexit}>Quit</button>
  </section>
{/if}

<style>
  .game {
    display: grid;
    gap: 25px;
  }
  .clues {
    list-style: none;
    display: grid;
    gap: 8px;
  }
  .clues li {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 4px 15px;
    font-size: 20px;
    padding-bottom: 8px;
    border-bottom: 1px solid rgba(0, 0, 0, 0.1);
    animation: type 0.4s steps(20, end);
  }
  .clues li:last-child {
    border-bottom: none;
    padding-bottom: 0;
  }
  .label {
    min-width: 150px;
    font-weight: 400;
  }
  .value {
    font-weight: 300;
    letter-spacing: 1px;
  }
  .clues img {
    max-height: 90px;
    max-width: 100%;
    border: 1px solid rgba(0, 0, 0, 0.1);
  }
  .wrong {
    text-align: left;
    font-weight: 300;
  }
  .actions {
    display: flex;
    flex-wrap: wrap;
    gap: 10px;
    justify-content: center;
  }
  .feedback {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 15px;
    font-size: 20px;
  }
  .quit {
    justify-self: start;
  }
  @keyframes type {
    from {
      clip-path: inset(0 100% 0 0);
    }
    to {
      clip-path: inset(0 0 0 0);
    }
  }
</style>
