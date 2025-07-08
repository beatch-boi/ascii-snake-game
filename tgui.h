#ifndef TGUI_LIBRARY
#define TGUI_LIBRARY

#define RGB(R, G, B) (Color_t) { R, G, B }

typedef enum Key Key_t;

typedef struct Color Color_t;

typedef struct Glyph Glyph_t;

typedef struct VTerm VTerm_t;

void InitWindow(void);

void ResetWindow(void);

void VTermInit(VTerm_t *vt, unsigned short rows, unsigned short cols);

void VTermDeinit(VTerm_t *vt);

void SetGlyph(VTerm_t *vt, char value, Color_t fg_color, Color_t bg_color, unsigned short row, unsigned short col);

void SetText(VTerm_t *vt, const char *text, Color_t fg_color, Color_t bg_color, unsigned short row, unsigned short col);

void SetMultilineText(VTerm_t *vt, const char **text, unsigned short lines_count, Color_t fg_color, Color_t bg_color, unsigned short row, unsigned short col);

void SetRect(VTerm_t *vt, Color_t fg_color, Color_t bg_color, unsigned short row_1, unsigned short col_1, unsigned short row_2, unsigned short col_2);

void VTermReset(VTerm_t *vt, char value, Color_t fg_color, Color_t bg_color);

void PrintGlyph(const Glyph_t *glyph, unsigned short row, unsigned short col);

void UpdateWindow(const VTerm_t *vt);

Key_t GetKeyPressed(void);

void GetWindowSize(unsigned short *cols, unsigned short *rows);

void DelayMs(unsigned long ms);

#ifdef TGUI_INCLUDE_IMPL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>

typedef enum Key {

  KEY_NONE, KEY_UNKNOWN,

  KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H,
  KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q,
  KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,

  KEY_1, KEY_2, KEY_3, KEY_4, KEY_5,
  KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,

  KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
  KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,

  KEY_ENTER, KET_SPACE, KEY_BACKSPACE, KEY_SHIFT, KEY_ESC,
  KEY_CRTL, KEY_ALT,

  KEY_ARROW_UP, KEY_ARROW_DOWN, KEY_ARROW_LEFT, KEY_ARROW_RIGHT,

} Key_t;  

typedef struct Color {
  unsigned char r, g, b;
} Color_t;

typedef struct Glyph {
  char value;
  Color_t fg_color;
  Color_t bg_color;
} Glyph_t;

typedef struct VTerm {
  unsigned short rows, cols;
  Glyph_t **screen;
} VTerm_t;

void InitWindow(void) {
  // TODO: Error checks (maybe not necessary?)
  struct termios config;
  // Set cannonical inpute mode and disable echoing
  tcgetattr(STDIN_FILENO, &config);
  config.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &config);
  // Make getchar() function non-blocking
  fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
  // Clear screen, hide the cursor and reset it's position
  write(STDOUT_FILENO, "\033[2J\033[?25l\033[0;0H", 16); 
}

void ResetWindow(void) {
  // TODO: Error checks (maybe not necessary?)
  struct termios config;
  // Unset cannonical input mode and enable echoing
  tcgetattr(STDIN_FILENO, &config);
  config.c_lflag |= (ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &config);
  // Reset blocking mode for getchar()
  fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) & ~O_NONBLOCK);
  // Clear screen, show the cursor and reset it's position
  write(STDOUT_FILENO, "\033[0m\033[2J\033[?25h\033[0;0H", 20);
}

void VTermInit(VTerm_t *vt, unsigned short rows, unsigned short cols) {
  vt->rows = rows;
  vt->cols = cols;

  vt->screen = (Glyph_t**) malloc(sizeof(Glyph_t*) * rows);
  if (vt->screen == NULL) {
    ResetWindow();
    fprintf(stderr, "  \033[31mError:\033[0m Couldn't allocate memory for VTerm in \033[33mVTermInit(...)\033[0m\n");
    exit(EXIT_FAILURE);
  }

  for (unsigned short i = 0; i < rows; i++) {
    *(vt->screen + i) = (Glyph_t*) malloc(sizeof(Glyph_t) * cols);
    if (vt->screen == NULL) {
      ResetWindow();
      fprintf(stderr, "  \033[31mError:\033[0m Couldn't allocate memory for VTerm in \033[33mVTermInit(...)\033[0m\n");
      exit(EXIT_FAILURE);
    }

    for (unsigned short j = 0; j < cols; j++) 
      *(*(vt->screen + i) + j) = (Glyph_t) {
        .value = '\0',
        .fg_color = (Color_t) { 0 },
        .bg_color = (Color_t) { 0 }
      };
  }
}

void VTermDeinit(VTerm_t *vt) {
  unsigned short rows = vt->rows;

  for (unsigned short i = 0; i < rows; i++) {
    Glyph_t *current = *(vt->screen + i);
    free(current); current = NULL;
  }

  free(vt->screen); vt->screen = NULL;
}

void SetGlyph(VTerm_t* vt, char value, Color_t fg_color, Color_t bg_color, unsigned short row, unsigned short col) {
  if (row >= vt->rows) {
    ResetWindow();
    fprintf(stderr, "  \033[31mError:\033[0m 'row' is out of bounds in \033[33mSetGlyph(...)\033[0m\n");
    exit(EXIT_FAILURE);
  } else if (col >= vt->cols) {
    ResetWindow();
    fprintf(stderr, "  \033[31mError:\033[0m 'col' is out of bounds in \033[33mSetGlyph(...)\033[0m\n");
    exit(EXIT_FAILURE);
  }

  vt->screen[row][col] = (Glyph_t) { value, fg_color, bg_color };
}

void SetText(VTerm_t *vt, const char *text, Color_t fg_color, Color_t bg_color, unsigned short row, unsigned short col) {
  if (row >= vt->rows) {
    ResetWindow();
    fprintf(stderr, "  \033[31mError:\033[0m 'row' is out of bounds in \033[SetText(...)\033[0m\n");
    exit(EXIT_FAILURE);
  } else if (col >= vt->cols) {
    ResetWindow();
    fprintf(stderr, "  \033[31mError:\033[0m 'col' is out of bounds in \033[SetText(...)\033[0m\n");
    exit(EXIT_FAILURE);
  }

  short text_size = (short) strlen(text);
  for (short i = 0; i < text_size; i++) {
    SetGlyph(vt, text[i], fg_color, bg_color, row, col + i);
  }
}

void SetMultilineText(VTerm_t *vt, const char **text, unsigned short lines_count, Color_t fg_color, Color_t bg_color, unsigned short row, unsigned short col) {
  if (row >= vt->rows) {
    ResetWindow();
    fprintf(stderr, "  \033[31mError:\033[0m 'row' is out of bounds in \033[33mSetMultilineText(...)\033[0m\n");
    exit(EXIT_FAILURE);
  } else if (col >= vt->cols) {
    ResetWindow();
    fprintf(stderr, "  \033[31mError:\033[0m 'col' is out of bounds in \033[33mSetMultilineText(...)\033[0m\n");
    exit(EXIT_FAILURE);
  }

  for (unsigned short i = 0; i < lines_count; i++) {
    SetText(vt, text[i], fg_color, bg_color, row + i, col);
  }
}

void SetRect(VTerm_t *vt, Color_t fg_color, Color_t bg_color, unsigned short row_1, unsigned short col_1, unsigned short row_2, unsigned short col_2) {
  if (row_1 >= vt->rows) {
    ResetWindow();
    fprintf(stderr, "  \033[31mError:\033[0m 'row_1' is out of bounds in \033[33mSetRect(...)\033[0m\n");
    exit(EXIT_FAILURE);
  } else if (col_1 >= vt->cols) {
    ResetWindow();
    fprintf(stderr, "  \033[31mError:\033[0m 'col_1' is out of bounds in \033[33mSetRect(...)\033[0m\n");
    exit(EXIT_FAILURE);
  } else if (row_2 >= vt->rows) {
    ResetWindow();
    fprintf(stderr, "  \033[31mError:\033[0m 'row_2' is out of bounds in \033[33mSetRect(...)\033[0m\n");
    exit(EXIT_FAILURE);
  } else if (col_2 >= vt->cols) {
    ResetWindow();
    fprintf(stderr, "  \033[31mError:\033[0m 'col_2' is out of bounds in \033[33mSetRect(...)\033[0m\n");
    exit(EXIT_FAILURE);
  }

  // Corners
  SetGlyph(vt, '+', fg_color, bg_color, row_1, col_1);
  SetGlyph(vt, '+', fg_color, bg_color, row_2, col_1);
  SetGlyph(vt, '+', fg_color, bg_color, row_1, col_2);
  SetGlyph(vt, '+', fg_color, bg_color, row_2, col_2);
  // Sides
  for (unsigned short i = col_1 + 1; i < col_2; i++) {
    SetGlyph(vt, '-', fg_color, bg_color, row_1, i);
    SetGlyph(vt, '-', fg_color, bg_color, row_2, i);
  }
  for (unsigned short i = row_1 + 1; i < row_2; i++) {
    SetGlyph(vt, '|', fg_color, bg_color, i, col_1);
    SetGlyph(vt, '|', fg_color, bg_color, i, col_2);
  }
}

void VTermReset(VTerm_t *vt, char value, Color_t fg_color, Color_t bg_color) {
  for (unsigned short i = 0; i < vt->rows; i++)
    for (unsigned short j = 0; j < vt->cols; j++)
      SetGlyph(vt, value, fg_color, bg_color, i, j);
}

void PrintGlyph(const Glyph_t *glyph, unsigned short row, unsigned short col) {
  // TODO: Error checks (maybe not necessary?)
  char buff[20];
  // Set foreground color
  sprintf(buff, "\033[38;2;%d;%d;%dm", 
    glyph->fg_color.r, 
    glyph->fg_color.g, 
    glyph->fg_color.b
  );
  write(STDOUT_FILENO, buff, strlen(buff));
  // Set background color
  sprintf(buff, "\033[48;2;%d;%d;%dm", 
    glyph->bg_color.r, 
    glyph->bg_color.g, 
    glyph->bg_color.b
  );
  write(STDOUT_FILENO, buff, strlen(buff));
  // Set the cursor to the corret position
  sprintf(buff, "\033[%d;%dH", row, col);
  write(STDOUT_FILENO, buff, strlen(buff));
  // Print the character
  write(STDOUT_FILENO, &glyph->value, 1);
}

void UpdateWindow(const VTerm_t *vt) {
  for (unsigned short i = 0; i < vt->rows; i++)
    for (unsigned short j = 0; j < vt->cols; j++)
      PrintGlyph(&vt->screen[i][j], i, j);
}

Key_t GetKeyPressed(void) {
  // TODO: Add the rest of the keys
  char c = getchar();

  if (c == -1) return KEY_NONE;

  switch (c) {
    case 'a': return KEY_A;
    case 'b': return KEY_B;
    case 'c': return KEY_C;
    case 'd': return KEY_D;
    case 'e': return KEY_E;
    case 'f': return KEY_F;
    case 'g': return KEY_G;
    case 'h': return KEY_H;
    case 'j': return KEY_J;
    case 'k': return KEY_K;
    case 'l': return KEY_L;
    case 'm': return KEY_M;
    case 'n': return KEY_N;
    case 'o': return KEY_O;
    case 'p': return KEY_P;
    case 'q': return KEY_Q;
    case 'r': return KEY_R;
    case 's': return KEY_S;
    case 't': return KEY_T;
    case 'u': return KEY_U;
    case 'v': return KEY_V;
    case 'w': return KEY_W;
    case 'x': return KEY_X;
    case 'y': return KEY_Y;
    case 'z': return KEY_Z;
    case '\n': return KEY_ENTER;
    default: break;
  }

  if (c == '\033') {
    if (getchar() == '[') {
      c = getchar();
      switch (c) {
        case 'A': return KEY_ARROW_UP;
        case 'B': return KEY_ARROW_DOWN;
        case 'C': return KEY_ARROW_RIGHT;
        case 'D': return KEY_ARROW_LEFT;
      }
    }
    return KEY_ESC;
  }

  return KEY_UNKNOWN;
}

void GetWindowSize(unsigned short *rows, unsigned short *cols) {
  // TODO: Error checks (maybe not necessary?)
  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  *rows = ws.ws_row;
  *cols = ws.ws_col;
}

void DelayMs(unsigned long ms) {
  usleep(ms * 1000);
}

#undef TGUI_INCLUDE_IMPL
#endif // TGUI_INCLUDE_IMPL

#endif // TGUI_LIBRARY
