<script>
  import { api, errorMessage, FLAG_ROUNDS } from './api.js'
  import GameOver from './GameOver.svelte'

  /** @type {{ onexit: () => void }} */
  let { onexit } = $props()

  let round = $state(1)
  let score = $state(0)
  let streak = $state(0)
  /** @type {import('./api.js').FlagQuestion | null} */
  let question = $state(null)
  /** @type {string | null} option the player clicked */
  let picked = $state(null)
  /** @type {import('./api.js').AnswerResult | null} */
  let result = $state(null)
  let error = $state('')
  let finished = $state(false)

  async function loadQuestion() {
    error = ''
    picked = null
    result = null
    question = null
    try {
      question = await api.flagQuestion()
    } catch (e) {
      error = errorMessage(e)
    }
  }

  /** @param {string} option */
  async function choose(option) {
    if (picked || !question) return
    picked = option
    try {
      result = await api.answer(question.qid, option, true)
      if (result.correct) {
        score += 1
        streak += 1
      } else {
        streak = 0
      }
    } catch (e) {
      error = errorMessage(e)
      picked = null
    }
  }

  function next() {
    if (round >= FLAG_ROUNDS) {
      finished = true
    } else {
      round += 1
      loadQuestion()
    }
  }

  function restart() {
    round = 1
    score = 0
    streak = 0
    finished = false
    loadQuestion()
  }

  /** @param {string} option */
  function optionState(option) {
    if (!result) return ''
    if (option === result.answer) return 'right'
    if (option === picked) return 'wrong'
    return 'dim'
  }

  loadQuestion()
</script>

{#if finished}
  <GameOver mode="flag" {score} max={FLAG_ROUNDS} onplayagain={restart} {onexit} />
{:else}
  <section>
    <h1 class="typewriter once">Guess the Flag.</h1>
  </section>

  <section class="section game">
    <div class="status">
      <span>Flag {round} / {FLAG_ROUNDS}</span>
      <span>Score : {score}{#if streak > 1} <i class="fa-solid fa-fire"></i> {streak}{/if}</span>
    </div>

    {#if error}
      <p class="error">{error}</p>
      <button class="btn" onclick={loadQuestion}>Try again</button>
    {:else}
      <div class="box flag-frame">
        {#if question}
          <img src={question.flag} alt="Flag to identify" />
        {:else}
          <p class="muted">Loading…</p>
        {/if}
      </div>

      {#if question}
        <div class="options">
          {#each question.options as option (option)}
            <button
              class="option {optionState(option)}"
              disabled={picked !== null}
              onclick={() => choose(option)}
            >
              {option}
            </button>
          {/each}
        </div>
      {/if}

      {#if result}
        <div class="feedback">
          <span class={result.correct ? 'good' : 'bad'}>
            {#if result.correct}
              <i class="fa-solid fa-check"></i> Correct!
            {:else}
              <i class="fa-solid fa-xmark"></i> It was {result.answer}.
            {/if}
          </span>
          <button class="btn dark" onclick={next}>
            {round >= FLAG_ROUNDS ? 'See results' : 'Next flag'}
          </button>
        </div>
      {/if}
    {/if}

    <button class="btn quit" onclick={onexit}>Quit</button>
  </section>
{/if}

<style>
  .game {
    display: grid;
    gap: 25px;
  }
  .flag-frame {
    display: grid;
    place-items: center;
    min-height: 260px;
  }
  .flag-frame img {
    max-width: 100%;
    max-height: 220px;
    border: 1px solid rgba(0, 0, 0, 0.1);
  }
  .options {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
    gap: 15px;
  }
  .option {
    font-family: inherit;
    font-size: 22px;
    font-weight: 300;
    letter-spacing: 1px;
    color: black;
    background: white;
    text-align: left;
    padding: 12px 20px;
    border: 1px solid rgba(0, 0, 0, 0.5);
    border-radius: 20px;
    cursor: pointer;
    transition: all 0.1s ease-in;
  }
  .option:hover:not(:disabled) {
    box-shadow: 0 0 20px 5px rgba(0, 0, 0, 0.1);
  }
  .option:disabled {
    cursor: default;
  }
  .option.right {
    border: 2px solid #2e7d32;
    color: #2e7d32;
    font-weight: 500;
  }
  .option.wrong {
    border: 2px solid #c62828;
    color: #c62828;
    text-decoration: line-through;
  }
  .option.dim {
    color: gray;
    border-color: rgba(0, 0, 0, 0.1);
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
</style>
