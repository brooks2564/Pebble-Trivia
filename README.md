# Pebble Trivia

A trivia game for Pebble smartwatches. Questions pulled live from the [Open Trivia Database](https://opentdb.com) — completely free, no account or API key needed.

---

## How to Play

**1. Pick a category**

When you open the app you get a category menu. Choose what you want to be quizzed on:

- **All Topics** — random questions from everything
- **General** — general knowledge
- **Geography** — countries, capitals, landmarks
- **History** — historical events and figures
- **Science** — physics, chemistry, biology, and more
- **Entertainment** — movies, TV, music, books
- **Sports** — athletes, teams, records
- **Animals** — wildlife and nature
- **True / False** — quick true-or-false questions
- **Easy (Kids)** — family-friendly easy questions

**2. Pick a difficulty**

After choosing a category, pick how hard you want it:

- **Any Difficulty** — mix of everything
- **Easy** — beginner friendly
- **Medium** — a solid challenge
- **Hard** — for trivia nerds only

**3. Select your answer**

The question appears on screen with four shuffled answer choices (A, B, C, D) — or just True / False for that category.

Use **UP / DOWN** to highlight an answer, then press **SELECT** to lock it in. The app automatically checks your answer — no self-reporting needed.

On **Pebble Time 2** and **Pebble Round 2**, you can also swipe up/down to move the highlight and tap to confirm.

**4. See your result**

The screen immediately shows whether you were right or wrong:

- Correct — green flash, short buzz, score goes up
- Wrong — red flash, double buzz, correct answer highlighted in green

The result auto-advances after 2 seconds. Press any button (or tap) to skip ahead.

---

## Controls

**Category and difficulty menus:**
- **UP / DOWN** — move the highlight
- **SELECT** — confirm selection
- **Swipe up / down** — move the highlight *(Time 2 and Round 2 only)*
- **Tap** — confirm selection *(Time 2 and Round 2 only)*

**Question screen:**
- **UP / DOWN** — move the answer highlight
- **SELECT** — lock in your answer
- **Swipe up / down** — move the answer highlight *(Time 2 and Round 2 only)*
- **Tap** — lock in your answer *(Time 2 and Round 2 only)*
- **BACK** — return to the difficulty menu

---

## Features

- **Live questions** — fetched fresh from the internet every session, so you never run out
- **Automatic scoring** — the app checks your answer, no self-reporting
- **Score display** — shown in the label bar as `Geography 5/8 x3` (correct/total + streak)
- **Category colors** — each category gets its own color on color-screen Pebbles
- **Difficulty filter** — fine-tune how hard the questions are per category
- **True / False mode** — a quick-fire category with simple yes-or-no questions
- **Marquee scrolling** — long answer choices scroll horizontally after a 1-second pause, then repeat
- **Question auto-scroll** — questions too long to fit scroll up automatically, pause, and repeat
- **Touch support** — swipe and tap to navigate on Pebble Time 2 and Pebble Round 2
- **Round screen support** — layout is adjusted for round Pebble displays
- **All platforms** — runs on every Pebble ever made
- **No account needed** — completely free, no API key, no login

---

## Supported Watches

Runs on all 7 Pebble platforms covering every watch ever released:

- **Pebble** and **Pebble Steel** — original classics
- **Pebble Time** and **Pebble Time Steel** — full color
- **Pebble Time Round** — color, round screen
- **Pebble Time 2** — color, large screen, touchscreen
- **Pebble 2 SE** and **Pebble 2 HR** — heart rate models
- **Pebble 2 Duo** — latest rectangular model
- **Pebble Round 2** — large round color screen, touchscreen

---

## Building

Requires the [Pebble SDK](https://developer.rebble.io/developer.pebble.com/sdk/index.html).

```bash
pebble build
pebble install --phone YOUR_PHONE_IP
```
