#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"

// causes errors, maye the bug is somewhere else
void initBuffer() {
    int i;
    for (i = 0; i < MAX_BUFSIZE; i++) {
        // buff.oplist[i].data = NULL;
        buff.oplist[i].operation = NOP;
        buff.oplist[i].px = -2; // value that must be modified
        buff.oplist[i].py = -2;
    }
    buff.bufsize = 0;
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

void addOperationToBuffer(int operation, char *data, int px, int py) {
    int i;

    // if the buffer is empty, move everithing one to left
    if (buff.bufsize == MAX_BUFSIZE) {
        for (i = 1; i < MAX_BUFSIZE; i++) { // maybe replace with mmove?
            buff.oplist[i - 1] = buff.oplist[i];
        }
        buff.oplist[MAX_BUFSIZE - 1].operation = operation;
        strcpy(buff.oplist[MAX_BUFSIZE - 1].data, data);
        buff.oplist[MAX_BUFSIZE - 1].px = px;
        buff.oplist[MAX_BUFSIZE - 1].py = py;
    } else {
        buff.oplist[buff.bufsize].operation = operation;
        if (data) {
            buff.oplist[buff.bufsize].data = (char *)malloc(sizeof(char) * (strlen(data) + 1));
            strcpy(buff.oplist[buff.bufsize].data, data);
        }

        buff.oplist[buff.bufsize].px = px;
        buff.oplist[buff.bufsize].py = py;

        buff.bufsize++;
    }
}
