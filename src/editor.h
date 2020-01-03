#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
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
// number of times the user needs to press ctrl q to close without saving
#define EDITOR_QUIT_TIMES 3

// to mirror ctrl key features
#define CTRL_KEY(k) ((k)&0x1f)
// init buffer empty
#define ABUF_INIT \
    { NULL, 0 }

// debug flag
#define PRINT_KEY 0

enum editorKey {
    BACKSPACE = 127,
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
    char *render;
} erow;

// editor info
struct editorConfig {
    int cx, cy; // cursor position
    int rx;     // for tabs and render
    int rowoff; // for vertical scrolling
    int coloff; // for horizontal scrolling
    int screenrows;
    int screencols;
    int numrows;        // number of rows
    int dirty;          // file saved or not
    int filesize;       // <-------- integer overflow if the file is big
    char *filename;     // current filename
    char *tempfilename; // filename saved
    char statusmsg[80];
    time_t statusmsg_time;
    erow *row; // content of the rows
    char *gitBranch;
    struct termios orig_termios;
};

// global variables
struct editorConfig E;

void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);
