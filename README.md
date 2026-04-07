# Pebble Trivia

A trivia game for Pebble smartwatches. Questions pulled live from the [Open Trivia Database](https://opentdb.com) — free, no account or API key needed.

---

## How to Play

**1. Pick a category**

When you open the app you get a category menu. Choose what you want to be quizzed on:

| Category | What you get |
|---|---|
| All Topics | Random questions from everything |
| General | General knowledge |
| Geography | Countries, capitals, landmarks |
| History | Historical events and figures |
| Science | Physics, chemistry, biology, and more |
| Entertainment | Movies, TV, music, books |
| Sports | Athletes, teams, records |
| Animals | Wildlife and nature |
| True / False | Quick true-or-false questions |
| Easy (Kids) | Family-friendly easy questions |

**2. Pick a difficulty**

After choosing a category, pick how hard you want it:
- **Any Difficulty** — mix of everything
- **Easy** — beginner friendly
- **Medium** — a solid challenge
- **Hard** — for trivia nerds only

**3. Answer questions**

- The question appears on screen with **four shuffled answer choices** (A, B, C, D) — or just **True / False** for that category
- Use **UP / DOWN** to scroll if the question is long
- Press **SELECT** when you think you know the answer — it reveals the correct answer with a little vibration buzz

**4. Score yourself honestly**

After the answer is revealed:
- Press **UP** if you got it right — your streak goes up
- Press **DOWN** if you missed it — streak resets to zero
- Press **SELECT** to skip without affecting your streak

Your current streak is always shown in the top bar so you can see how hot your run is.

---

## Controls at a Glance

| Button | Question screen | Answer screen |
|---|---|---|
| UP | Scroll up | Got it right (streak +1) |
| DOWN | Scroll down | Missed it (streak resets) |
| SELECT | Reveal answer | Next question (skip) |
| BACK | Return to difficulty menu | — |

---

## Features

- **Live questions** — fetched fresh from the internet every session, so you never run out
- **Streak counter** — shown in the label bar as `Geography x7`, resets on a miss
- **Category colors** — each category has its own color on color-screen Pebbles (Basalt, Chalk, Emery)
- **Difficulty filter** — fine-tune how hard the questions are per category
- **True / False mode** — a quick-fire category with simple yes-or-no questions
- **Scrollable text** — long questions and answers scroll smoothly with UP / DOWN
- **Round screen support** — layout adjusted for Pebble Time Round (Chalk)
- **All platforms** — runs on Aplite, Basalt, Chalk, Diorite, and Emery
- **No account needed** — completely free, no API key, no login

---

## Supported Watches

| Watch | Platform | Color |
|---|---|---|
| Pebble (OG) | Aplite | No |
| Pebble Steel | Aplite | No |
| Pebble Time | Basalt | Yes |
| Pebble Time Steel | Basalt | Yes |
| Pebble Time Round | Chalk | Yes |
| Pebble 2 SE | Diorite | No |
| Pebble 2 HR | Diorite | No |
| Pebble Time 2 | Emery | Yes |

---

## Building

Requires the [Pebble SDK](https://developer.rebble.io/developer.pebble.com/sdk/index.html).

```bash
pebble build
pebble install --phone YOUR_PHONE_IP
```
