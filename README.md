# 🎯 Pebble Trivia

A trivia game for Pebble smartwatches. Questions pulled live from the [Open Trivia Database](https://opentdb.com) — completely free, no account or API key needed.

---

## 🕹️ How to Play

**1. Pick a category**

When you open the app you get a category menu. Choose what you want to be quizzed on:

- 🌍 **All Topics** — random questions from everything
- 🧠 **General** — general knowledge
- 🗺️ **Geography** — countries, capitals, landmarks
- 📜 **History** — historical events and figures
- 🔬 **Science** — physics, chemistry, biology, and more
- 🎬 **Entertainment** — movies, TV, music, books
- 🏆 **Sports** — athletes, teams, records
- 🐾 **Animals** — wildlife and nature
- ✅ **True / False** — quick true-or-false questions
- ⭐ **Easy (Kids)** — family-friendly easy questions

**2. Pick a difficulty**

After choosing a category, pick how hard you want it:

- 🎲 **Any Difficulty** — mix of everything
- 🟢 **Easy** — beginner friendly
- 🟡 **Medium** — a solid challenge
- 🔴 **Hard** — for trivia nerds only

**3. Answer the question**

The question appears on screen with four shuffled answer choices (A, B, C, D) — or just True / False for that category. Use the **UP / DOWN** buttons to scroll if the question is long. Press **SELECT** when you think you know the answer and the correct answer is revealed with a little vibration buzz. 📳

**4. Score yourself honestly**

After the answer is revealed, it's on you to be honest:

- 👆 Press **UP** if you got it right — your streak goes up
- 👇 Press **DOWN** if you missed it — streak resets to zero
- ▶️ Press **SELECT** to skip without affecting your streak

Your current streak is always shown in the top bar so you can track how hot your run is. 🔥

---

## 🎮 Controls

**On the question screen:**
- **UP / DOWN** — scroll the question text
- **SELECT** — reveal the answer

**On the answer screen:**
- **UP** — got it right ✅ (streak +1)
- **DOWN** — missed it ❌ (streak resets to zero)
- **SELECT** — skip, no streak change
- **BACK** — return to the difficulty menu

---

## ✨ Features

- 🌐 **Live questions** — fetched fresh from the internet every session, so you never run out
- 🔥 **Streak counter** — shown in the label bar as `Geography x7`, resets on a miss
- 🎨 **Category colors** — each category gets its own color on color-screen Pebbles
- ⚙️ **Difficulty filter** — fine-tune how hard the questions are per category
- ✅ **True / False mode** — a quick-fire category with simple yes-or-no questions
- 📜 **Scrollable text** — long questions and answers scroll smoothly
- ⌚ **Round screen support** — layout is adjusted for round Pebble displays
- 📱 **All platforms** — runs on every Pebble ever made
- 🔓 **No account needed** — completely free, no API key, no login

---

## ⌚ Supported Watches

Runs on all 7 Pebble platforms covering every watch ever released:

- **Pebble** and **Pebble Steel** — original classics
- **Pebble Time** and **Pebble Time Steel** — full color
- **Pebble Time Round** — color, round screen
- **Pebble Time 2** — color, large screen
- **Pebble 2 SE** and **Pebble 2 HR** — heart rate models
- **Pebble 2 Duo** — latest rectangular model
- **Pebble Round 2** — large round color screen

---

## 🔧 Building

Requires the [Pebble SDK](https://developer.rebble.io/developer.pebble.com/sdk/index.html).

```bash
pebble build
pebble install --phone YOUR_PHONE_IP
```
