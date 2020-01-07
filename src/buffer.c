#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "editor.h"

void initBuffer() {
    int i;
    for (i = 0; i < MAX_BUFSIZE; i++) {
        buff.oplist[i].data = NULL;
        buff.oplist[i].lenData = 0;
        buff.oplist[i].operation = NOP;
        buff.oplist[i].px = -2; // value that must be modified
        buff.oplist[i].py = -2;
    }
    buff.bufsize = 0;
    buff.position = -1;
}

void freeBuffer() {
    int i;
    for (i = 0; i < MAX_BUFSIZE; i++) {
        if (buff.oplist[i].data) {
            free(buff.oplist[i].data);
        }
    }
}

void dumpBuffer() {
    int i;
    for (i = 0; i < buff.bufsize; i++) {
        printf("buff[%d]: %s %d %d %d\n", i, buff.oplist[i].data, buff.oplist[i].operation, buff.oplist[buff.bufsize].px, buff.oplist[buff.bufsize].py = -2);
    }
}

void addOperationToBuffer(int operation, char *data, int lenData, int px, int py) {
    int i;
    // if the buffer is full, move everithing one to left
    if (buff.bufsize == MAX_BUFSIZE) {
        for (i = 1; i < MAX_BUFSIZE; i++) { 
            free(buff.oplist[i-1].data);
            buff.oplist[i-1].data = (char *) malloc(sizeof(char)*(buff.oplist[i].lenData+1));
            strcpy(buff.oplist[i-1].data,buff.oplist[i].data);
            buff.oplist[i - 1].lenData = buff.oplist[i].lenData;
            buff.oplist[i - 1].operation = buff.oplist[i].operation;
            buff.oplist[i - 1].px = buff.oplist[i].px;
            buff.oplist[i - 1].py = buff.oplist[i].py;
            buff.oplist[i - 1].flag = buff.oplist[i].flag;
        }
        buff.oplist[MAX_BUFSIZE - 1] .operation = operation;
        free(buff.oplist[MAX_BUFSIZE - 1].data);
        buff.oplist[MAX_BUFSIZE - 1].data = (char *)malloc(sizeof(char) * (lenData + 1));
        strcpy(buff.oplist[MAX_BUFSIZE - 1].data, data);
        free(data); // because the data was allocated with malloc, at least for char
        buff.oplist[MAX_BUFSIZE - 1].lenData = lenData;
        buff.oplist[MAX_BUFSIZE - 1].px = px;
        buff.oplist[MAX_BUFSIZE - 1].py = py;
        buff.oplist[MAX_BUFSIZE - 1].flag = 0;
        
    } 
    else {
        buff.oplist[buff.bufsize].operation = operation;
        if (data) {
            buff.oplist[buff.bufsize].data = (char *)malloc(sizeof(char) * (lenData + 1));
            strcpy(buff.oplist[buff.bufsize].data, data);
            free(data);
        }

        buff.oplist[buff.bufsize].px = px;
        buff.oplist[buff.bufsize].py = py;
        buff.oplist[buff.bufsize].flag = 0;

        buff.bufsize++;
        buff.position++;
    }
}

void bufferOperation(int redo) {
    int px,py;

    if (buff.bufsize == 0 || (buff.position == -1 && redo == 0) || (buff.position == buff.bufsize && redo == 1)) {
        return;
    }

    // TODO: refactor with function as parameter for save cursor position
    // and reset cursor position, something like saveExecuteAndRestore
    switch (buff.oplist[buff.position].operation) {
        case InsertChar:
            // move to the char position
            px = E.cx;
            py = E.cy;
            E.cx = buff.oplist[buff.position + redo].px;
            E.cy = buff.oplist[buff.position + redo].py;
            editorDelChar();
            E.cx = px;
            E.cy = py;

            // -1 * 2*redo -> redo == 0 -> undo (-1), redo == 1 -> redo (+1)
            buff.position = buff.position - 1 + 2 * redo;
        
            break;
    
        case DeleteChar:
            px = E.cx;
            py = E.cy;
            E.cx = buff.oplist[buff.position + redo].px;
            E.cy = buff.oplist[buff.position + redo].py;
            editorInsertChar(buff.oplist[buff.position].data[0]);
            E.cx = px;
            E.cy = py;

            buff.position = buff.position - 1 + 2 * redo;

        default:
            break;
    }

}

void bufferUndoOperation() {
    bufferOperation(0);
}

void bufferRedoOperation() {
    bufferOperation(1);
}