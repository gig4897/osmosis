#!/usr/bin/env python3
"""Generate a beautiful word list HTML page from all vocab CSVs.

Usage:
    python3 tools/generate_wordlist_html.py > /path/to/vcwebsite/osmosis/words.html
"""

import csv
import os
from pathlib import Path

VOCAB_DIR = Path(__file__).parent / "vocab"

# Language display info: (id, display_name, flag_emoji_hex, script_direction)
LANGUAGES = [
    ("arabic", "Arabic", "1f1ea_1f1ec", "rtl"),
    ("chinese", "Chinese", "1f1e8_1f1f3", "ltr"),
    ("dutch", "Dutch", "1f1f3_1f1f1", "ltr"),
    ("french", "French", "1f1eb_1f1f7", "ltr"),
    ("german", "German", "1f1e9_1f1ea", "ltr"),
    ("hindi", "Hindi", "1f1ee_1f1f3", "ltr"),
    ("japanese", "Japanese", "1f1ef_1f1f5", "ltr"),
    ("korean", "Korean", "1f1f0_1f1f7", "ltr"),
    ("mvskoke", "Mvskoke (Creek)", "None", "ltr"),
    ("portuguese_br", "Portuguese (BR)", "1f1e7_1f1f7", "ltr"),
    ("portuguese_pt", "Portuguese (PT)", "1f1f5_1f1f9", "ltr"),
    ("qeqchi", "Q'eqchi'", "1f1ec_1f1f9", "ltr"),
    ("spanish", "Spanish", "1f1ea_1f1f8", "ltr"),
    ("tsalagi", "Tsalagi (Cherokee)", "None", "ltr"),
    ("urdu", "Urdu", "1f1f5_1f1f0", "rtl"),
]

TIERS = ["beginner", "intermediate", "advanced", "expert", "numbers"]
TIER_DISPLAY = {"beginner": "Beginner", "intermediate": "Intermediate",
                "advanced": "Advanced", "expert": "Expert", "numbers": "Numbers"}


def load_language(lang_id):
    """Load all 4 tier CSVs for a language. Returns dict of tier -> list of word dicts."""
    tiers = {}
    for tier in TIERS:
        csv_path = VOCAB_DIR / f"{lang_id}_{tier}.csv"
        if not csv_path.exists():
            continue
        words = []
        with open(csv_path) as f:
            reader = csv.DictReader(f)
            for row in reader:
                words.append({
                    "emoji": row["emoji"],
                    "english": row["english"],
                    "translation": row["translation"],
                    "phonetic": row["phonetic"],
                    "category": row["category"],
                })
        tiers[tier] = words
    return tiers


def emoji_html(codepoint):
    """Convert hex codepoint to HTML entity."""
    return f"&#x{codepoint};"


def generate_html():
    """Generate the full word list HTML page."""
    # Collect available languages
    available = []
    for lang_id, display, flag_hex, direction in LANGUAGES:
        tiers = load_language(lang_id)
        if tiers:
            available.append((lang_id, display, flag_hex, direction, tiers))

    flag_entities = {}
    for _, _, flag_hex, _, _ in available:
        if flag_hex != "None":
            parts = flag_hex.split("_")
            flag_entities[flag_hex] = "".join(f"&#x{p};" for p in parts)

    html = []
    html.append("""<!DOCTYPE html>
<html lang="en" style="scroll-behavior: smooth;">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Osmosis — Complete Word Lists</title>
  <meta name="description" content="Browse all 5,625 vocabulary words available in Osmosis language packs. 15 languages, 375 words each, with emoji, translations, and pronunciation guides.">
  <style>
    *, *::before, *::after { margin: 0; padding: 0; box-sizing: border-box; }

    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background: #0a0a1a; color: #ffffff; line-height: 1.6;
      -webkit-font-smoothing: antialiased;
    }

    .container { max-width: 900px; margin: 0 auto; padding: 0 24px; }

    /* Hero */
    .hero { text-align: center; padding: 60px 24px 40px; }
    .hero h1 {
      font-size: 2.5rem; font-weight: 800; letter-spacing: 0.12em;
      color: #fff;
      text-shadow: 0 0 20px rgba(78,127,255,0.5), 0 0 60px rgba(78,127,255,0.2);
      margin-bottom: 8px;
    }
    .hero p { color: #b0b0b0; font-size: 1.05rem; }
    .hero a { color: #4e7fff; text-decoration: none; }
    .hero a:hover { text-decoration: underline; }

    /* Language nav */
    .lang-nav {
      display: flex; flex-wrap: wrap; gap: 8px; justify-content: center;
      padding: 20px 24px; position: sticky; top: 0; z-index: 10;
      background: rgba(10,10,26,0.95); backdrop-filter: blur(8px);
      border-bottom: 1px solid #2a2a4e;
    }
    .lang-nav a {
      display: inline-block; padding: 6px 14px; border-radius: 20px;
      background: #1a1a2e; border: 1px solid #2a2a4e; color: #b0b0b0;
      text-decoration: none; font-size: 0.85rem; font-weight: 500;
      transition: all 0.2s;
    }
    .lang-nav a:hover { background: #2a2a4e; color: #fff; }

    /* Language section */
    .lang-section { padding: 48px 0 32px; }
    .lang-header {
      font-size: 1.6rem; font-weight: 700; color: #4e7fff;
      margin-bottom: 8px; padding-bottom: 12px; border-bottom: 2px solid #2a2a4e;
    }

    /* Tier */
    .tier { margin: 24px 0; }
    .tier-title {
      font-size: 1rem; font-weight: 600; color: #808080;
      text-transform: uppercase; letter-spacing: 0.08em;
      margin-bottom: 12px; padding-left: 4px;
    }
    .tier-count { font-weight: 400; color: #555; }

    /* Word grid */
    .word-grid {
      display: grid;
      grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
      gap: 8px;
    }
    .word-card {
      background: #12122a; border: 1px solid #1e1e3a; border-radius: 8px;
      padding: 10px 12px; display: flex; align-items: center; gap: 10px;
    }
    .word-emoji { font-size: 1.6rem; flex-shrink: 0; width: 32px; text-align: center; }
    .word-info { min-width: 0; }
    .word-foreign { font-weight: 600; font-size: 0.9rem; color: #fff; }
    .word-foreign[dir="rtl"] { text-align: right; }
    .word-phonetic { font-size: 0.75rem; color: #808080; }
    .word-english { font-size: 0.78rem; color: #b0b0b0; }

    /* Footer */
    footer {
      text-align: center; padding: 40px 24px; border-top: 1px solid #1a1a2e;
      margin-top: 40px;
    }
    footer a { color: #4e7fff; text-decoration: none; }
    footer p { color: #6b6b6b; font-size: 0.85rem; margin: 4px 0; }

    @media (max-width: 480px) {
      .hero h1 { font-size: 1.8rem; }
      .word-grid { grid-template-columns: 1fr; }
      .lang-nav a { font-size: 0.78rem; padding: 5px 10px; }
    }
  </style>
</head>
<body>

  <header class="hero">
    <div class="container">
      <h1>OSMOSIS</h1>
      <p>Complete Word Lists &mdash; <a href="/osmosis/">Back to Osmosis</a></p>
    </div>
  </header>
""")

    # Language navigation
    html.append('  <nav class="lang-nav">\n')
    for lang_id, display, flag_hex, direction, tiers in available:
        flag = flag_entities.get(flag_hex, "")
        word_count = sum(len(words) for words in tiers.values())
        html.append(f'    <a href="#{lang_id}">{flag} {display}</a>\n')
    html.append('  </nav>\n\n')

    # Language sections
    html.append('  <main class="container">\n')
    for lang_id, display, flag_hex, direction, tiers in available:
        flag = flag_entities.get(flag_hex, "")
        word_count = sum(len(words) for words in tiers.values())

        html.append(f'    <section class="lang-section" id="{lang_id}">\n')
        html.append(f'      <h2 class="lang-header">{flag} {display} &mdash; {word_count} words</h2>\n')

        for tier in TIERS:
            if tier not in tiers:
                continue
            words = tiers[tier]
            html.append(f'      <div class="tier">\n')
            html.append(f'        <h3 class="tier-title">{TIER_DISPLAY[tier]} <span class="tier-count">({len(words)} words)</span></h3>\n')
            html.append(f'        <div class="word-grid">\n')

            for w in words:
                emoji = emoji_html(w["emoji"])
                # Escape HTML
                foreign = w["translation"].replace("&", "&amp;").replace("<", "&lt;").replace('"', "&quot;")
                phonetic = w["phonetic"].replace("&", "&amp;").replace("<", "&lt;")
                english = w["english"].replace("&", "&amp;").replace("<", "&lt;")
                dir_attr = f' dir="rtl"' if direction == "rtl" else ""

                html.append(f'          <div class="word-card">\n')
                html.append(f'            <span class="word-emoji">{emoji}</span>\n')
                html.append(f'            <div class="word-info">\n')
                html.append(f'              <div class="word-foreign"{dir_attr}>{foreign}</div>\n')
                html.append(f'              <div class="word-phonetic">{phonetic}</div>\n')
                html.append(f'              <div class="word-english">{english}</div>\n')
                html.append(f'            </div>\n')
                html.append(f'          </div>\n')

            html.append(f'        </div>\n')
            html.append(f'      </div>\n')

        html.append(f'    </section>\n\n')

    html.append('  </main>\n\n')

    # Footer
    html.append("""  <footer>
    <div class="container">
      <p><a href="/osmosis/">Back to Osmosis</a></p>
      <p>Built by VCodeworks LLC &mdash; <a href="https://vcodeworks.dev">vcodeworks.dev</a></p>
      <p>&copy; 2026</p>
    </div>
  </footer>

</body>
</html>
""")

    return "".join(html)


if __name__ == "__main__":
    print(generate_html())
