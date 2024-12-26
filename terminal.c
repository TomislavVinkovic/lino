#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>

struct termios orig_termios;

void enableRawMode() {
    // get the current state of the terminal
    if(tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        die("tcgetattr");
    }

    // at exit, execute the disableRawMode function
    atexit(disableRawMode);

    // copy the original terminal state into
    // the raw variable for editing
    struct termios raw = orig_termios;
    
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
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
        die("tcsetattr");
    }
}

void die(const char* s) {
    perror(s);
    exit(1);
}