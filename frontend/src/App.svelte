<script>
  import FlagGame from './lib/FlagGame.svelte'
  import ClueGame from './lib/ClueGame.svelte'
  import Leaderboard from './lib/Leaderboard.svelte'
  import { FLAG_ROUNDS, CLUE_ROUNDS } from './lib/api.js'

  // 'menu' | 'flag' | 'clue'
  let screen = $state('menu')
  /** @type {import('./lib/api.js').Mode} */
  let boardMode = $state('flag')
</script>

<!--logo and name-->
<header>
  <nav>
    <button onclick={() => (screen = 'menu')} aria-label="Back to menu">
      <i class="fa-regular fa-flag"></i><span>Country Database</span>
    </button>
  </nav>
</header>

<main>
  {#if screen === 'menu'}
    <section>
      <h1 class="typewriter">Guess a Country.</h1>
    </section>

    <section class="section modes">
      <button class="mode" onclick={() => (screen = 'flag')}>
        <i class="fa-regular fa-flag"></i>
        <h2>Flag Quiz</h2>
        <p>{FLAG_ROUNDS} flags, 4 choices each. One point per correct answer.</p>
      </button>
      <button class="mode" onclick={() => (screen = 'clue')}>
        <i class="fa-solid fa-magnifying-glass"></i>
        <h2>Clue Hunt</h2>
        <p>{CLUE_ROUNDS} countries. Clues get easier as you go. Guess early for up to 8 points.</p>
      </button>
    </section>

    <section class="section field">
      <fieldset>
        <legend>Leaderboard :</legend>
        <div class="tabs" role="tablist">
          <button
            role="tab"
            class="btn"
            class:dark={boardMode === 'flag'}
            aria-selected={boardMode === 'flag'}
            onclick={() => (boardMode = 'flag')}>Flag Quiz</button
          >
          <button
            role="tab"
            class="btn"
            class:dark={boardMode === 'clue'}
            aria-selected={boardMode === 'clue'}
            onclick={() => (boardMode = 'clue')}>Clue Hunt</button
          >
        </div>
        <Leaderboard mode={boardMode} />
      </fieldset>
    </section>

    <p class="muted credits">
      Data: <a href="https://github.com/mledoze/countries" target="_blank" rel="noreferrer">mledoze/countries</a>
      · Flags: <a href="https://flagcdn.com" target="_blank" rel="noreferrer">flagcdn.com</a>
    </p>
  {:else if screen === 'flag'}
    <FlagGame onexit={() => (screen = 'menu')} />
  {:else}
    <ClueGame onexit={() => (screen = 'menu')} />
  {/if}
</main>

<footer>
  <a href="https://github.com/rocketonthemoon" target="_blank" rel="noreferrer">github/rocketonthemoon</a>
</footer>

<style>
  .modes {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
    gap: 30px;
  }
  .mode {
    font-family: inherit;
    color: black;
    background: white;
    text-align: left;
    padding: 30px;
    border: 1px solid rgba(0, 0, 0, 0.1);
    border-radius: 20px;
    cursor: pointer;
    transition: box-shadow 0.15s ease-in;
  }
  .mode:hover,
  .mode:focus-visible {
    box-shadow: 0 0 20px 5px rgba(0, 0, 0, 0.1);
    outline: none;
  }
  .mode i {
    font-size: 30px;
  }
  .mode h2 {
    font-size: 30px;
    font-weight: 500;
    margin: 10px 0 5px;
  }
  .mode p {
    font-weight: 300;
    color: gray;
  }
  .tabs {
    display: flex;
    gap: 10px;
    margin: 10px 0 15px;
  }
  .credits {
    font-size: 14px;
  }
  .credits a {
    color: gray;
  }
</style>
