import './styles.css';

const app = document.querySelector<HTMLDivElement>('#app');

if (!app) {
    throw new Error('Missing #app root');
}

const header = `
  <header class="site-header" aria-label="Primary">
    <a class="brand-mark" href="/" aria-label="Pilgrim of the Thorn home">
      <span class="brand-cross" aria-hidden="true">✠</span>
      <span>Pilgrim of the Thorn</span>
    </a>
    <nav class="nav-links" aria-label="Sections">
      <a href="/#vow">Vow</a>
      <a href="/#world">World</a>
      <a href="/#combat">Combat</a>
      <a href="/#download">Demo</a>
      <span class="nav-coming-soon"><s>Purchase</s> · Coming soon</span>
    </nav>
  </header>
`;

function renderHomePage(): void {
    app!.innerHTML = `
    ${header}
    <main id="top">
      <section class="hero" id="download" aria-labelledby="hero-title">
        <div class="hero-vignette"></div>
        <div class="hero-content">
          <p class="kicker">A Catholic gothic action platformer</p>
          <h1 id="hero-title">Pilgrim of the Thorn</h1>
          <p class="hero-copy">
            Walk through ruined cloisters, moonlit belfries, and candle-bright chapels as a pilgrim-knight
            carrying prayer, relic, and blade into a city that has forgotten grace.
          </p>
          <div class="hero-actions" aria-label="Game actions">
            <a class="download-button" href="/api/download">Download free demo</a>
            <span class="secondary-button coming-soon-button" aria-disabled="true">
              <s>Purchase</s>
              <span>Coming soon</span>
            </span>
          </div>
          <p class="download-note">
            Free macOS demo · DMG version ${__PILGRIM_VERSION__}<br />
            First launch: if macOS blocks the demo, open System Settings → Privacy &amp; Security
            and click Open Anyway.
          </p>
        </div>
      </section>

      <section class="intro-band" id="vow" aria-labelledby="vow-title">
        <div class="section-heading">
          <p class="kicker">Sanctity under siege</p>
          <h2 id="vow-title">Not dark fantasy with incense. A world shaped by the Church.</h2>
        </div>
        <div class="triptych">
          <article>
            <span aria-hidden="true">I</span>
            <h3>Relics Have Weight</h3>
            <p>Holy objects are not trinkets. They are memory, obligation, and the stubborn presence of saints in a city of ash.</p>
          </article>
          <article>
            <span aria-hidden="true">II</span>
            <h3>Prayer Is Defiance</h3>
            <p>Candlelight, chant, and confession stand against decay as surely as sword steel stands against monsters.</p>
          </article>
          <article>
            <span aria-hidden="true">III</span>
            <h3>Beauty Still Wounds</h3>
            <p>Stained glass and carved stone are not scenery. They are signs that even ruin can be ordered toward glory.</p>
          </article>
        </div>
      </section>

      <section class="image-section" id="world" aria-labelledby="world-title">
        <div class="image-copy">
          <p class="kicker">Cloisters, belfries, thorned gardens</p>
          <h2 id="world-title">A gothic pilgrimage through sacred architecture.</h2>
          <p>
            Explore layered 2D spaces built from cathedral silhouettes, reliquary alcoves, moonlit towers,
            ivy-wrapped arches, and pools of votive gold. The page art comes straight from the game's concept
            direction, where Castlevania mood meets Catholic material culture.
          </p>
        </div>
        <figure class="showcase-frame">
          <img src="/art/cathedral-background.png" alt="Pixel art cathedral ruins under a moonlit sky" />
        </figure>
      </section>

      <section class="combat-band" id="combat" aria-labelledby="combat-title">
        <div class="combat-media">
          <img src="/art/pilgrim-walk-keyed.png" alt="Pixel art animation sheet of the pilgrim walking" />
        </div>
        <div class="combat-copy">
          <p class="kicker">Measured steel</p>
          <h2 id="combat-title">Classic action-platforming with a pilgrim's restraint.</h2>
          <p>
            Move through deliberate jumps, short backdashes, and committed sword strikes tuned toward old-school
            gothic platformers. The hero is not a swaggering demon hunter. He is a penitent with a lantern.
          </p>
          <dl class="feature-list">
            <div>
              <dt>Movement</dt>
              <dd>Side-scrolling rooms, grounded jumps, backdash spacing, and candlelit platform reads.</dd>
            </div>
            <div>
              <dt>Combat</dt>
              <dd>Readable sword timing and attack commitment built for careful, tactile encounters.</dd>
            </div>
            <div>
              <dt>Mood</dt>
              <dd>Chapels, banners, Marian glass, reliquaries, and ruins glowing against the night.</dd>
            </div>
          </dl>
        </div>
      </section>

      <section class="final-call" aria-labelledby="final-title">
        <div>
          <p class="kicker">The bell is still ringing</p>
          <h2 id="final-title">Take up the lantern.</h2>
          <p>Begin the pilgrimage with the free Mac demo. The full game is coming soon.</p>
        </div>
        <div class="final-actions">
          <a class="download-button" href="/api/download">Download free demo</a>
          <span class="secondary-button coming-soon-button" aria-disabled="true">
            <s>Purchase</s>
            <span>Coming soon</span>
          </span>
        </div>
      </section>
    </main>
  `;
}

renderHomePage();
