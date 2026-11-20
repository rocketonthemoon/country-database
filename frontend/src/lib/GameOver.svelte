<script>
  import { api, errorMessage } from './api.js'
  import Leaderboard from './Leaderboard.svelte'

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
  <h1 class="typewriter once">Game Over.</h1>
</section>

<section class="section over">
  <p class="big">{score} <span>/ {max}</span></p>

  {#if rank === null}
    <article class="field">
      <fieldset>
        <legend>Enter Your Name :</legend>
        <form onsubmit={save}>
          <input bind:value={player} maxlength="20" aria-label="Enter your name" autocomplete="nickname" />
        </form>
      </fieldset>
    </article>
    <button class="btn dark" onclick={save} disabled={saving || !player.trim()}>Save score</button>
  {:else}
    <p class="saved">
      <i class="fa-solid fa-trophy"></i> Saved! You're <strong>#{rank}</strong> on the
      {mode === 'flag' ? 'Flag Quiz' : 'Clue Hunt'} board.
    </p>
  {/if}
  {#if error}<p class="error">{error}</p>{/if}

  <div class="actions">
    <button class="btn dark" onclick={onplayagain}>Play again</button>
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
  .big {
    font-size: 80px;
    font-weight: 300;
    line-height: 1;
  }
  .big span {
    font-size: 30px;
    color: gray;
  }
  .saved {
    font-size: 20px;
    font-weight: 300;
  }
  .saved strong {
    font-weight: 600;
  }
  .actions {
    display: flex;
    gap: 10px;
  }
</style>
