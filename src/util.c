#include "main.h"

int clamp(int value, int min, int max) {
  return fmax(min, fmin(value, max));
}
// NOTE: vibe coded
void disableRawMode(struct termios* origTermios) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, origTermios);

  /* Restore cursor and original screen */
  printf("\033[?1049l");  // leave alternate screen
  printf("\033[?25h");    // show cursor
  printf("\0338");        // restore cursor pos
  fflush(stdout);
}

// NOTE: vibe coded
void enableRawMode(struct termios* origTermios) {
  tcgetattr(STDIN_FILENO, origTermios);

  struct termios raw = *origTermios;
  raw.c_lflag &= ~(ECHO | ICANON);  // no echo, no line buffering
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  // printf("\0337");       // save cursor pos
  printf("\033[?1049h");  // alternate screen
  printf("\033[2J");      // clear screen
  printf("\033[H");       // cursor home
  fflush(stdout);
}
