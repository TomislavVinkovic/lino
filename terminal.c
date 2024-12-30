#include "terminal.h"
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

struct editorConfig E;

void enableRawMode() {
    // get the current state of the terminal
    if(tcgetattr(STDIN_FILENO, &(E.orig_termios)) == -1) {
        die("tcgetattr");
    }

    // at exit, execute the disableRawMode function
    atexit(disableRawMode);

    // copy the original terminal state into
    // the raw variable for editing
    struct termios raw = E.orig_termios;
    
    // disable terminal flags
    // to enable it, I would use the following bitmask:
    // raw.c_lflag |= ECHO etc

    // Terminal flags disabled:
    // ECHO: disable outputting every keyboard input to the terminal
    // ICANON: read the input byte by byte, instead of line by line
    // ISIG: Enable reading Ctrl-C and Ctrl-V into the terminal
    // IXON: Disable software flow control flags (Ctrl-S and Ctrl-Q)
    // IEXTEN: Disable Ctrl-V buffering
    // ICRNL: Disable turning carriage returns into new lines
    // OPOST: Turn off all output processing. This will require the full \r\n to input a new line

    // Some flags not necessary on most modern systems
    // But they are a part of enabling raw mode no the less
    // So we include them in the process:

    // BRKINT: When a break condition occurs, a SIGINT will be sent to the terminal
    // INPCK: Enables parity checking
    // ISTRIP: Strips 8th bit of every input character, replacing it with a zero
    // CS8: A bit mask with multiple bits. Sets the character mask to 8 bits per byte

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

    // The VMIN value sets the minimum number of bytes of input needed before read() can return
    raw.c_cc[VMIN] = 0;
    // The VTIME value sets the maximum amount of time to wait before read() returns
    raw.c_cc[VTIME] = 1;
    
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        die("tcsetattr");
    }
}

void disableRawMode() {
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &(E.orig_termios)) == -1) {
        die("tcsetattr");
    }
}

void die(const char* s) {

    // clear the screen and reposition the cursor
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    // exit
    perror(s);
    exit(1);
}

char editorReadKey() {
    int nread;
    char c;

    if((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        // the second exit condition makes it compatible with cygwin
        if(nread == -1 && errno != EAGAIN) die("read");
    }

    return c;
}

void editorProcessKeypress() {
    char c = editorReadKey();

    switch (c) {

    case CTRL_KEY('q'):
        exit(0);
        break;
    }

}

void editorDrawRows(struct abuf* ab) {
    char buffer[20];
    int i;


    for(i = 0; i < E.screenrows; i++) {
        abAppend(ab, "~", 1);


        // clear line
        abAppend(ab, "\x1b[K", 3);
        if(i < E.screenrows - 1) {
            abAppend(ab, "\r\n", 2);
        }
    }

}

void editorRefreshScreen() {

    struct abuf ab = ABUF_INIT;

    // hide cursor while redrawing
    abAppend(&ab, "\x1b[?25l", 6);

    // clearaj ekran

    // iscrtaj interfejs od vrha prema dnu
    // ova komanda zapravo prima 2 argumenta: vertikalnu i horizontalnu poziciju kursora
    // ali oni su po defaultu 1
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    abAppend(&ab, "\x1b[H", 3);
    // show cursor
    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

// get the window size on systems where ioctl does not work
int getCursorPosition(int* rows, int* cols) {
    char buf[32];

    unsigned int i = 0;
    while(i < sizeof(buf) - 1) {
        if(read(STDIN_FILENO, &buf[i], 1) != 1) return -1;
        if(buf[i] == 'R') break;
        i++;
    }

    // end the string
    buf[i] = '\0';

    if(buf[0] != '\x1b' || buf[1] != '[') return -1;
    if(sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}

int getWindowSize(int* rows, int* cols) {
    struct winsize ws;
    int nwrite;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        // TODO: DEBUG ALTERNATE SIZE FETCH FUNCTION USING GDB
        if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return getCursorPosition(rows, cols);
    }
    else {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
        return 0;
    }
}

void initEditor() {
    if(getWindowSize(&(E.screenrows), &(E.screencolumns)) == -1) {
        die("getWindowSize");
    }
}

void abAppend(struct abuf* ab, const char* s, int len) {
    char* new = (char*) realloc(ab->b, ab->len + len);
    if(new == NULL) return;

    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf* ab) {
    free(ab->b);
}