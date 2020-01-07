#ifndef EDITOR_H
#define EDITOR_H

#include <termios.h>
#include <time.h>

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

// represents a single line of the file
typedef struct erow {
    int size; // size of the row, excluding the null term
    int rsize; // size of the rendered row
    char *chars; // row content
    char *render; // row content "rendered" for screen (for TABs)
} erow;

// editor info
struct editorConfig {
    int cx, cy; // cursor position in characters
    int rx;     // for tabs and render
    int rowoff; // offset of row displayed for vertical scrolling
    int coloff; // offset of column displayed for horizontal scrolling
    int screenrows; // number of rows that we can show
    int screencols; // number of columns that we can show
    int numrows;        // number of rows
    int dirty;          // file saved or not
    int filesize;       // <-------- integer overflow if the file is big
    char *filename;     // current filename
    char *tempfilename; // filename saved
    char *copyBuffer; // buffer that store ctrl c
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
void initEditor();
void die(const char *s);
int getCursorPosition(int *rows, int *cols);
int getWindowSize(int *rows, int *cols);
void editorInsertChar(int c);
void editorRowInsertChar(erow *row, int at, int c);
void editorUpdateRow(erow *row);
void editorInsertRow(int at, const char *s, size_t len);
void editorRowDelChar(erow *row, int at);
void editorDelChar();
void editorRowAppendString(erow *row, char *s, size_t len);
void editorDelRow(int at);
void editorFreeRow(erow *row);
void undoOperation();

#endif