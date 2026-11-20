<script>
  import { api, errorMessage } from './api.js'
  import Leaderboard from './Leaderboard.svelte'
  import Typewriter from './Typewriter.svelte'

  /** @type {{ mode: import('./api.js').Mode, score: number, max: number, onplayagain: () => void, onexit: () => void }} */
  let { mode, score, max, onplayagain, onexit } = $props()

  const NAME_KEY = 'guess-country:player'

  function savedName() {
    try {
      return localStorage.getItem(NAME_KEY) ?? ''
    } catch {
      return ''
    }
  }

  let player = $state(savedName())
  /** @type {number | null} */
  let rank = $state(null)
  let error = $state('')
  let saving = $state(false)
  let boardKey = $state(0)

  const verdict = $derived.by(() => {
    const ratio = max > 0 ? score / max : 0
    if (ratio >= 0.9) return 'Outstanding. You know your world.'
    if (ratio >= 0.7) return 'Great run!'
    if (ratio >= 0.4) return 'Not bad. Go again?'
    return 'The world is big. Try another round.'
  })

  /** @param {Event} event */
  async function save(event) {
    event.preventDefault()
    const name = player.trim()
    if (!name || saving) return
    saving = true
    error = ''
    try {
      const res = await api.submitScore(name, mode, score)
      rank = res.rank
      boardKey += 1
      try {
        localStorage.setItem(NAME_KEY, name)
      } catch {
        // storage unavailable (private mode) - not important
      }
    } catch (e) {
      error = errorMessage(e)
    } finally {
      saving = false
    }
  }
</script>

<section>
  <Typewriter text="Game Over." />
</section>

<section class="section over">
  <div class="result">
    <p class="big">{score}<span class="of">/ {max}</span></p>
    <p class="verdict">{verdict}</p>
  </div>

  {#if rank === null}
    <article class="field">
      <fieldset>
        <legend>Enter Your Name :</legend>
        <form onsubmit={save}>
          <input
            bind:value={player}
            maxlength="20"
            aria-label="Enter your name"
            autocomplete="nickname"
          />
          <button class="submit" type="submit" disabled={saving || !player.trim()}>
            Save <i class="fa-solid fa-arrow-right"></i>
          </button>
        </form>
      </fieldset>
    </article>
  {:else}
    <p class="saved">
      <i class="fa-solid fa-trophy"></i>
      Saved. You're <strong>#{rank}</strong> on the {mode === 'flag' ? 'Flag Quiz' : 'Clue Hunt'} board.
    </p>
  {/if}
  {#if error}<p class="error">{error}</p>{/if}

  <div class="actions">
    <button class="btn dark" onclick={onplayagain}><i class="fa-solid fa-rotate-right"></i> Play again</button>
    <button class="btn" onclick={onexit}>Menu</button>
  </div>
</section>

<section class="section field">
  <fieldset>
    <legend>Top Scores :</legend>
    {#key boardKey}
      <Leaderboard {mode} />
    {/key}
  </fieldset>
</section>

<style>
  .over {
    display: grid;
    gap: 25px;
    justify-items: center;
  }
  .over .field {
    width: 100%;
  }
  .result {
    display: grid;
    gap: 4px;
  }
  .big {
    font-size: 64px;
    font-weight: 300;
    line-height: 1.1;
  }
  .of {
    margin-left: 8px;
    font-size: 24px;
    color: gray;
  }
  .verdict {
    font-size: 16px;
    font-weight: 300;
    color: gray;
    letter-spacing: 0.5px;
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
    display: inline-flex;
    align-items: center;
    gap: 8px;
    font-family: inherit;
    font-size: 14px;
    color: white;
    background: black;
    border: none;
    border-radius: 10px;
    padding: 9px 14px;
    cursor: pointer;
    transition: opacity 0.1s ease-in;
  }
  .submit:disabled {
    opacity: 0.2;
    cursor: default;
  }
  .saved {
    font-size: 16px;
    font-weight: 300;
  }
  .saved i {
    color: #f9a825;
    margin-right: 4px;
  }
  .saved strong {
    font-weight: 600;
  }
  .actions {
    display: flex;
    gap: 10px;
  }
  .actions .btn {
    display: inline-flex;
    align-items: center;
    gap: 8px;
  }
  @media (max-width: 600px) {
    .big {
      font-size: 52px;
    }
  }
</style>
