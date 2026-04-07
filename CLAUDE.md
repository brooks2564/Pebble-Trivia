# Pebble Trivia — Claude Code Instructions

## Project Overview
**Pebble Trivia** is a Pebble smartwatch **watchapp** (not a watchface) that pulls trivia questions from the Open Trivia Database (opentdb.com) — free, no API key required.
UUID: `a1b2c3d4-e5f6-7890-abcd-ef1234567890`
GitHub: `brooks2564/Pebble-Trivia`

## Build & Install
```bash
pebble build
pebble install --phone 192.168.0.182
```

## Project Structure
```
pebble-trivia/
├── package.json            ← Pebble manifest, all 7 platforms, 4 message keys
├── wscript                 ← Build script
├── CLAUDE.md               ← This file
├── resources/
│   └── images/
│       └── menu_icon.png   ← 25x25 launcher icon (pixel art ?)
├── screenshots/
│   ├── banner2.png         ← 720x320 app store banner (retro style)
│   ├── icon-color.png      ← 25x25 color icon
│   ├── icon-bw.png         ← 25x25 B&W icon
│   └── question-mark-icon.png ← 144x144 source icon
└── src/
    ├── c/main.c            ← Watchapp C code (3 windows: category→difficulty→trivia)
    └── pkjs/index.js       ← PebbleKit JS (fetches from OpenTDB, shuffles answers)
```

## Target Platforms (all 7)
aplite, basalt, chalk, diorite, emery, flint, gabbro

## Message Keys
| Key          | ID | Notes                              |
|--------------|----|------------------------------------|
| CATEGORY     | 0  | JS → Watch (category name string)  |
| QUESTION     | 1  | JS → Watch (question + choices)    |
| ANSWER       | 2  | JS → Watch (correct answer string) |
| REQUEST_NEXT | 3  | Watch → JS (encoded int)           |

## REQUEST_NEXT Encoding
- Value `1` = request next question (same category/difficulty)
- Value `cat_id + diff * 200` = category/difficulty change
  - diff: 0=any, 1=easy, 2=medium, 3=hard
  - cat_id 101 = Easy (Kids) — always easy multiple choice
  - cat_id 102 = True/False — boolean type

## Key Architecture Details
- **3-window stack**: category menu → difficulty menu → trivia window
- **Round screen**: `#ifdef PBL_ROUND` → LABEL_H=28, HINT_H=24, HPAD=20
- **Color support**: per-category background colors via `CAT_BG[]` (PBL_COLOR only)
- **Streak counter**: shown in label bar as `"Geography  x5"`, resets on wrong answer
- **Answer states**: STATE_LOADING → STATE_QUESTION → STATE_ANSWER
- **OpenTDB rate limit**: 1 request per 5 seconds; JS enforces 5.5s minimum interval
- **response_code 1**: no results for category+difficulty — JS sets `skipDiff=true` and retries
- **response_code 5**: rate limited — JS retries after 6 seconds
- **dict_write_int32**: used for REQUEST_NEXT (values can reach 702 = 102 + 3*200)

## OpenTDB Categories
| cat_id | Name        |
|--------|-------------|
| 0      | All Topics  |
| 9      | General     |
| 22     | Geography   |
| 23     | History     |
| 17     | Science     |
| 11     | Entertainment |
| 21     | Sports      |
| 27     | Animals     |
| 101    | Easy (Kids) |
| 102    | True/False  |

## Git
```bash
git add -p
git commit -m "message"
git push   # token already embedded in remote URL
```
