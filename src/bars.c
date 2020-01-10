#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "bars.h"

void editorSetStatusMessage(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}

// draw status bar on the bottom of the screen
// filename max 20 char
void editorDrawStatusBar(struct abuf *ab) {
    int len, rlen;
    char tempCopyBuffer[20];
    char status[80], rstatus[80];
    abAppend(ab, "\x1b[7m", 4);
    if(E.copyBuffer) {
        if (strlen(E.copyBuffer) > 19) {
            // strncpy(tempCopyBuffer, E.copyBuffer, 7);
            strcpy(tempCopyBuffer,"[too long]");
        } 
        else {
            strcpy(tempCopyBuffer, E.copyBuffer);
        }
    }
    else {
        strcpy(tempCopyBuffer,"empty");
    }
    

    len = snprintf(status, sizeof(status), "%.20s%s (%d bytes) - %d lines - branch: %s - buffer: %s",
                   E.filename ? E.filename : "[No Name]",
                   E.dirty ? "*" : "", E.filesize, E.numrows, E.gitBranch, tempCopyBuffer);
    rlen = snprintf(rstatus, sizeof(rstatus), "%d,%d",
                    E.cx + 1, E.cy + 1);
    if (len > E.screencols)
        len = E.screencols;
    abAppend(ab, status, len);
    while (len < E.screencols) {
        if (E.screencols - len == rlen) {
            abAppend(ab, rstatus, rlen);
            break;
        } else {
            abAppend(ab, " ", 1); // fill the line with spaces
            len++;
        }
    }
    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}

void editorDrawMessageBar(struct abuf *ab) {
    int msglen;
    abAppend(ab, "\x1b[K", 3);
    msglen = strlen(E.statusmsg);
    if (msglen > E.screencols)
        msglen = E.screencols;
    // status message only for 5 seconds
    // if (msglen && time(NULL) - E.statusmsg_time < 5)
    if (msglen)
        abAppend(ab, E.statusmsg, msglen);
}