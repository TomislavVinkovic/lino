#ifndef TERMINAL_H
#define TERMINAL_H

// TODO: Document header file

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

#define LINO_VERSION "0.0.1"

#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,

    DEL_KEY,
    HOME_KEY,
    END_KEY,

    PAGE_UP,
    PAGE_DOWN
};

struct editorConfig {
    int cx, cy; // cursor positions
    struct termios orig_termios;
    int screenrows;
    int screencolumns;
};

extern struct editorConfig E;

struct abuf {
    char* b;
    int len;
};
#define ABUF_INIT {NULL, 0}

// terminal
void disableRawMode();
void enableRawMode();
void die();
int getWindowSize(int* rows, int* cols);
int getCursorPosition(int* rows, int* cols);

// input
int editorReadKey();
void editorProcessKeypress();
void editorMoveCursor(int key);

// output
void editorDrawRows(struct abuf* ab);
void editorRefreshScreen();

// init
void initEditor();

// append buffer
void abAppend(struct abuf* ab, const char* s, int len);
void abFree(struct abuf* ab);


#endif