#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

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

// creates a child process that exec git branch
char *getGitBranch() {
    int fd[2];
    int pid;
    int nbytes;
    int maxLenGitBranchName = 80;
    char *readbuffer = NULL;

    if (pipe(fd) < 0) {
        return NULL;
    }

    pid = fork();

    if (pid == 0) {
        // child process
        close(fd[0]); // close input
        dup2(fd[1], 1);
        close(fd[1]);
        // execute the command git rev-parse --abbrev-ref HEAD
        execlp("git", "git", "rev-parse", "--abbrev-ref", "HEAD", NULL);
    } else if (pid > 0) {
        // parent
        close(fd[1]); // close output
        // wait(NULL);
        readbuffer = (char *)malloc(sizeof(char) * maxLenGitBranchName);
        nbytes = read(fd[0], readbuffer, (maxLenGitBranchName - 1));
        if (nbytes > 0) {
            readbuffer[nbytes - 1] = '\0';
            // editorSetStatusMessage(readbuffer);
            if (!strstr(readbuffer, "Not a git repository")) {
                // free(E.gitBranch);
                // E.gitBranch = strdup(readbuffer);
                return readbuffer;
            }
        }
    }

    free(readbuffer);
    return NULL;
}

char *extractWordFromLine(char *line, int len, int pos) {
    int lower, upper;
    int i = pos;
    char *word = NULL;
    if (pos > len) {
        return NULL;
    } else if (line[pos] == ' ' || line[pos] == '\n' || line[pos] == '\t') {
        word = (char *)malloc(sizeof(char) * 2);
        word[0] = ' ';
        word[1] = '\0';
    } else {
        // move right and left to find a space
        while (i >= 0 && line[i] != ' ' && line[i] != '\t' && line[i] != '\n') {
            i--;
        }
        lower = i + 1;
        i = pos;
        while (i < len && line[i] != ' ' && line[i] != '\t' && line[i] != '\n') {
            i++;
        }
        upper = i;
        // quick fix if the cursor position is on space
        if (lower == upper + 1) {
            lower = upper;
        }
        // extract the word
        word = (char *)malloc(sizeof(char) * (upper - lower + 1));
        strncpy(word, line + lower, upper - lower);
        word[upper - lower] = '\0';
    }
    return word;
}

// // used to store data in buffer
// void stringFromChar(char c, char *str) {
//     *str = c;
//     *(str + 1) = '\0';
// }

// same as above, remember to FREE the buffer
char *stringFromChar(char c) {
    char *str = malloc(sizeof(char)*2);
    *str = c;
    *(str + 1) = '\0';
    return str;
}
