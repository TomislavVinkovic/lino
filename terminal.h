#ifndef TERMINAL_H
#define TERMINAL_H

// TODO: Document header file

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

extern struct termios orig_termios;

void disableRawMode();
void enableRawMode();
void die();

#endif