#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#include "terminal.h"

int main(int argc, char** argv) {

    enableRawMode();

    
    while (true) {
        char c = '\0';
        // the second exit condition makes it compatible with cygwin
        if(read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) die("read");
        if(iscntrl(c)) {
            printf("%d\r\n", c);
        }
        else {
            printf("%d ('%c')\r\n", c, c);
        }
        if(c == 'q') {
            break;
        }
    }
    
    disableRawMode();

    return 0;
}