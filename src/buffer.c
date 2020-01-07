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
        }
        buff.oplist[MAX_BUFSIZE - 1] .operation = operation;
        free(buff.oplist[MAX_BUFSIZE - 1].data);
        buff.oplist[MAX_BUFSIZE - 1].data = (char *)malloc(sizeof(char) * (lenData + 1));
        strcpy(buff.oplist[MAX_BUFSIZE - 1].data, data);
        free(data); // because the data was allocated with malloc, at least for char
        buff.oplist[MAX_BUFSIZE - 1].lenData = lenData;
        buff.oplist[MAX_BUFSIZE - 1].px = px;
        buff.oplist[MAX_BUFSIZE - 1].py = py;
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

        buff.bufsize++;
        buff.position++;
    }
}

void bufferUndoOperation() {

    if (buff.bufsize == 0 || buff.position == -1) {
        return;
    }

    if (buff.oplist[buff.position].operation == InsertChar) {
        // delete char
    }
}

void bufferRedoOperation() {

}