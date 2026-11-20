<script>
  import { api, errorMessage, CLUE_ROUNDS } from './api.js'
  import GameOver from './GameOver.svelte'
  import Typewriter from './Typewriter.svelte'
  import GameStatus from './GameStatus.svelte'

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

  /** @type {HTMLButtonElement | undefined} */
  let nextButton = $state()

  // After a round ends, focus "Next" so Enter continues.
  $effect(() => {
    if (outcome && nextButton) nextButton.focus()
  })

  const points = $derived(total - clues.length + 1)
  const locked = $derived(Math.max(0, total - clues.length))
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
    <Typewriter text="Guess from Clues." />
  </section>

  <section class="section game">
    <GameStatus label="Country" {round} rounds={CLUE_ROUNDS} {score} done={!!outcome} {onexit} />

    <ol class="box clues" aria-live="polite">
      {#each clues as clue (clue.n)}
        <li class="clue">
          <span class="num">{clue.n + 1}</span>
          <span class="label">{clue.label}</span>
          {#if clue.type === 'flag'}
            <img src={clue.value} alt="Flag clue" />
          {:else}
            <span class="value">{clue.value}</span>
          {/if}
        </li>
      {/each}
      {#if !outcome && locked > 0 && clues.length > 0}
        <li class="clue locked">
          <span class="num"><i class="fa-solid fa-lock"></i></span>
          <span class="label">{locked} more clue{locked === 1 ? '' : 's'}</span>
          <span class="pips" aria-hidden="true">
            {#each { length: total }, i (i)}
              <span class="pip" class:on={i < clues.length}></span>
            {/each}
          </span>
        </li>
      {/if}
    </ol>

    {#if wrongGuesses.length && !outcome}
      <p class="wrong"><i class="fa-solid fa-xmark"></i> Not {wrongGuesses.join(', ')}</p>
    {/if}

    {#if outcome}
      <div class="feedback">
        <span class={outcome.correct ? 'good' : 'bad'}>
          {#if outcome.correct}
            <i class="fa-solid fa-check"></i> {outcome.answer}! +{outcome.points}
          {:else}
            It was {outcome.answer}.
          {/if}
        </span>
        <button class="btn dark" bind:this={nextButton} onclick={next}>
          {round >= CLUE_ROUNDS ? 'See results' : 'Next country'}
          <i class="fa-solid fa-arrow-right"></i>
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
            <button
              class="submit"
              type="submit"
              aria-label="Submit guess"
              disabled={busy || !guess.trim()}
            >
              <i class="fa-solid fa-arrow-right"></i>
            </button>
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
        <span class="worth">Worth <strong>{points}</strong> pt{points === 1 ? '' : 's'}</span>
        <div class="buttons">
          <button class="btn" onclick={nextClue} disabled={busy || clues.length >= total}>
            <i class="fa-regular fa-lightbulb"></i> Next clue
          </button>
          <button class="btn" onclick={giveUp} disabled={busy}>Give up</button>
        </div>
      </div>
    {/if}

    {#if error}<p class="error">{error}</p>{/if}
  </section>
{/if}

<style>
  .game {
    display: grid;
    gap: 22px;
  }
  .clues {
    list-style: none;
    display: grid;
    padding: 6px 20px;
  }
  .clue {
    display: grid;
    grid-template-columns: 24px 120px 1fr;
    align-items: center;
    gap: 12px;
    min-height: 44px;
    padding: 8px 0;
    font-size: 16px;
    border-bottom: 1px solid rgba(0, 0, 0, 0.1);
    animation: type 0.4s steps(20, end);
  }
  .clue:last-child {
    border-bottom: none;
  }
  .num {
    font-size: 13px;
    color: gray;
    font-weight: 300;
  }
  .label {
    font-weight: 400;
  }
  .value {
    font-weight: 300;
    letter-spacing: 0.5px;
  }
  .clue img {
    max-height: 70px;
    max-width: 100%;
    box-shadow: 0 0 20px 5px rgba(0, 0, 0, 0.1);
  }
  .locked {
    color: #bdbdbd;
    animation: none;
  }
  .locked .label {
    font-weight: 300;
  }
  .locked .num {
    font-size: 12px;
    color: #bdbdbd;
  }
  .pips {
    display: flex;
    gap: 5px;
  }
  .pip {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: rgba(0, 0, 0, 0.12);
  }
  .pip.on {
    background: black;
  }
  .wrong {
    text-align: left;
    font-size: 15px;
    font-weight: 300;
    color: #c62828;
  }
  .field form {
    display: flex;
    align-items: center;
  }
  .field input {
    flex: 1;
    width: auto;
    min-width: 0;
  }
  .submit {
    flex: none;
    display: grid;
    place-items: center;
    width: 38px;
    height: 38px;
    font-size: 15px;
    color: white;
    background: black;
    border: none;
    border-radius: 10px;
    cursor: pointer;
    transition: opacity 0.1s ease-in;
  }
  .submit:disabled {
    opacity: 0.2;
    cursor: default;
  }
  .actions {
    display: flex;
    flex-wrap: wrap;
    justify-content: space-between;
    align-items: center;
    gap: 10px;
  }
  .worth {
    font-size: 15px;
    font-weight: 300;
    color: gray;
  }
  .worth strong {
    font-weight: 600;
    color: black;
  }
  .buttons {
    display: flex;
    gap: 10px;
  }
  .feedback {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 15px;
    font-size: 17px;
  }
  .feedback .btn {
    display: inline-flex;
    align-items: center;
    gap: 10px;
  }
  @keyframes type {
    from {
      clip-path: inset(0 100% 0 0);
    }
    to {
      clip-path: inset(0 0 0 0);
    }
  }
  @media (max-width: 600px) {
    .game {
      gap: 16px;
    }
    .clues {
      padding: 4px 14px;
    }
    .clue {
      grid-template-columns: 18px 1fr;
      gap: 2px 10px;
      font-size: 15px;
      min-height: 0;
    }
    .clue .value,
    .clue img {
      grid-column: 2;
    }
    .pips {
      grid-column: 2;
    }
    .feedback {
      font-size: 15px;
    }
  }
</style>
