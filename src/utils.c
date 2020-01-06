#include <stdlib.h>
#include <string.h>

#include "utils.h"

// messy but works 
int getFileCommentChars(char *filename) {
    int i = 0;
    char *ext = NULL;

    if(!filename) {
        return NOEXT;
    }

    if (strcmp(filename, "Makefile") == 0) {
        return HASH;
    }

    while (filename[i] != '\0' && filename[i] != '.') {
        i++;
    }
    if (i < (int)strlen(filename)) {

        ext = (char *)malloc(sizeof(char) * (strlen(filename) - i));
        memcpy(ext, &filename[i + 1], strlen(filename) - i);

        if (strcmp(ext, "c") == 0 || strcmp(ext, "cpp") == 0 || strcmp(ext, "h") == 0) {
            free(ext);
            return DOUBLESLASH;
        } else if (strcmp(ext, "pl") == 0) {
            free(ext);
            return PERCENT;
        } else if (strcmp(ext, "sh") == 0 || strcmp(ext, "py") == 0) {
            free(ext);
            return HASH;
        } else {
            free(ext);
            return NOEXT;
        }
    } else {
        return NOEXT;
    }
}