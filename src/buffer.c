#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "editor.h"

void initBuffer(buffer *buff, const char *name) {
    int i;
    for (i = 0; i < MAX_BUFSIZE; i++) {
        buff->oplist[i].data = NULL;
        buff->oplist[i].lenData = 0;
        buff->oplist[i].operation = NOP;
        buff->oplist[i].px = -2; // value that must be modified
        buff->oplist[i].py = -2;
    }
    buff->bufsize = 0;
    undoStack.bufsize = 0;
    strcpy(buff->name,name);
}

void freeBuffer(buffer *buff) {
    int i;
    for (i = 0; i < MAX_BUFSIZE; i++) {
        if (buff->oplist[i].data) {
            free(buff->oplist[i].data);
        }
    }
}

void dumpBothBuffersFile() {
    FILE *fp;
    fp = fopen("log.txt", "a");
    fprintf(fp,"Undo stack:\n");
    dumpBufferFile(undoStack,fp);
    fprintf(fp,"Redo stack\n");
    dumpBufferFile(redoStack,fp);
    printf("\n");
    fclose(fp);
}

void dumpBufferFile(buffer buff, FILE *fp) {
    int i;
    fprintf(fp,"size: %d\n", buff.bufsize);
    for (i = 0; i < buff.bufsize; i++) {
        fprintf(fp,"buff[%d]: \n\tdata: %s \n\toperation: %d \n\tpx: %d \n\tpy: %d\n", i, buff.oplist[i].data, buff.oplist[i].operation, buff.oplist[i].px, buff.oplist[i].py);
    }
}

void dumpBuffer(buffer buff) {
    int i;
    printf("size: %d\n", buff.bufsize);
    for (i = 0; i < buff.bufsize; i++) {
        printf("buff[%d]: \n\tdata: %s \n\toperation: %d \n\tpx: %d \n\tpy: %d\n", i, buff.oplist[i].data, buff.oplist[i].operation, buff.oplist[i].px, buff.oplist[i].py);
    }
}

int invertOperation(int operation) {
    switch (operation) {
    case NOP:
        return NOP;
    case InsertChar:
        return DeleteChar;
    case InsertLine:
    case DuplicateLine:
        return DeleteLine;
    case IndentLine:
        return UnindentLine;
    default:
        return NOP;
    }
}

void addToBuffer(buffer *buff, int operation, char *data, int lenData, int px, int py) {
    int i;
    // printf("chiamata: data %s, lendata %d\n", data, lenData);
    // if the buffer is full, move everithing one to left
    if (buff->bufsize == MAX_BUFSIZE) {
        for (i = 1; i < MAX_BUFSIZE; i++) {
            free(buff->oplist[i - 1].data);
            buff->oplist[i - 1].data = (char *)malloc(sizeof(char) * (buff->oplist[i].lenData + 1));
            strcpy(buff->oplist[i - 1].data, buff->oplist[i].data);
            buff->oplist[i - 1].lenData = buff->oplist[i].lenData;
            buff->oplist[i - 1].operation = buff->oplist[i].operation;
            buff->oplist[i - 1].px = buff->oplist[i].px;
            buff->oplist[i - 1].py = buff->oplist[i].py;
        }
        buff->oplist[MAX_BUFSIZE - 1].operation = operation;
        free(buff->oplist[MAX_BUFSIZE - 1].data);
        buff->oplist[MAX_BUFSIZE - 1].data = (char *)malloc(sizeof(char) * (lenData + 1));
        strcpy(buff->oplist[MAX_BUFSIZE - 1].data, data);
        free(data); // because the data was allocated with malloc, at least for char
        buff->oplist[MAX_BUFSIZE - 1].lenData = lenData;
        buff->oplist[MAX_BUFSIZE - 1].px = px;
        buff->oplist[MAX_BUFSIZE - 1].py = py;

    } else {
        buff->oplist[buff->bufsize].operation = operation;
        buff->oplist[buff->bufsize].lenData = lenData;

        if (data) {
            // printf("data: %s\nlendata: %d\n",data,lenData);
            buff->oplist[buff->bufsize].data = (char *)malloc(sizeof(char) * (lenData + 1));
            strcpy(buff->oplist[buff->bufsize].data, data);
            free(data);
            data = NULL;
        }

        buff->oplist[buff->bufsize].px = px;
        buff->oplist[buff->bufsize].py = py;

        buff->bufsize++;
    }
}

void popFromBuffer(buffer *buff) {
    if (buff->oplist[buff->bufsize - 1].data) {
        // free(buff->oplist[buff->bufsize - 1].data);
        buff->oplist[buff->bufsize - 1].data = NULL;
    }
    buff->oplist[buff->bufsize - 1].lenData = -1;
    buff->oplist[buff->bufsize - 1].operation = NOP;
    buff->oplist[buff->bufsize - 1].px = -1;
    buff->oplist[buff->bufsize - 1].py = -1;

    buff->bufsize--;
}

void popFromOneAndPushToTheOther(buffer *toPop, buffer *toPush) {
    addToBuffer(toPush, toPop->oplist[toPop->bufsize - 1].operation, toPop->oplist[toPop->bufsize - 1].data, toPop->oplist[toPop->bufsize - 1].lenData, toPop->oplist[toPop->bufsize - 1].px, toPop->oplist[toPop->bufsize - 1].py);
    popFromBuffer(toPop);
}

void bufferOperation(int type) {
    if (type == UNDO) {
        bufferExecOperation(&undoStack, &redoStack);
    } else { // redo
        bufferExecOperation(&redoStack, &undoStack);
    }
}

void bufferExecOperation(buffer *bufferFrom, buffer *bufferTo) {
    int px,py;
    int operation;

    if (bufferFrom->bufsize > 0) {    
        if(strcmp(bufferFrom->name,"redo") == 0) {
            operation = invertOperation(bufferFrom->oplist[bufferFrom->bufsize - 1].operation);
        }
        else {
            operation = bufferFrom->oplist[bufferFrom->bufsize - 1].operation;
        }
        // TODO: refactor with function as parameter for save cursor position
        // and reset cursor position, something like saveExecuteAndRestore
        switch (operation) {
            // i do the opposite
            case InsertChar:
                // move to the char position
                px = E.cx;
                py = E.cy;
                E.cx = bufferFrom->oplist[bufferFrom->bufsize - 1].px;
                E.cy = bufferFrom->oplist[bufferFrom->bufsize - 1].py;
                editorDelChar();
                E.cx = px - 1;
                E.cy = py;

                popFromOneAndPushToTheOther(bufferFrom, bufferTo);

                break;

            case DeleteChar:
                px = E.cx;
                py = E.cy;
                E.cx = bufferFrom->oplist[bufferFrom->bufsize - 1].px;
                E.cy = bufferFrom->oplist[bufferFrom->bufsize - 1].py;
                editorInsertChar(bufferFrom->oplist[bufferFrom->bufsize - 1].data[0]);
                E.cx = px;
                E.cy = py;

                popFromOneAndPushToTheOther(bufferFrom, bufferTo);

            default:
                break;
        }
    }

}
