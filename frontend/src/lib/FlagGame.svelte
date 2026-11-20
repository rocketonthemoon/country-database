<script>
  import { tick } from 'svelte'
  import { api, errorMessage, FLAG_ROUNDS } from './api.js'
  import GameOver from './GameOver.svelte'
  import Typewriter from './Typewriter.svelte'
  import GameStatus from './GameStatus.svelte'

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
  let imageLoaded = $state(false)
  /** @type {HTMLButtonElement | undefined} */
  let nextButton = $state()

  async function loadQuestion() {
    error = ''
    picked = null
    result = null
    imageLoaded = false
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
      await tick()
      nextButton?.focus()
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
    question = null
    loadQuestion()
  }

  /** @param {string} option */
  function optionState(option) {
    if (!result) return ''
    if (option === result.answer) return 'right'
    if (option === picked) return 'wrong'
    return 'dim'
  }

  /** Number keys 1-4 pick an answer. @param {KeyboardEvent} event */
  function onKeydown(event) {
    if (finished || !question || picked || event.metaKey || event.ctrlKey || event.altKey) return
    const index = Number(event.key) - 1
    if (index >= 0 && index < question.options.length) choose(question.options[index])
  }

  loadQuestion()
</script>

<svelte:window onkeydown={onKeydown} />

{#if finished}
  <GameOver mode="flag" {score} max={FLAG_ROUNDS} onplayagain={restart} {onexit} />
{:else}
  <section>
    <Typewriter text="Guess the Flag." />
  </section>

  <section class="section game">
    <GameStatus label="Flag" {round} rounds={FLAG_ROUNDS} {score} {streak} done={!!result} {onexit} />

    {#if error}
      <div class="box center">
        <p class="error">{error}</p>
        <button class="btn" onclick={loadQuestion}>Try again</button>
      </div>
    {:else}
      <div class="flag-frame" class:loaded={imageLoaded}>
        {#if question}
          {#key question.qid}
            <img
              src={question.flag}
              alt="Flag to identify"
              onload={() => (imageLoaded = true)}
              onerror={() => (imageLoaded = true)}
            />
          {/key}
        {/if}
      </div>

      <div class="options" role="group" aria-label="Answers">
        {#if question}
          {#each question.options as option, i (option)}
            <button
              class="option {optionState(option)}"
              disabled={picked !== null}
              aria-keyshortcuts={String(i + 1)}
              onclick={() => choose(option)}
            >
              <span class="key" aria-hidden="true">{i + 1}</span>
              <span class="name">{option}</span>
              {#if result && option === result.answer}
                <i class="fa-solid fa-check mark" aria-label="correct answer"></i>
              {:else if result && option === picked}
                <i class="fa-solid fa-xmark mark" aria-label="your answer"></i>
              {/if}
            </button>
          {/each}
        {:else}
          {#each [0, 1, 2, 3] as i (i)}
            <div class="option placeholder" aria-hidden="true"></div>
          {/each}
        {/if}
      </div>

      <div class="feedback" aria-live="polite">
        {#if result}
          <span class={result.correct ? 'good' : 'bad'}>
            {result.correct ? 'Correct!' : `It was ${result.answer}.`}
          </span>
          <button class="btn dark" bind:this={nextButton} onclick={next}>
            {round >= FLAG_ROUNDS ? 'See results' : 'Next flag'}
            <i class="fa-solid fa-arrow-right"></i>
          </button>
        {:else}
          <span class="muted hint">Pick an answer, or press 1-4.</span>
        {/if}
      </div>
    {/if}
  </section>
{/if}

<style>
  .game {
    display: grid;
    gap: 25px;
  }
  .center {
    display: grid;
    justify-items: center;
    gap: 15px;
  }
  .flag-frame {
    display: flex;
    align-items: center;
    justify-content: center;
    height: 280px;
    border: 1px solid rgba(0, 0, 0, 0.1);
    padding: 25px;
    background: linear-gradient(90deg, #fafafa, #f0f0f0, #fafafa);
    background-size: 200% 100%;
    animation: shimmer 1.2s infinite linear;
  }
  .flag-frame.loaded {
    background: white;
    animation: none;
  }
  .flag-frame img {
    display: block;
    max-width: 100%;
    max-height: 100%;
    width: auto;
    height: auto;
    min-height: 0;
    object-fit: contain;
    box-shadow: 0 0 20px 5px rgba(0, 0, 0, 0.1);
    opacity: 0;
    transition: opacity 0.25s ease-in;
  }
  .flag-frame.loaded img {
    opacity: 1;
  }
  .options {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 15px;
  }
  .option {
    display: flex;
    align-items: center;
    gap: 15px;
    min-height: 62px;
    font-family: inherit;
    font-size: 20px;
    font-weight: 300;
    letter-spacing: 1px;
    color: black;
    background: white;
    text-align: left;
    padding: 10px 20px;
    border: 1px solid rgba(0, 0, 0, 0.5);
    border-radius: 20px;
    cursor: pointer;
    transition: box-shadow 0.1s ease-in, border-color 0.1s ease-in;
  }
  .option:hover:not(:disabled) {
    box-shadow: 0 0 20px 5px rgba(0, 0, 0, 0.1);
    border-color: black;
  }
  .option:focus-visible {
    outline: 2px solid black;
    outline-offset: 2px;
  }
  .option:disabled {
    cursor: default;
  }
  .option.placeholder {
    border-color: rgba(0, 0, 0, 0.1);
    cursor: default;
  }
  .key {
    flex: none;
    display: grid;
    place-items: center;
    width: 28px;
    height: 28px;
    font-size: 14px;
    font-weight: 400;
    letter-spacing: 0;
    color: gray;
    border: 1px solid rgba(0, 0, 0, 0.15);
    border-radius: 8px;
  }
  .name {
    flex: 1;
  }
  .mark {
    flex: none;
  }
  .option.right {
    border: 2px solid #2e7d32;
    color: #2e7d32;
    font-weight: 500;
  }
  .option.right .key {
    border-color: #2e7d32;
    color: #2e7d32;
  }
  .option.wrong {
    border: 2px solid #c62828;
    color: #c62828;
  }
  .option.wrong .name {
    text-decoration: line-through;
  }
  .option.wrong .key {
    border-color: #c62828;
    color: #c62828;
  }
  .option.dim {
    color: #9e9e9e;
    border-color: rgba(0, 0, 0, 0.1);
  }
  .feedback {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 15px;
    min-height: 46px;
    font-size: 20px;
  }
  .feedback .btn {
    display: inline-flex;
    align-items: center;
    gap: 10px;
  }
  .hint {
    font-size: 16px;
  }
  @keyframes shimmer {
    to {
      background-position: -200% 0;
    }
  }
  @media (max-width: 600px) {
    .game {
      gap: 18px;
    }
    .flag-frame {
      height: 190px;
      padding: 15px;
    }
    .options {
      gap: 10px;
    }
    .option {
      min-height: 56px;
      font-size: 16px;
      letter-spacing: 0;
      padding: 8px 12px;
      gap: 10px;
      border-radius: 16px;
    }
    .key {
      display: none;
    }
    .hint {
      display: none;
    }
    .feedback {
      font-size: 17px;
    }
  }
</style>
