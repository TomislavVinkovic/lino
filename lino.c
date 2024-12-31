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
    initEditor();
    if(argc >= 2) {
        editorOpen(argv[1]);
    }
    
    
    while (true) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    
    disableRawMode();

    return 0;
}