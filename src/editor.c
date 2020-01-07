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

    initBuffer();
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

void editorInsertRow(int at, const char *s, size_t len) {
    if (at < 0 || at > E.numrows)
        return;
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
    memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    editorUpdateRow(&E.row[at]);
    E.numrows++;
    E.dirty++;
}

void editorUpdateRow(erow *row) {
    int tabs = 0;
    int j;
    int idx;

    for (j = 0; j < row->size; j++)
        if (row->chars[j] == '\t')
            tabs++;
    free(row->render);
    // add space for the line number
    // by now fixed at 1
    // int line_number_size = 2; // number + space
    row->render = malloc(row->size + tabs * (EDITOR_TAB_STOP - 1) + 1);
    // row->render[0] = '1';
    // row->render[1] = ' ';
    idx = 0;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % EDITOR_TAB_STOP != 0)
                row->render[idx++] = ' ';
        } else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
}

void editorRowInsertChar(erow *row, int at, int c) {
    if (at < 0 || at > row->size)
        at = row->size;
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    editorUpdateRow(row);
    E.dirty++;
}

void editorInsertChar(int c) {
    if (E.cy == E.numrows) {
        editorInsertRow(E.numrows, "", 0);
    }
    editorRowInsertChar(&E.row[E.cy], E.cx, c);
    E.cx++;
}