#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define EDITOR_VERSION "0.0.1"
#define EDITOR_TAB_STOP 4

// to mirror ctrl key features
#define CTRL_KEY(k) ((k)&0x1f)
// init buffer empty
#define ABUF_INIT {NULL, 0}

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

// buffer that stores what needs to be printed on screen
// to avoid multiple write, we append the data to a buffer
// and then write the buffer
struct abuf {
    char *b;
    int len;
};

// to store row of text
typedef struct erow {
    int size;
    int rsize;
    char *chars;
    char *render; // to render tab
} erow;

// editor info
struct editorConfig {
    int cx, cy; // cursor position
    int rx;
    int rowoff; // for vertical scrolling
    int coloff; // for horizontal scrolling
    int screenrows;
    int screencols;
    int numrows; // number of rows
    char *filename; // current filename
    char statusmsg[80];
    time_t statusmsg_time;
    erow *row; // content of the rows
    struct termios orig_termios;
};

// global variables
struct editorConfig E;

// prototypes
void disableRawMode();
void enableRawMode();
void die(const char *s);
int editorReadKey();
void editorProcessKeypress();
void editorDrawRows(struct abuf *ab);
void editorRefreshScreen();
int getWindowSize(int *rows, int *cols);
void initEditor();
int getCursorPosition(int *rows, int *cols);
void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);
void editorMoveCursor(int key);
void editorOpen();
void editorAppendRow(char *s, size_t len);
void editorScroll();
void editorUpdateRow(erow *row);
int editorRowCxToRx(erow *row, int cx);
void editorDrawStatusBar(struct abuf *ab);
void editorSetStatusMessage(const char *fmt, ...);
void editorDrawMessageBar(struct abuf *ab);

// functions    
void initEditor() {
    E.cx = 0;
    E.cy = 0;
    E.rx = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.row = NULL;
    E.filename = NULL;
    E.statusmsg[0] = '\0';
    E.statusmsg_time = 0;
    if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
        die("getWindowSize");
    }
    // room for the status bar on the last line
    // and the status message on the second to last
    E.screenrows -= 2; 
}

void editorAppendRow(char *s, size_t len) {
    int at;
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
    at = E.numrows;
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';

    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    editorUpdateRow(&E.row[at]);

    E.numrows++;
}

void editorOpen(char *filename) {
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    FILE *fp = fopen(filename, "r");
    if (!fp) { 
        die("fopen");
    }

    free(E.filename);
    E.filename = strdup(filename);

    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' ||
                               line[linelen - 1] == '\r'))
            linelen--;
        editorAppendRow(line, linelen);
    }
    free(line);
    fclose(fp);
}

void editorSetStatusMessage(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}

void editorDrawMessageBar(struct abuf *ab) {
    int msglen;
    abAppend(ab, "\x1b[K", 3);
    msglen = strlen(E.statusmsg);
    if (msglen > E.screencols)
        msglen = E.screencols;
    if (msglen && time(NULL) - E.statusmsg_time < 5)
        abAppend(ab, E.statusmsg, msglen);
}

void editorScroll() {
    E.rx = 0;
    if (E.cy < E.numrows) {
        E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
    }

    if (E.cy < E.rowoff) {
        E.rowoff = E.cy;
    }
    if (E.cy >= E.rowoff + E.screenrows) {
        E.rowoff = E.cy - E.screenrows + 1;
    }
    if (E.rx < E.coloff) {
        E.coloff = E.rx;
    }
    if (E.rx >= E.coloff + E.screencols) {
        E.coloff = E.rx - E.screencols + 1;
    }
}

// draw status bar on the bottom of the screen
// filename max 20 char
void editorDrawStatusBar(struct abuf *ab) {
    int len, rlen;
    char status[80], rstatus[80];
    abAppend(ab, "\x1b[7m", 4);
    len = snprintf(status, sizeof(status), "%.20s - %d lines",
                       E.filename ? E.filename : "[No Name]", E.numrows);
    rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",
                        E.cy + 1, E.numrows);
    if (len > E.screencols)
        len = E.screencols;
    abAppend(ab, status, len);
    while (len < E.screencols) {
        if (E.screencols - len == rlen) {
            abAppend(ab, rstatus, rlen);
            break;
        } else {
            abAppend(ab, " ", 1);
            len++;
        }
    }
    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}

void editorUpdateRow(erow *row) {
    int tabs = 0;
    int j;
    int idx;

    for (j = 0; j < row->size; j++)
        if (row->chars[j] == '\t')
            tabs++;
    free(row->render);
    row->render = malloc(row->size + tabs * (EDITOR_TAB_STOP - 1) + 1);
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

int editorRowCxToRx(erow *row, int cx) {
    int rx = 0;
    int j;
    for (j = 0; j < cx; j++) {
        if (row->chars[j] == '\t')
            rx += (EDITOR_TAB_STOP - 1) - (rx % EDITOR_TAB_STOP);
        rx++;
    }
    return rx;
}

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

// fallback used in getwindowsize
int getCursorPosition(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;
    
    if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
        return -1;
    }

    while(i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    if(buf[0] != '\x1b' || buf[1] != '[') {
        return -1;
    }

    if(sscanf(&buf[2], "%d;%d", rows, cols) != 2) {
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
    } 
    else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
} 

// prints rows
void editorDrawRows(struct abuf *ab) {
    int y;
    int filerow;
    int padding;
    int welcomelen;
    int len;
    char welcome[80];
    for (y = 0; y < E.screenrows; y++) {
        filerow = y + E.rowoff;
        if (filerow >= E.numrows) {
            if (E.numrows == 0 && y == E.screenrows / 3) {
                welcomelen = snprintf(welcome, sizeof(welcome),
                                          "Text editor -- version %s", EDITOR_VERSION);
                if (welcomelen > E.screencols)
                    welcomelen = E.screencols;
                padding = (E.screencols - welcomelen) / 2;
                if (padding) {
                    abAppend(ab, "~", 1);
                    padding--;
                }
                while (padding--)
                    abAppend(ab, " ", 1);
                abAppend(ab, welcome, welcomelen);
            } else {
                abAppend(ab, "~", 1);
            }
        } else {
            len = E.row[filerow].rsize - E.coloff;
            if (len < 0)
                len = 0;
            if (len > E.screencols)
                len = E.screencols;
            abAppend(ab, &E.row[filerow].render[E.coloff], len);
        }

        abAppend(ab, "\x1b[K", 3);
        
        abAppend(ab, "\r\n", 2);
        
    }
}

// clear the screen
void editorRefreshScreen() {
    char buf[32];
    struct abuf ab = ABUF_INIT;

    editorScroll();

    abAppend(&ab, "\x1b[?25l", 6); // hide the cursor
    abAppend(&ab, "\x1b[H", 3);  // reposition the cursor

    editorDrawRows(&ab);
    editorDrawStatusBar(&ab);
    editorDrawMessageBar(&ab);

    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);
    abAppend(&ab, buf, strlen(buf));
    abAppend(&ab, "\x1b[?25h", 6); // show the cursor

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

// editor cursor commands
void editorMoveCursor(int key) {
    int rowlen;
    erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

    switch (key) {
        case ARROW_LEFT:
            if (E.cx != 0) {
                E.cx--;
            } else if (E.cy > 0) {
                E.cy--;
                E.cx = E.row[E.cy].size;
            }
            break;
        case ARROW_RIGHT:
            if (row && E.cx < row->size) {
                E.cx++;
            } 
            else if (row && E.cx == row->size) {
                E.cy++;
                E.cx = 0;
            }
            break;
        case ARROW_UP:
            if (E.cy != 0) {
                E.cy--;
            }
            break;
        case ARROW_DOWN:
            if (E.cy < E.numrows) {
                E.cy++;
            }
            break;
    }
    row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    rowlen = row ? row->size : 0;
    if (E.cx > rowlen) {
        E.cx = rowlen;
    }
}

// handles ctrl combinations and other special keys
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
            if (E.cy < E.numrows)
                E.cx = E.row[E.cy].size;
            break;
        case PAGE_UP:
        case PAGE_DOWN: {
            int times;
            if (c == PAGE_UP) {
                E.cy = E.rowoff;
            } else if (c == PAGE_DOWN) {
                E.cy = E.rowoff + E.screenrows - 1;
                if (E.cy > E.numrows)
                    E.cy = E.numrows;
            }
            times = E.screenrows;
            while (times--)
                editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
        } break;
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
    }
}

// read one char from stdin
int editorReadKey() {
    int nread; 
    char c;
    char seq[3];

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) {
            die("read");
        }
    }

    // handles arrow keys to move the cursor
    if (c == '\x1b') {
        if (read(STDIN_FILENO, &seq[0], 1) != 1)
            return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1)
            return '\x1b';
        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1)
                    return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1':
                            return HOME_KEY;
                        case '3':
                            return DEL_KEY;
                        case '4':
                            return END_KEY;
                        case '5':
                            return PAGE_UP;
                        case '6':
                            return PAGE_DOWN;
                        case '7':
                            return HOME_KEY;
                        case '8':
                            return END_KEY;
                    }
                }
            }
            else {
                switch (seq[1]) {
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
        }
        else if (seq[0] == 'O') {
            switch (seq[1]) {
            case 'H':
                return HOME_KEY;
            case 'F':
                return END_KEY;
            }
        }
    
        return '\x1b';
    } 
    else {
        return c;
    }
}

// prints the string and dies
void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
        die("tcsetattr");
    }
}

void enableRawMode() {
    struct termios raw = E.orig_termios;
    
    if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) {
        die("tcgetattr"); 
    }
    atexit(disableRawMode);

    // ECHO -> do not print char to screen
    // ICANON -> disable canonical mode 
    // ISIG -> disable CTRL C - CTRL Z
    // IXON -> disable CTRl S - CTRL Q
    // IEXTEN -> disable CTRL V
    // ICRNL -> disable CTRL M as 10
    // OPOST -> disable processing features
    // BRKINT, INPCK, ISTRIP and CS8 -> miscellaneus flags
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    // The VMIN value sets the minimum number of bytes
    // of input needed before read() can return
    // 0 -> read() returns as soon as there is any input to be read 
    raw.c_cc[VMIN] = 0;
    // The VTIME value sets the maximum amount of time to wait 
    // before read() returns. It is in tenths of a second, 
    // so we set it to 1/10 of a second, or 100 milliseconds. 
    // If read() times out, it will return 0
    raw.c_cc[VTIME] = 1;

    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        die("tcsetattr");
    }
}

int main(int argc, char *argv[]) {
    enableRawMode();
    initEditor();
    if(argc >= 2) {
        editorOpen(argv[1]);
    }

    editorSetStatusMessage("HELP: Ctrl-Q = quit");

    while(1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    
    return 0;
}