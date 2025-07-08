#include <stdbool.h>
#include <time.h>

#include <signal.h>

#define TGUI_INCLUDE_IMPL
#include "tgui.h"

// In the Windows terminal, the colors seem to be reversed
#define RED    RGB(245, 0,  0)   // RGB(10,  255, 255)
#define GREEN  RGB(0,  245, 0)   // RGB(255, 10,  255)
#define WHITE  RGB(25, 25, 25)   // RGB(230, 230, 230)
#define BG     RGB(0,  64, 64)   // RGB(255, 191, 191)

#define MAX_LIFES     3
#define MAX_SNAKE_LEN 101
#define SCORE_TO_WIN  100

static struct SnakePart {
  unsigned short row, col;
  bool is_head;
} snake[MAX_SNAKE_LEN] = {0};

static unsigned short snake_length = 1;

static enum MoveDir { UP, DOWN, RIGHT, LEFT, IDLE } moving_dir = IDLE, ex_moving_dir = IDLE;

static unsigned short score = 0;
static unsigned short best_score = 0;
static unsigned short lifes = MAX_LIFES;

static unsigned short self_intersection_index = 0;

static bool game_should_quit = false;

static enum Scene { 
  START_MENU,  PAUSE_MENU,
  GAME_SCREEN, HELP_SCREEN,
  WIN_MESSAGE, LOSE_MESSAGE
} scene = START_MENU, ex_scene = START_MENU;

static struct Food { unsigned short row, col; } food = {0};

static void GrowSnake(void) {
  if (snake_length >= MAX_SNAKE_LEN)
    return;

  unsigned short row = 0, col = 0;

  switch (moving_dir) {
    case UP:
      row = snake[snake_length-1].row + 1;
      col = snake[snake_length-1].col;
      break;
    case DOWN:
      row = snake[snake_length-1].row - 1;
      col = snake[snake_length-1].col;
      break;
    case RIGHT:
      row = snake[snake_length-1].row;
      col = snake[snake_length-1].col - 1;
      break;
    case LEFT:
      row = snake[snake_length-1].row;
      col = snake[snake_length-1].col + 1;
      break;
    default:
      break;
  }

  snake[snake_length].row = row;
  snake[snake_length].col = col;
  snake[snake_length].is_head = false;
  snake_length++;

  score++;
  if (best_score < score) best_score = score;
}

static void ChopSnake(void) {
  snake_length -= (snake_length - self_intersection_index);
  score = snake_length - 1;
  --lifes;
}

static void UpdateSnakePosition(void) {
  short horizontal = 0, vertical = 0;
  unsigned short ex_row, ex_col, buff_row, buff_col;

  switch (moving_dir) {
    case UP:
      horizontal = -1;
      break;
    case DOWN:
      horizontal =  1;
      break;
    case RIGHT:
      vertical   =  1;
      break;
    case LEFT:
      vertical   = -1;
      break;
    default:
      break;
  }

  ex_row = snake[0].row;
  ex_col = snake[0].col;

  snake[0].row += horizontal;
  snake[0].col += vertical;

  for (unsigned short i = 1; i < snake_length; i++) {
    buff_row = snake[i].row;
    buff_col = snake[i].col;

    snake[i].row = ex_row;
    snake[i].col = ex_col;

    ex_row = buff_row;
    ex_col = buff_col;
  }
}

static int RandInt(int min, int max) {
  return (rand() % (max - min + 1)) + min;
}

static void SpawnFood(VTerm_t *vt) {
  food.row = (short)RandInt(4, vt->rows - 4);
  food.col = (short)RandInt(4, vt->cols - 4);
}

static bool CheckWallCollision(VTerm_t *vt) {
  return (
    snake[0].row == 2 ||
    snake[0].row == vt->rows - 1 ||
    snake[0].col == 2 ||
    snake[0].col == vt->cols - 2
  );
}

static bool CheckFoodCollision(void) {
  return (
    snake[0].row == food.row &&
    snake[0].col == food.col
  );
}

static bool CheckSelfCollision(void) {
  if (snake_length == 1) return false;

  if ((moving_dir == UP    && ex_moving_dir == DOWN)  ||
      (moving_dir == DOWN  && ex_moving_dir == UP)    ||
      (moving_dir == LEFT  && ex_moving_dir == RIGHT) ||
      (moving_dir == RIGHT && ex_moving_dir == LEFT)  )
  {
    self_intersection_index = 1;
    return true;
  }

  for (unsigned short i = 1; i < snake_length; i++) {
    if (snake[0].row == snake[i].row && snake[0].col == snake[i].col) {
      self_intersection_index = i;
      return true;
    }
  }

  return false;
}

static void ResetGame(VTerm_t *vt, bool reset_best) {
  score = 0;

  if (reset_best == true)
    best_score = 0;

  snake_length = 1;
  lifes = MAX_LIFES;

  moving_dir = IDLE;
  ex_moving_dir = IDLE;

  snake[0].row = vt->rows / 2;
  snake[0].col = vt->cols / 2;

  SpawnFood(vt);
}

static void StartMenuScene(VTerm_t *vt) {
  ex_scene = scene;

  static const char *text[] = {
    "      Play      ",
    "                ",
    "      Help      ",
    "                ",
    "      Quit      "
  };

  unsigned short text_width  = strlen(text[0]) ;
  unsigned short text_height = sizeof(text) / sizeof(text[0]);

  // Row (r1) and col (c1) of the top left corner
  unsigned short r1 = (vt->rows - text_height) / 2;
  unsigned short c1 = (vt->cols - text_width)  / 2;

  // Row (r2) and col (c2) of the bottom right corner
  unsigned short r2 = (vt->rows + text_height) / 2 + 1;
  unsigned short c2 = (vt->cols + text_width)  / 2 + 1;

  short cursor = 0;

  Key_t k = GetKeyPressed();
  while (k != KEY_ENTER) {
      k = GetKeyPressed();
      switch (k) {
        case KEY_W:
        case KEY_ARROW_UP:
          cursor -= 2;
          break;
        case KEY_S: 
        case KEY_ARROW_DOWN:
          cursor += 2;
          break;
        default:
          break;
      }

      if (cursor < 0) cursor = 4;
      else if (cursor > 4) cursor = 0;

      VTermReset(vt, ' ', BG, BG);

      SetRect(vt, WHITE, BG, r1, c1, r2, c2);
      SetMultilineText(vt, text, text_height, WHITE, BG, r1 + 1, c1 + 1);

      SetGlyph(vt, '>', WHITE, BG, r1 + cursor + 1, c1 + 4);
      SetGlyph(vt, '<', WHITE, BG, r1 + cursor + 1, c2 - 4);

      UpdateWindow(vt);
      DelayMs(10);
  }

  switch (cursor) {
    case 0: scene = GAME_SCREEN;
      break;
    case 2: scene = HELP_SCREEN;
      break;
    case 4: game_should_quit = true;
      break;
    default:
      break;
  }
}

static void PauseMenuScene(VTerm_t *vt) {
  ex_scene = scene;

  static const char *text[] = {
    "     Continue     ",
    "                  ",
    "     New Game     ",
    "                  ",
    "       Help       ",
    "                  ",
    "       Quit       "
  };

  unsigned short text_width  = strlen(text[0]);
  unsigned short text_height = sizeof(text) / sizeof(text[0]);

  // Row (r1) and col (c1) of the top left corner
  unsigned short r1 = (vt->rows - text_height) / 2;
  unsigned short c1 = (vt->cols - text_width)  / 2;

  // Row (r2) and col (c2) of the bottom right corner
  unsigned short r2 = (vt->rows + text_height) / 2 + 1;
  unsigned short c2 = (vt->cols + text_width)  / 2 + 1;

  short cursor = 0;

  Key_t k = GetKeyPressed();
  while (k != KEY_ENTER) {
      k = GetKeyPressed();
      switch (k) {
        case KEY_W:
        case KEY_ARROW_UP:
          cursor -= 2;
          break;
        case KEY_S:
        case KEY_ARROW_DOWN: 
          cursor += 2;
          break;
        default:
          break;
      }

      if (cursor < 0) cursor = 6;
      else if (cursor > 6) cursor = 0;

      VTermReset(vt, ' ', BG, BG);

      SetRect(vt, WHITE, BG, r1, c1, r2, c2);
      SetMultilineText(vt, text, text_height, WHITE, BG, r1 + 1, c1 + 1);

      SetGlyph(vt, '>', WHITE, BG, r1 + cursor + 1, c1 + 4);
      SetGlyph(vt, '<', WHITE, BG, r1 + cursor + 1, c2 - 4);
      
      UpdateWindow(vt);
      DelayMs(10);
  }
  
  switch (cursor) {
    case 0: scene = GAME_SCREEN;
      break;
    case 2: scene = GAME_SCREEN;
      ResetGame(vt, true);      
      break;
    case 4: scene = HELP_SCREEN;
      break;
    case 6: game_should_quit = true;
      break;
    default:
      break;
  }
}

static void GameScreenScene(VTerm_t *vt) {
  ex_scene = scene;

  Key_t k = GetKeyPressed();
  if (k != KEY_NONE) {
    switch (k) {
      case KEY_W:
      case KEY_ARROW_UP:
        ex_moving_dir = moving_dir;
        moving_dir = UP;
        break;
      case KEY_S:
      case KEY_ARROW_DOWN:
        ex_moving_dir = moving_dir;
        moving_dir = DOWN;
        break;
      case KEY_D:
      case KEY_ARROW_RIGHT:
        ex_moving_dir = moving_dir;
        moving_dir = RIGHT;
        break;
      case KEY_A:
      case KEY_ARROW_LEFT:
        ex_moving_dir = moving_dir;
        moving_dir = LEFT;
        break;
      case KEY_Q:
      case KEY_ESC:
        scene = PAUSE_MENU;
        break;
      default: 
        break;
    }
  }

  UpdateSnakePosition();

  bool hit_wall = CheckWallCollision(vt);
  if (hit_wall) {
    scene = LOSE_MESSAGE;
  }

  bool hit_food = CheckFoodCollision();
  if (hit_food) {
    GrowSnake();
    SpawnFood(vt);
  }

  bool hit_itself = CheckSelfCollision();
  if (hit_itself) {
    ChopSnake();
  }

  VTermReset(vt, ' ', BG, BG);

  // Borders
  SetRect(vt, WHITE, BG, 2, 2, vt->rows - 1, vt->cols - 2);

  // Score
  char buff[31];
  sprintf(buff, "Score: %d Best score: %d", score, best_score);
  SetText(vt, buff, WHITE, BG, 3, 4);

  // Lifes
  sprintf(buff, "Lifes: ");
  for (unsigned short i = 0; i < lifes; i++) {
    strcat(buff, "@ ");
  }
  SetText(vt, buff, WHITE, BG, 3, vt->cols - 16);

  // Food
  SetGlyph(vt, '*', RED, BG, food.row, food.col);

  // Snake
  for (unsigned short i = 0; i < snake_length; i++) {
    char c = snake[i].is_head ? '@' : '#';
    SetGlyph(vt, c, GREEN, BG, snake[i].row, snake[i].col);
  }

  if (score == SCORE_TO_WIN) scene = WIN_MESSAGE;
  if (lifes == 0) scene = LOSE_MESSAGE;

  UpdateWindow(vt);
  DelayMs(30);
}

static void HelpScreenScene(VTerm_t* vt) {
  static const char *text[] = {
    "               Controls:                  ",
    "                                          ",
    "        UP    -> W | <Arror Up>           ",
    "                                          ",
    "        DOWN  -> S | <Arrow Down>         ",
    "                                          ",
    "        RIGHT -> D | <Arrow Right>        ",
    "                                          ",
    "        LEFT  -> A | <Arrow Left>         ",
    "                                          ",
    "                                          ",
    "                 Rules:                   ",
    "                                          ",
    "     1) Eat food; don't hit walls         ",
    "        or yourself.                      ",
    "                                          ",
    "     2) You have 3 lives. If you hit      ",
    "        yourself, you lose one life.      ",
    "                                          ",
    "     3) If you lose all lives, you lose.  ",
    "                                          ",
    "     4) If you hit a wall, you lose.      ",
    "                                          ",
    "     5) If you reach a score of 100,      ",
    "        you win!                          "
  };
  
  unsigned short text_width  = strlen(text[0]);
  unsigned short text_height = sizeof(text) / sizeof(text[0]);

  // Row (r1) and col (c1) of the top left corner
  unsigned short r1 = (vt->rows - text_height) / 2;
  unsigned short c1 = (vt->cols - text_width)  / 2;

  // Row (r2) and col (c2) of the bottom right corner
  unsigned short r2 = (vt->rows + text_height) / 2 + 1;
  unsigned short c2 = (vt->cols + text_width)  / 2 + 1;

  VTermReset(vt, ' ', BG, BG);

  SetRect(vt, WHITE, BG, r1, c1, r2, c2);
  SetMultilineText(vt, text, text_height, WHITE, BG, r1 + 1, c1 + 1);

  UpdateWindow(vt);

  Key_t k = GetKeyPressed();
  while (k == KEY_NONE) {
    // Halt the program untill any key is pressed
    k = GetKeyPressed();
    DelayMs(10);
  }

  scene = ex_scene;
}

static void WinMessageScene(VTerm_t *vt) {
  static const char *text[] = {
    "                    ",
    "      You won!      ",
    "                    "      
  };
 
  unsigned short text_width  = strlen(text[0]);
  unsigned short text_height = sizeof(text) / sizeof(text[0]);

  // Row (r1) and col (c1) of the top left corner
  unsigned short r1 = (vt->rows - text_height) / 2;
  unsigned short c1 = (vt->cols - text_width)  / 2;

  // Row (r2) and col (c2) of the bottom right corner
  unsigned short r2 = (vt->rows + text_height) / 2 + 1;
  unsigned short c2 = (vt->cols + text_width)  / 2 + 1;

  VTermReset(vt, ' ', BG, BG);

  SetRect(vt, WHITE, BG, r1, c1, r2, c2);
  SetMultilineText(vt, text, text_height, GREEN, BG, r1 + 1, c1 + 1);

  UpdateWindow(vt);
  DelayMs(1000);

  Key_t k = GetKeyPressed();
  while (k == KEY_NONE) {
    // Halt the program untill any key is pressed
    k = GetKeyPressed();
    DelayMs(10);
  }

  ResetGame(vt, true);
  scene = START_MENU;
}

static void LoseMessageScene(VTerm_t *vt) {
  static const char *text[] = {
    "                       ",
    "      You lost :(      ",
    "                       "      
  };
 
  unsigned short text_width  = strlen(text[0]);
  unsigned short text_height = sizeof(text) / sizeof(text[0]);

  // Row (r1) and col (c1) of the top left corner
  unsigned short r1 = (vt->rows - text_height) / 2;
  unsigned short c1 = (vt->cols - text_width)  / 2;

  // Row (r2) and col (c2) of the bottom right corner
  unsigned short r2 = (vt->rows + text_height) / 2 + 1;
  unsigned short c2 = (vt->cols + text_width)  / 2 + 1;

  VTermReset(vt, ' ', BG, BG);

  SetRect(vt, WHITE, BG, r1, c1, r2, c2);
  SetMultilineText(vt, text, text_height, RED, BG, r1 + 1, c1 + 1);

  UpdateWindow(vt);
  DelayMs(1000);

  Key_t k = GetKeyPressed();
  while (k == KEY_NONE) {
    // Halt the program untill any key is pressed
    k = GetKeyPressed();
    DelayMs(10);
  }

  ResetGame(vt, false);
  scene = START_MENU;
}

static void RunGameLoop(VTerm_t *vt) {
  while (!game_should_quit) {
    switch (scene) {
      case START_MENU:
        StartMenuScene(vt);
        break;
      case PAUSE_MENU:
        PauseMenuScene(vt);
        break;
      case GAME_SCREEN:
        GameScreenScene(vt);
        break;
      case HELP_SCREEN:
        HelpScreenScene(vt);
        break;
      case WIN_MESSAGE:
        WinMessageScene(vt);
        break;
      case LOSE_MESSAGE:
        LoseMessageScene(vt);
        break;
      default:
        break;
    }
  }
}

void HandleSigInt(int) {
  ResetWindow();
  exit(EXIT_FAILURE); // VTerm will be cleared on exit
}

void HandleSigSegv(int) {
  ResetWindow();
  exit(EXIT_FAILURE); // VTerm will be cleared on exit
}

void HandleSigAbrt(int) {
  ResetWindow();
  exit(EXIT_FAILURE); // VTerm will be cleared on exit
}

int main(void) {
  // If the game crashes or CTRL-C is pressed, this will ensure that the window resets before exit.
  // The behavior of signal() varies across UNIX versions; it is better to use sigaction() instead.
  signal(SIGINT, HandleSigInt);
  signal(SIGSEGV, HandleSigSegv);
  signal(SIGABRT, HandleSigAbrt);

  srand(time(NULL));

  InitWindow();

  unsigned short wrs, wcs; // Window size (rows x cols)
  GetWindowSize(&wrs, &wcs);

  VTerm_t vt;
  VTermInit(&vt, ++wrs, ++wcs);
  
  // This is the size of the biggest text box. If the windows is smaller
  // then the specified size, the game will crush on opening the Help menu.
  if (wrs < 25 || wcs < 42) {
    const char *text[] = {
      "       The window size is too small.        ",
      "Window must be at least 25x42 (rows X cols)."
    };

    unsigned short text_width = strlen(text[0]);
    unsigned short text_height = sizeof(text) / sizeof(text[0]);

    // Row (r1) and col (c1) of the top left corner
    unsigned short r1 = (vt.rows - text_height) / 2;
    unsigned short c1 = (vt.cols - text_width)  / 2;

    // Row (r2) and col (c2) of the bottom right corner
    unsigned short r2 = (vt.rows + text_height) / 2 + 1;
    unsigned short c2 = (vt.cols + text_width)  / 2 + 1;
    SetRect(&vt, WHITE, BG, r1, c1, r2, c2);
    SetMultilineText(&vt, text, text_height, WHITE, BG, r1 + 1, c1 + 1);
    UpdateWindow(&vt);

    Key_t k = GetKeyPressed();
    while (k == KEY_NONE) {
      // Halt the program untill any key is pressed
      k = GetKeyPressed();
      DelayMs(10);
    }

    // Clean up
    VTermDeinit(&vt);
    ResetWindow();
    exit(EXIT_FAILURE);
  }

  // Init the snake
  snake[0].row = wrs / 2;
  snake[0].col = wcs / 2;
  snake[0].is_head = true;

  SpawnFood(&vt);

  // Start main game loop
  RunGameLoop(&vt);

  // Clean up
  VTermDeinit(&vt);
  ResetWindow();
}
