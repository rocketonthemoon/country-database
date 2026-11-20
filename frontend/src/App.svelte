<script>
  import FlagGame from './lib/FlagGame.svelte'
  import ClueGame from './lib/ClueGame.svelte'
  import Leaderboard from './lib/Leaderboard.svelte'
  import Typewriter from './lib/Typewriter.svelte'
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
      <Typewriter text="Guess a Country." loop />
    </section>

    <section class="section modes">
      <button class="mode" onclick={() => (screen = 'flag')}>
        <i class="fa-regular fa-flag icon"></i>
        <h2>Flag Quiz</h2>
        <p>{FLAG_ROUNDS} flags, 4 choices each. One point per correct answer.</p>
        <span class="play">Play <i class="fa-solid fa-arrow-right"></i></span>
      </button>
      <button class="mode" onclick={() => (screen = 'clue')}>
        <i class="fa-solid fa-magnifying-glass icon"></i>
        <h2>Clue Hunt</h2>
        <p>{CLUE_ROUNDS} countries. Clues get easier as you go. Guess early for up to 8 points.</p>
        <span class="play">Play <i class="fa-solid fa-arrow-right"></i></span>
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
    grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
    gap: 20px;
  }
  .mode {
    display: flex;
    flex-direction: column;
    font-family: inherit;
    color: black;
    background: white;
    text-align: left;
    padding: 24px;
    border: 1px solid rgba(0, 0, 0, 0.1);
    border-radius: 20px;
    cursor: pointer;
    transition: box-shadow 0.15s ease-in, border-color 0.15s ease-in;
  }
  .mode:hover,
  .mode:focus-visible {
    box-shadow: 0 0 20px 5px rgba(0, 0, 0, 0.1);
    border-color: rgba(0, 0, 0, 0.3);
    outline: none;
  }
  .icon {
    font-size: 22px;
  }
  .mode h2 {
    font-size: 22px;
    font-weight: 500;
    margin: 10px 0 4px;
  }
  .mode p {
    flex: 1;
    font-size: 14px;
    font-weight: 300;
    color: gray;
  }
  .play {
    margin-top: 16px;
    font-size: 14px;
    font-weight: 500;
    display: inline-flex;
    align-items: center;
    gap: 8px;
  }
  .play i {
    transition: transform 0.15s ease-in;
  }
  .mode:hover .play i,
  .mode:focus-visible .play i {
    transform: translateX(4px);
  }
  .tabs {
    display: flex;
    gap: 8px;
    margin: 8px 0 10px;
  }
  .credits {
    font-size: 13px;
  }
  .credits a {
    color: gray;
  }
</style>
