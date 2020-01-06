// #include <ctype.h>
#include <errno.h>
// #include <fcntl.h>
// #include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
// #include <sys/types.h>
#include <unistd.h>

#include "buffer.h"
#include "editor.h"
#include "utils.h"

// append data to buffer
void abAppend(struct abuf *ab, const char *s, int len) {
    char *new = realloc(ab->b, ab->len + len);
    if (new == NULL) {
        return; // to do handle with error
    }
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

// free the buffer
void abFree(struct abuf *ab) {
    free(ab->b);
}

void initEditor() {
    E.cx = 0;
    E.cy = 0;
    E.rx = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.dirty = 0;
    E.row = NULL;
    E.filename = NULL;
    E.copyBuffer = NULL;
    E.gitBranch = getGitBranch();
    E.statusmsg[0] = '\0';
    E.statusmsg_time = 0;
    if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
        die("getWindowSize");
    }
    // room for the status bar on the last line
    // and the status message on the second to last
    E.screenrows -= 2;
    E.filesize = 0;

    // initBuffer(); // causes errors
}

void die(const char *s) {
    if (write(STDOUT_FILENO, "\x1b[2J", 4) < 0) {
        perror(strerror(errno));
    }
    if (write(STDOUT_FILENO, "\x1b[H", 3) < 0) {
        perror(strerror(errno));
    }
    perror(s);
    exit(1);
}

// fallback used in getwindowsize
int getCursorPosition(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
        return -1;
    }

    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1)
            break;
        if (buf[i] == 'R')
            break;
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') {
        return -1;
    }

    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) {
        return -1;
    }

    return 0;
}

// get the size of the terminal
int getWindowSize(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
            return -1;
        }
        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}