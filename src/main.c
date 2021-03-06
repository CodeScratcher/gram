#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
// #include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <sys/ioctl.h>
#include <sys/types.h>
// #include <termios.h>
// #include <time.h>
#include <unistd.h>

#include "editor.h"
#include "bars.h"
#include "buffer.h"
#include "utils.h"

const char *default_status_msg = "HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find |";

// prototypes    
// TODO REMOVE UNUSED PROTOTYPES
void disableRawMode();
void enableRawMode();
// void die(const char *s);
int editorReadKey();
void editorProcessKeypress();
void editorDrawRows(struct abuf *ab);
void editorRefreshScreen();
// int getWindowSize(int *rows, int *cols);
// void initEditor();
int getCursorPosition(int *rows, int *cols);
// void abAppend(struct abuf *ab, const char *s, int len);
// void abFree(struct abuf *ab);
void editorMoveCursor(int key, int mode);
void editorOpen();
void editorInsertRow(int at, const char *s, size_t len) ;
void editorScroll();
void editorUpdateRow(erow *row);
int editorRowCxToRx(erow *row, int cx);
// void editorDrawStatusBar(struct abuf *ab);
// void editorSetStatusMessage(const char *fmt, ...);
// void editorDrawMessageBar(struct abuf *ab);
void editorRowInsertChar(erow *row, int at, int c);
void editorInsertChar(int c);
char *editorRowsToString(int *buflen);
void editorSave();
void editorRowDelChar(erow *row, int at);
void editorDelChar();
void editorFreeRow(erow *row);
void editorDelRow(int at);
void editorRowAppendString(erow *row, char *s, size_t len);
void editorInsertNewline();
void editorFind();
int editorRowRxToCx(erow *row, int rx);
char *editorPrompt(const char *prompt, void (*callback)(char *, int));
void editorFindCallback(char *query, int key);
void resetFileSize(FILE *fp);
void editorIndentLine();
void editorUnindentLine();
void editorCommentLine();
char *extractWordFromLine(char *line, int len, int pos);
void editorShowShortcutsList();

// debug utilities
void dumpReceivedReadKey();

void dumpReceivedReadKey(int key) {
    char str[1000];
    char dest[1000] = "Received: ";
    sprintf(str, "%d", key);
    strcat(dest,str);
    editorSetStatusMessage(dest);
}

// char *extractWordFromLine(char *line, int len, int pos) {
//     int lower, upper;
//     int i = pos;
//     char *word = NULL;
//     if (pos > len) {
//         return NULL;
//     }
//     else if (line[pos] == ' ' || line[pos] == '\n' || line[pos] == '\t') {
//         word = (char *)malloc(sizeof(char) * 2);
//         word[0] = ' ';
//         word[1] = '\0';
//     }
//     else {
//         // move right and left to find a space
//         while (i >= 0 && line[i] != ' ' && line[i] != '\t' && line[i] != '\n') {
//             i--;
//         }
//         lower = i + 1;
//         i = pos;
//         while (i < len && line[i] != ' ' && line[i] != '\t' && line[i] != '\n') {
//             i++;
//         }
//         upper = i;
//         // quick fix if the cursor position is on space
//         if (lower == upper + 1) {
//             lower = upper;
//         }
//         // extract the word
//         word = (char *)malloc(sizeof(char) * (upper - lower + 1));
//         strncpy(word, line + lower, upper - lower);
//         word[upper - lower] = '\0';
//     }
//     return word;
// }

void editorFindCallback(char *query, int key) {
    static int last_match = -1;
    static int direction = 1;
    if (key == '\r' || key == '\x1b') {
        last_match = -1;
        direction = 1;
        return;
    } 
    else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
        direction = 1;
    } 
    else if (key == ARROW_LEFT || key == ARROW_UP) {
        direction = -1;
    } 
    else {
        last_match = -1;
        direction = 1;
    }
    if (last_match == -1)
        direction = 1;
    int current = last_match;
    int i;
    for (i = 0; i < E.numrows; i++) {
        current += direction;
        if (current == -1)
            current = E.numrows - 1;
        else if (current == E.numrows)
            current = 0;
        erow *row = &E.row[current];
        char *match = strstr(row->render, query);
        if (match) {
            last_match = current;
            E.cy = current;
            E.cx = editorRowRxToCx(row, match - row->render);
            E.rowoff = E.numrows;
            break;
        }
    }
}

void editorFind() {
    int saved_cx = E.cx;
    int saved_cy = E.cy;
    int saved_coloff = E.coloff;
    int saved_rowoff = E.rowoff;
    char *query = editorPrompt("Search: %s (Use ESC/Arrows/Enter)",editorFindCallback);
    if (query) {
        free(query);
    } else {
        E.cx = saved_cx;
        E.cy = saved_cy;
        E.coloff = saved_coloff;
        E.rowoff = saved_rowoff;
    }
}

int editorRowRxToCx(erow *row, int rx) {
    int cur_rx = 0;
    int cx;
    for (cx = 0; cx < row->size; cx++) {
        if (row->chars[cx] == '\t')
            cur_rx += (EDITOR_TAB_STOP - 1) - (cur_rx % EDITOR_TAB_STOP);
        cur_rx++;
        if (cur_rx > rx)
            return cx;
    }
    return cx;
}

// void editorInsertRow(int at, const char *s, size_t len) {
//     if (at < 0 || at > E.numrows)
//         return;
//     E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
//     memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
//     E.row[at].size = len;
//     E.row[at].chars = malloc(len + 1);
//     memcpy(E.row[at].chars, s, len);
//     E.row[at].chars[len] = '\0';
//     E.row[at].rsize = 0;
//     E.row[at].render = NULL;
//     editorUpdateRow(&E.row[at]);
//     E.numrows++;
//     E.dirty++;
// }

void editorInsertNewline() {
    if (E.cx == 0) {
        editorInsertRow(E.cy, "", 0);
    } 
    else {
        erow *row = &E.row[E.cy];
        editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
        row = &E.row[E.cy];
        row->size = E.cx;
        row->chars[row->size] = '\0';
        editorUpdateRow(row);
    }
    E.cy++;
    E.cx = 0;
}

void resetFileSize(FILE *fp) {
    fseek(fp, 0, SEEK_END);
    E.filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
}

void editorOpen(char *filename) {
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    // int c;
    // char *tempFilename;
    // FILE *copied;
    FILE *fp;

    if (access(filename, F_OK) != -1) {
        fp = fopen(filename, "r");
        fseek(fp, 0L, SEEK_END);
        E.filesize = ftell(fp);
        rewind(fp);
    } 
    else {
        fp = fopen(filename, "w");
    }

    if (!fp) { 
        die("fopen");
    }

    // get file size
    // resetFileSize(fp);

    // filename -> .filename.tmp -> n+5
    // tempFilename = (char *)malloc(sizeof(char) * strlen(filename) + 6);
    // snprintf(tempFilename,100,".%s.tmp",filename);
    // copied = fopen(tempFilename, "w");

    // if(!copied) {
    //     die("fopen");
    // }

    // free(E.tempfilename);
    // E.tempfilename = strdup(tempFilename);

    // while ((c = fgetc(fp)) != EOF) {
    //     fputc(c, copied);
    // }

    // fclose(fp);
    // fclose(copied);

    // // now the file is in fp not in copied
    // fp = fopen(tempFilename, "r");
    free(E.filename);
    E.filename = strdup(filename);

    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' ||
                               line[linelen - 1] == '\r'))
            linelen--;
        editorInsertRow(E.numrows, line, linelen);
    }
    free(line);
    fclose(fp);
    // free(tempFilename);
    E.dirty = 0;
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

// void editorUpdateRow(erow *row) {
//     int tabs = 0;
//     int j;
//     int idx;

//     for (j = 0; j < row->size; j++)
//         if (row->chars[j] == '\t')
//             tabs++;
//     free(row->render);
//     // add space for the line number
//     // by now fixed at 1
//     // int line_number_size = 2; // number + space
//     row->render = malloc(row->size + tabs * (EDITOR_TAB_STOP - 1) + 1);
//     // row->render[0] = '1';
//     // row->render[1] = ' ';
//     idx = 0;
//     for (j = 0; j < row->size; j++) {
//         if (row->chars[j] == '\t') {
//             row->render[idx++] = ' ';
//             while (idx % EDITOR_TAB_STOP != 0)
//                 row->render[idx++] = ' ';
//         } else {
//             row->render[idx++] = row->chars[j];
//         }
//     }
//     row->render[idx] = '\0';
//     row->rsize = idx;
// }

// delete a char (backspace)
// void editorRowDelChar(erow *row, int at) {
//     if (at < 0 || at >= row->size)
//         return;
//     memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
//     row->size--;
//     editorUpdateRow(row);
//     E.dirty++;
// }

// void editorDelChar() {
//     if (E.cy == E.numrows)
//         return;
//     if (E.cx == 0 && E.cy == 0)
//         return;
//     erow *row = &E.row[E.cy];
//     if (E.cx > 0) {
//         editorRowDelChar(row, E.cx - 1);
//         E.cx--;
//     } else {
//         E.cx = E.row[E.cy - 1].size;
//         editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
//         editorDelRow(E.cy);
//         E.cy--;
//     }
// }

// insert a char in a row
// void editorRowInsertChar(erow *row, int at, int c) {
//     if (at < 0 || at > row->size)
//         at = row->size;
//     row->chars = realloc(row->chars, row->size + 2);
//     memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
//     row->size++;
//     row->chars[at] = c;
//     editorUpdateRow(row);
//     E.dirty++;
// }

// void editorInsertChar(int c) {
//     if (E.cy == E.numrows) {
//         editorInsertRow(E.numrows,"", 0);
//     }
//     editorRowInsertChar(&E.row[E.cy], E.cx, c);
//     E.cx++;
// }

// void editorFreeRow(erow *row) {
//     free(row->render);
//     free(row->chars);
// }
// void editorDelRow(int at) {
//     if (at < 0 || at >= E.numrows)
//         return;
//     editorFreeRow(&E.row[at]);
//     memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
//     E.numrows--;
//     E.dirty++;
// }

// void editorRowAppendString(erow *row, char *s, size_t len) {
//     row->chars = realloc(row->chars, row->size + len + 1);
//     memcpy(&row->chars[row->size], s, len);
//     row->size += len;
//     row->chars[row->size] = '\0';
//     editorUpdateRow(row);
//     E.dirty++;
// }

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

char *editorPrompt(const char *prompt, void (*callback)(char *, int))  {
    size_t bufsize = 128;
    char *buf = malloc(bufsize);
    size_t buflen = 0;
    buf[0] = '\0';
    while (1) {
        editorSetStatusMessage(prompt, buf);
        editorRefreshScreen();
        int c = editorReadKey();
        if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (buflen != 0)
                buf[--buflen] = '\0';
        } 
        else if (c == '\x1b') {
            // editorSetStatusMessage("");
            editorSetStatusMessage(default_status_msg);
            if (callback)
                callback(buf, c);
            free(buf);
            return NULL;
        } 
        else if (c == '\r') {
            if (buflen != 0) {
                // editorSetStatusMessage("");
                editorSetStatusMessage(default_status_msg);
                if (callback)
                    callback(buf, c);

                return buf;
            }
        } 
        else if (!iscntrl(c) && c < 128) {
            if (buflen == bufsize - 1) {
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }
            buf[buflen++] = c;
            buf[buflen] = '\0';
        }
        if (callback)
            callback(buf, c);
    }
}

void editorIndentLine() {
    int prev = E.cx;
    E.cx = 0;
    editorInsertChar('\t');
    E.cx = prev + 1;
}

void editorUnindentLine() {
    if(E.row[E.cy].chars[0] == '\t') {
        editorRowDelChar(&E.row[E.cy],0);
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
        } 
        else {
            len = E.row[filerow].rsize - E.coloff;
            if (len < 0)
                len = 0;
            if (len > E.screencols)
                len = E.screencols;
            // char line_number[10];
            // sprintf(line_number, "%d ", filerow);
            // abAppend(ab, line_number, strlen(line_number));
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

    if(write(STDOUT_FILENO, ab.b, ab.len) < 0) {
        die(strerror(errno));
    }
    abFree(&ab);
}

// move one word right or left
// merge with editorMoveCursor(int key) and then add 
// another param, such as mode (boolean), where 0 means
// single movement and 1 means word movement


// editor cursor commands
// int move: boolean
// 0 -> single 
// 1 -> move by a word (CTRL arrow)
void editorMoveCursor(int key, int mode) {
    int rowlen;
    
    erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

    switch (key) {
        case ARROW_LEFT:
            if (E.cx != 0 && mode == 0) {
                E.cx--;
            } 
            else if (E.cx != 0 && mode == 1) {
                if (E.row[E.cy].chars[E.cx] == ' ') {
                    E.cx--;
                }
                else {
                    while(E.cx > 0 && E.row[E.cy].chars[E.cx] != ' ') {
                        E.cx --;
                    }
                }
            }
            else if (E.cy > 0) {
                E.cy--;
                E.cx = E.row[E.cy].size;
            }
            break;
        case ARROW_RIGHT:
            if (row && E.cx < row->size && mode == 0) {
                E.cx++;
            } 
            else if (row && E.cx < row->size && mode == 1) {
                if (E.row[E.cy].chars[E.cx] == ' ') {
                    E.cx++;
                } else {
                    while (E.cx < row->size && E.row[E.cy].chars[E.cx] != ' ') {
                        E.cx++;
                    }
                }
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
    static int quit_times = EDITOR_QUIT_TIMES;
    // char str[2];
    int i;

    int c = editorReadKey();
    // FILE* fp;

    if(PRINT_KEY) {
        dumpReceivedReadKey(c);
    }

    // reset the status msg in case is not default after 2 sec
    // maybe do it with a flag?
    if (time(NULL) - E.statusmsg_time > 2) {
        editorSetStatusMessage(default_status_msg);
    }

    switch (c) {
        case '\r':
            editorInsertNewline();
            // addOperationToBuffer(InsertLine,NULL,-1,-1);
            break;
        
        // close
        case CTRL_KEY('q'):
            if (E.dirty && quit_times > 0) {
                editorSetStatusMessage("Warning - File has unsaved changes. "
                                       "Press Ctrl-Q %d more times to quit.",
                                       quit_times);
                quit_times--;
                return;
            }
            if(write(STDOUT_FILENO, "\x1b[2J", 4) < 0) {
                die(strerror(errno));
            }
            if(write(STDOUT_FILENO, "\x1b[H", 3) < 0) {
                die(strerror(errno));
            }
            // delete the tempfile
            if(E.tempfilename != NULL) {
                remove(E.tempfilename);
            }
            // do all free in a function
            freeBuffer(&redoStack); 
            freeBuffer(&undoStack); 
            free(E.copyBuffer);
            free(E.gitBranch);
            exit(0); // i do not like it...
            break;

        // duplicate line
        case CTRL_KEY('d'):
            editorInsertRow(E.cy, E.row[E.cy].chars, E.row[E.cy].size);
            // addOperationToBuffer(DuplicateLine,NULL,-1,E.cy);
            break;

        // remove line
        case CTRL_KEY('r'):
            // addOperationToBuffer(DeleteLine, E.row[E.cy].chars, -1, E.cy);
            editorDelRow(E.cy);
            break;

        // copy word
        case CTRL_KEY('c'):
            // fp = fopen("log.txt","a");
            // fprintf(fp, "E.copyBuffer: %s\nE.row[E.cy].chars:%s\nE.row[E.cy].size%d\nE.cx: %d\n", E.copyBuffer, E.row[E.cy].chars, E.row[E.cy].size, E.cx);
            free(E.copyBuffer);
            E.copyBuffer = extractWordFromLine(E.row[E.cy].chars,E.row[E.cy].size,E.cx);
            // fclose(fp);
            break;
            
        // paste word
        case CTRL_KEY('v'):
            if(E.copyBuffer) {
                for(i = 0; i < (int) strlen(E.copyBuffer); i++) {
                    editorInsertChar(E.copyBuffer[i]);
                }
            }
            break;

        // move beginning line
        case CTRL_KEY('b'):
            E.cx = 0;
            break;
        
        // move end line
        case CTRL_KEY('e'):
            E.cx = E.row[E.cy].size;
            break;

        // save
        case CTRL_KEY('s'):
            editorSave();
            break;
        
        // indent line
        case CTRL_KEY('t'):
            editorIndentLine();
            // addOperationToBuffer(IndentLine, NULL, -1, E.cy);
            break;
        
        // unindent line
        case CTRL_KEY('j'):
            editorUnindentLine();
            // addOperationToBuffer(UnindentLine, NULL, -1, E.cy);
            break;

        // move line up
        case CTRL_KEY('w'): {
            int len;
            char *templine;
            if(E.cy > 0) {
                len = E.row[E.cy].size;
                templine = (char *) malloc(sizeof(char)*(len + 1));
                strcpy(templine,E.row[E.cy].chars);
                editorDelRow(E.cy);
                editorInsertRow(E.cy-1,templine,len+1);
                free(templine);
            }
        }
        break;


        // comment line
        case CTRL_KEY('i'): {
            int prev = E.cx;
            int commentPosition = 0;
            int fileExt = getFileCommentChars(E.filename);
            // now the comment position depends from the current indentation
            while(E.row[E.cy].chars[commentPosition] == '\t' || E.row[E.cy].chars[commentPosition] == ' ') {
                commentPosition++;
            }

            E.cx = commentPosition;

            switch (fileExt) {
            case DOUBLESLASH:
                editorInsertChar('/');
                editorInsertChar('/');
                editorInsertChar(' ');
                E.cx = prev + 3;
                break;
            case PERCENT:
                editorInsertChar('%');
                editorInsertChar(' ');
                E.cx = prev + 2;
                break;
            case HASH:
                editorInsertChar('#');
                editorInsertChar(' ');
                E.cx = prev + 2;
                break;
            default:
                break;
            }

        }
        break;     

        // uncomment line
        case CTRL_KEY('u'): {
                int commentPosition = 0;
                int prev = E.cx;
                int space = 0;
                int commentChars = getFileCommentChars(E.filename);

                while (E.row[E.cy].chars[commentPosition] == '\t' || E.row[E.cy].chars[commentPosition] == ' ') {
                    commentPosition++;
                }

                E.cx = commentPosition;
                
                switch (commentChars) {
                    case DOUBLESLASH:
                        if (E.row[E.cy].chars[commentPosition] == '/' && E.row[E.cy].chars[commentPosition + 1] == '/') {
                            if(E.row[E.cy].chars[commentPosition + 2] == ' ') {
                                space = 1;
                            }
                            editorRowDelChar(&E.row[E.cy],commentPosition);
                            editorRowDelChar(&E.row[E.cy],commentPosition);
                        }
                        if(space == 1) {
                            editorRowDelChar(&E.row[E.cy], commentPosition);
                        }
                        E.cx = prev - 2 - space;
                        break;
                    
                    case HASH:
                    case PERCENT:
                        if ((E.row[E.cy].chars[commentPosition] == '%' && commentChars == PERCENT) || (E.row[E.cy].chars[commentPosition] == '#' && commentChars == HASH)) {
                            if (E.row[E.cy].chars[commentPosition + 1] == ' ') {
                                space = 1;
                            }
                            editorRowDelChar(&E.row[E.cy], commentPosition);
                        }
                        if (space == 1) {
                            editorRowDelChar(&E.row[E.cy], commentPosition);
                        }
                        E.cx = prev - 1 - space;
                        break;
                    
                    default:
                        break;
                }
            }

            break;       


        case HOME_KEY:
            E.cx = 0;
            break;

        case END_KEY:
            if (E.cy < E.numrows)
                E.cx = E.row[E.cy].size;
            break;

        case CTRL_KEY('f'):
            editorFind();
            break;

        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if (c == DEL_KEY)
                editorMoveCursor(ARROW_RIGHT,0);
            editorDelChar();
            // str[0] = E.row[E.cy].chars[E.cx]; 
            // str[1] = '\0';
            // addOperationToBuffer(DeleteChar, str, E.cx, E.cy);
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
                editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN,0);
        } break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c,0);
            break;

        case CTRL_KEY('p'):
            editorMoveCursor(ARROW_LEFT, 1);
            break;

        case CTRL_KEY('n'):
            editorMoveCursor(ARROW_RIGHT, 1);
            break;

        case CTRL_KEY('l'):
        case '\x1b':
            break;

        // undo
        case CTRL_KEY('z'):
            bufferOperation(UNDO);
            // dumpBothBuffersFile();
            break;

        // redo
        case CTRL_KEY('y'):
            bufferOperation(REDO);
            // dumpBothBuffersFile();
            break;

        default: 
            editorInsertChar(c);
            addToBuffer(&undoStack, InsertChar, stringFromChar(c), 1, E.cx, E.cy);
            // dumpBothBuffersFile(); 
            break;
    }
    quit_times = EDITOR_QUIT_TIMES;
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

char *editorRowsToString(int *buflen) {
    int totlen = 0;
    int j;
    for (j = 0; j < E.numrows; j++)
        totlen += E.row[j].size + 1;
    *buflen = totlen;
    char *buf = malloc(totlen);
    char *p = buf;
    for (j = 0; j < E.numrows; j++) {
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }
    return buf;
}

void editorSave() {
    int len, fd, written;
    char *buf;
    // FILE *original;
    // FILE *fp;
    // int c;
    if (E.filename == NULL) {
        E.filename = editorPrompt("Save as: %s (ESC to cancel)",NULL);
        // E.tempfilename = strdup(E.filename);
        if (E.filename == NULL) {
            editorSetStatusMessage("Unable to read the filename - unsaved");
            return;
        }
    }

    buf = editorRowsToString(&len);
    // fd = open(E.tempfilename, O_RDWR | O_CREAT, 0644);
    fd = open(E.filename, O_RDWR | O_CREAT, 0644);
    if(fd == -1) {
        editorSetStatusMessage("Unable to save the file\n"); // improve
    } 
    else {
        if(ftruncate(fd,len) == -1) {
            editorSetStatusMessage("Something went wrong with ftruncate: %s\n", strerror(errno));
        }
        else {
            written = write(fd, buf, len);
            if (written == len) {
                close(fd);
                free(buf);
                E.dirty = 0;
                E.filesize = written;
                // now copy all the data to the original file
                // if(strcmp(E.filename,E.tempfilename) != 0) {
                //     fp = fopen(E.tempfilename, "r");
                //     original = fopen(E.filename, "w");
                //     if(fp == NULL || original == NULL) {
                //         die("Bad things happened during the copy");
                //     }
                //     while ((c = fgetc(fp)) != EOF) {
                //         fputc(c, original);
                //     }
                //     resetFileSize(fp);
                //     fclose(original);
                //     fclose(fp);
                // }
                editorSetStatusMessage("%d bytes written to disk", len);
                return;
            }
            else {
                editorSetStatusMessage("Error while writing file\nWritten: %d bytes, supposed: %d bytes\nDo not trust the new file\n", written, len);
            }
        }
        close(fd);
    }
    free(buf);
    editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
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

    editorSetStatusMessage(default_status_msg);
    while(1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    
    return 0;
}