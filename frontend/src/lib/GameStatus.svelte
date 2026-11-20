<script>
  /**
   * Shared status bar for both game modes: quit, round counter, score,
   * and a thin progress line in the v1 hairline style.
   * @type {{
   *   label: string,
   *   round: number,
   *   rounds: number,
   *   score: number,
   *   done?: boolean,
   *   streak?: number,
   *   onexit: () => void
   * }}
   */
  let { label, round, rounds, score, done = false, streak = 0, onexit } = $props()

  // Finished rounds; the current round's segment fills once it's answered.
  const completed = $derived(round - 1 + (done ? 1 : 0))
</script>

<div class="status-bar">
  <div class="status">
    <button class="back" onclick={onexit} aria-label="Quit to menu">
      <i class="fa-solid fa-arrow-left"></i><span>Menu</span>
    </button>
    <span class="round">{label} {round} / {rounds}</span>
    <span class="score">
      {#if streak > 1}<span class="streak" title="{streak} in a row"><i class="fa-solid fa-fire"></i> {streak}</span>{/if}
      <span>Score : <strong>{score}</strong></span>
    </span>
  </div>
  <div
    class="progress"
    role="progressbar"
    aria-label="Game progress"
    aria-valuemin="0"
    aria-valuemax={rounds}
    aria-valuenow={completed}
  >
    {#each { length: rounds }, i (i)}
      <span class="seg" class:filled={i < completed} class:current={i === completed}></span>
    {/each}
  </div>
</div>

<style>
  .status-bar {
    display: grid;
    gap: 12px;
  }
  .status {
    display: grid;
    grid-template-columns: 1fr auto 1fr;
    align-items: center;
    font-size: 18px;
    font-weight: 300;
    letter-spacing: 1px;
  }
  .back {
    justify-self: start;
    display: inline-flex;
    align-items: center;
    gap: 8px;
    font: inherit;
    font-size: 16px;
    letter-spacing: 1px;
    color: gray;
    background: none;
    border: none;
    padding: 4px 0;
    cursor: pointer;
    transition: color 0.1s ease-in;
  }
  .back:hover {
    color: black;
  }
  .round {
    justify-self: center;
  }
  .score {
    justify-self: end;
    display: inline-flex;
    align-items: center;
    gap: 14px;
  }
  .score strong {
    font-weight: 500;
  }
  .streak {
    color: #e65100;
    font-weight: 400;
  }
  .progress {
    display: flex;
    gap: 4px;
  }
  .seg {
    flex: 1;
    height: 3px;
    border-radius: 2px;
    background: rgba(0, 0, 0, 0.1);
    transition: background-color 0.3s ease;
  }
  .seg.current {
    background: rgba(0, 0, 0, 0.35);
  }
  .seg.filled {
    background: black;
  }
  @media (max-width: 600px) {
    .status {
      font-size: 15px;
      letter-spacing: 0;
    }
    .back span {
      display: none;
    }
  }
</style>
