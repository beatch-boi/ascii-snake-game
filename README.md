# ASCII Snake Game

A classic Snake game for a terminal, written in C.  
Enjoy smooth gameplay, colorful ASCII graphics, and a simple UI!

---

## Features

- **Classic Snake Gameplay:** Eat food, grow your snake, and avoid collisions.
- **Lives System:** You start with three lives. You'll lose one if you collide with yourself, and the game ends if you hit a wall or run out of lives.
- **Colorful ASCII Graphics:** Uses RGB colors.
- **High Score Tracking:** See your best score in the session.

---

## Controls

- **Move Up:** `W` or `↑`
- **Move Down:** `S` or `↓`
- **Move Left:** `A` or `←`
- **Move Right:** `D` or `→`
- **Pause/Quit:** `Q` or `Esc`
- **Select Menu:** `Enter`

---

## Requirements

- GCC or Clang (C99 or later)
- Unix-like terminal (Linux, macOS, WSL, or similar)
- Terminal window at least **25 rows x 42 columns**

---

## Build & Run

```sh
make
./build/snake
```