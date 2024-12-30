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

struct editorConfig {
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
char editorReadKey();
void editorProcessKeypress();

// output
void editorDrawRows(struct abuf* ab);
void editorRefreshScreen();

// init
void initEditor();

// append buffer
void abAppend(struct abuf* ab, const char* s, int len);
void abFree(struct abuf* ab);


#endif