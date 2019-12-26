#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

// to mirror ctrl key features
#define CTRL_KEY(k) ((k)&0x1f)

// prototypes
void disableRawMode();
void enableRawMode();
void die(const char *s);
char editorReadKey();
void editorProcessKeypress();
void editorRefreshScreen();
void editorDrawRows();
int getWindowSize(int *rows, int *cols);
void initEditor();
int getCursorPosition(int *rows, int *cols);


struct editorConfig {
    // stores the original termios struct
    // to restore the terminal state 
    int screenrows;
    int screencols;
    struct termios orig_termios;
};

// global variables
struct editorConfig E;

// functions
void initEditor() {
    if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
        die("getWindowSize");
    }
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

// clear the screen
void editorRefreshScreen() {
    write(STDOUT_FILENO, "\x1b[2J", 4); // clear screen
    write(STDOUT_FILENO, "\x1b[H", 3); // reposition the cursor
    editorDrawRows();
    write(STDOUT_FILENO, "\x1b[H", 3);
}

void editorDrawRows() {
    int y;
    for(y = 0; y < E.screenrows; y++) {
        write(STDOUT_FILENO, "~\r\n", 3);
    }
}

// handles ctrl combinations
void editorProcessKeypress() {
    char c = editorReadKey();
    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
    }
}

// read one char from stdin
char editorReadKey() {
    int nread; 
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) {
            die("read");
        }
    }
    return c;

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

int main() {
    enableRawMode();
    initEditor();

    while(1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    
    return 0;
}