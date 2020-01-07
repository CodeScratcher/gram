enum extensions {
    NOEXT,DOUBLESLASH,PERCENT,HASH
};

int getFileCommentChars(char *filename);
char *getGitBranch();
// void stringFromChar(char c, char *str);
char *extractWordFromLine(char *line, int len, int pos);
char *stringFromChar(char c);