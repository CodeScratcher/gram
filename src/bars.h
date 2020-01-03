// operations on status bar and message bars
#ifndef BARS_H
#define BARS_H

#include "editor.h"

void editorSetStatusMessage(const char *fmt, ...);
void editorDrawStatusBar(struct abuf *ab);
void editorDrawMessageBar(struct abuf *ab);

#endif
