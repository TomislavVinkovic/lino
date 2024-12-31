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

int editorReadKey() {
    int nread;
    char c;

    if((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        // the second exit condition makes it compatible with cygwin
        if(nread == -1 && errno != EAGAIN) die("read");
    }

    // process arrow key presses (map them to WASD)
    if(c == '\x1b') {
        char seq[3];

        if(read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if(read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if(seq[0] == '[') {

            // page up/page down
            if(seq[1] >= '0' && seq[1] <= '9') {
                if(read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if(seq[2] == '~') {
                    switch(seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5':
                            return PAGE_UP;
                        case '6':
                            return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            }

            // arrow keys
            switch(seq[1]) {
                case 'A':
                    return ARROW_UP;
                case 'B':
                    return ARROW_DOWN;
                case 'C':
                    return ARROW_RIGHT;
                case 'D':
                    return ARROW_LEFT;
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
            }
        }
        else if(seq[0] == 'O') {
            switch(seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }

        return '\x1b';
    }

    else {
        return c;
    }
}

void editorProcessKeypress() {
    int c = editorReadKey();

    switch (c) {

        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case HOME_KEY:
            E.cx = 0;
            break;
        case END_KEY:
            E.cx = E.screencolumns - 1;
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                int times = E.screenrows;
                while(times--) {
                    editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
                }
            }

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
        
        case DEL_KEY:
            // TODO: implement DEL key
            break;

    }

}

void editorDrawRows(struct abuf* ab) {
    int i;

    for(i = 0; i < E.screenrows; i++) {
        int filerow = i + E.rowoff;
        if(filerow >= E.numrows) {
            if(E.numrows == 0 && i == E.screenrows / 3) {
                char welcome[80];
                int welcomelen = snprintf(
                    welcome,
                    sizeof(welcome),
                    "Lino editor -- version %s",
                    LINO_VERSION
                );

                // prevent horizontal screen overflow
                if(welcomelen > E.screencolumns) welcomelen = E.screencolumns;
                int padding = (E.screencolumns - welcomelen) / 2;
                if(padding) {
                    abAppend(ab, "~", 1);
                    padding--;
                }
                while(padding--) abAppend(ab, " ", 1);

                abAppend(ab, welcome, welcomelen);
            }
            else {
                abAppend(ab, "~", 1);
            }
        }
        else {
            int len = E.row[filerow].size;
            if(len > E.screencolumns) len = E.screencolumns;
            abAppend(ab, E.row[filerow].chars, len);
        }

        // clear line
        abAppend(ab, "\x1b[K", 3);
        if(i < E.screenrows - 1) {
            abAppend(ab, "\r\n", 2);
        }
    }

}

void editorMoveCursor(int c) {
    switch(c) {
        case ARROW_LEFT:
            if(E.cx != 0) E.cx--;
            break;
        case ARROW_RIGHT:
            if(E.cx != E.screencolumns - 1) E.cx++;
            break;
        case ARROW_UP:
            if(E.cy != 0) E.cy--;
            break;
        case ARROW_DOWN:
            if(E.cy < E.numrows) E.cy++;
            break;
    }
}

void editorRefreshScreen() {

    editorScroll();

    struct abuf ab = ABUF_INIT;

    // hide cursor while redrawing
    abAppend(&ab, "\x1b[?25l", 6);

    // clearaj ekran

    // iscrtaj interfejs od vrha prema dnu
    // ova komanda zapravo prima 2 argumenta: vertikalnu i horizontalnu poziciju kursora
    // ali oni su po defaultu 1
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    // draw the cursor at the exact position (cx, cy)
    // cursor positions are 1-indexed, not 0-indexed
    char buf[32];
    snprintf(
        buf, 
        sizeof(buf), 
        "\x1b[%d;%dH", 
        (E.cy - E.rowoff) + 1, 
        E.cx + 1
    );
    abAppend(&ab, buf, strlen(buf));

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
    E.cx = 0;
    E.cy = 0;
    E.rowoff = 0;
    E.numrows = 0;
    E.row = NULL;

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

void editorOpen(char* filename) {
    FILE* fp = fopen(filename, "r");
    if(!fp) die("fopen");

    char* line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    // if linelen <> EOF
    while((linelen = getline(&line, &linecap, fp)) != -1) {
        // cut off carriage return and new line
        while(
            linelen > 0 &&
            (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')
        ) {
            linelen--;
        }
        editorAppendRow(line, linelen);
    }
    free(line);
    fclose(fp);
}

void editorAppendRow(char* s, size_t len) {
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));

    int at = E.numrows;
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);

    E.row[at].chars[len] = '\0';
    E.numrows++;
}

void editorScroll() {
    if(E.cy < E.rowoff) {
        E.rowoff = E.cy;
    }
    if(E.cy >= E.rowoff + E.screenrows) {
        E.rowoff = E.cy - E.screenrows + 1;
    }
}