<script>
  import { api, errorMessage } from './api.js'

  /** @type {{ mode: import('./api.js').Mode }} */
  let { mode } = $props()

  /** @type {import('./api.js').ScoreRow[] | null} */
  let rows = $state(null)
  let error = $state('')

  $effect(() => {
    const current = mode
    rows = null
    error = ''
    api
      .scores(current)
      .then((r) => {
        if (current === mode) rows = r
      })
      .catch((e) => (error = errorMessage(e)))
  })
</script>

{#if error}
  <p class="error">{error}</p>
{:else if rows === null}
  <p class="muted">Loading…</p>
{:else if rows.length === 0}
  <p class="muted">No scores yet. Be the first!</p>
{:else}
  <ol class="board">
    {#each rows as row, i (i)}
      <li>
        <span class="rank">{i + 1}.</span>
        <span class="player">{row.player}</span>
        <span class="score">{row.score}</span>
      </li>
    {/each}
  </ol>
{/if}

<style>
  p {
    text-align: left;
    margin: 0 10px;
  }
  .board {
    list-style: none;
    margin: 0 10px;
  }
  .board li {
    display: grid;
    grid-template-columns: 32px 1fr auto;
    gap: 10px;
    text-align: left;
    font-size: 16px;
    letter-spacing: 1px;
    padding: 6px 0;
    border-bottom: 1px solid rgba(0, 0, 0, 0.1);
  }
  .board li:last-child {
    border-bottom: none;
  }
  .board li:nth-child(1) .rank {
    color: #f9a825;
    font-weight: 600;
  }
  .board li:nth-child(2) .rank {
    color: #9e9e9e;
    font-weight: 600;
  }
  .board li:nth-child(3) .rank {
    color: #a1662f;
    font-weight: 600;
  }
  .rank {
    color: gray;
    font-weight: 300;
  }
  .player {
    font-weight: 300;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  .score {
    font-weight: 500;
  }
</style>
