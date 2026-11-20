<script>
  /**
   * Typewriter heading in the v1 style. Types the real characters so the
   * caret always sits right after the last letter. A hidden copy of the
   * full text reserves the space, so the heading never shifts while typing.
   * @type {{ text: string, loop?: boolean }}
   */
  let { text, loop = false } = $props()

  const TYPE_MS = 85
  const ERASE_MS = 35
  const HOLD_MS = 2600
  const GAP_MS = 450

  let count = $state(0)
  let typing = $state(true)

  const reducedMotion =
    typeof window !== 'undefined' &&
    window.matchMedia?.('(prefers-reduced-motion: reduce)').matches

  $effect(() => {
    const full = text.length
    /** @type {ReturnType<typeof setTimeout> | undefined} */
    let timer

    if (reducedMotion) {
      count = full
      typing = false
      return
    }

    count = 0
    typing = true

    const typeNext = () => {
      if (count < full) {
        count += 1
        timer = setTimeout(typeNext, TYPE_MS)
      } else {
        typing = false
        if (loop) timer = setTimeout(eraseNext, HOLD_MS)
      }
    }
    const eraseNext = () => {
      typing = true
      if (count > 0) {
        count -= 1
        timer = setTimeout(eraseNext, ERASE_MS)
      } else {
        timer = setTimeout(typeNext, GAP_MS)
      }
    }

    timer = setTimeout(typeNext, 250)
    return () => clearTimeout(timer)
  })
</script>

<h1 class="typewriter" aria-label={text}>
  <span class="ghost" aria-hidden="true">{text}</span>
  <span class="typed" aria-hidden="true"
    >{text.slice(0, count)}<span class="caret" class:typing class:hidden={reducedMotion}></span></span
  >
</h1>

<style>
  .typewriter {
    display: grid;
    width: fit-content;
    max-width: 100%;
    margin: 40px auto 0;
    padding: 0 8px; /* room for the caret past the last letter */
    font-size: clamp(28px, 6vw, 44px);
    font-weight: 300;
    letter-spacing: 2px;
    line-height: 1.3;
    white-space: nowrap;
  }
  .ghost,
  .typed {
    grid-area: 1 / 1;
    text-align: left;
  }
  .ghost {
    visibility: hidden;
  }
  .caret {
    display: inline-block;
    width: 2px;
    height: 0.95em;
    margin-left: 4px;
    vertical-align: -0.1em;
    background: black;
    animation: blink 1s step-end infinite;
  }
  .caret.typing {
    animation: none;
  }
  .caret.hidden {
    display: none;
  }
  @keyframes blink {
    50% {
      opacity: 0;
    }
  }
  @media (max-width: 600px) {
    .typewriter {
      margin-top: 28px;
      letter-spacing: 1px;
      font-weight: 400;
    }
  }
</style>
