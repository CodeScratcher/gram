#define MAX_BUFSIZE 100

enum operations { NOP,
                  InsertChar,
                  DeleteChar,
                  InsertLine,
                  DeleteLine,
                  DuplicateLine,
                  IndentLine,
                  UnindentLine
                };

enum bufferKind {UNDO, REDO};

// not very portable...
// operation is one of the enum
// px and py are the position of the operation
//  if both of them are -1 then there is no position
//  if px is -1 but py is > 0 this is a duplicate line
//  if both are > -1 then this is an isert char 
typedef struct opstruct {
    int operation;
    int px,py; // position x and y
    int lenData;
    char *data; // data removed or added
} opstruct;

typedef struct buffer {
    int bufsize;
    char name[5]; // undo or redo
    opstruct oplist[MAX_BUFSIZE];
} buffer;

void initBuffer(buffer *buff, const char *name);
void freeBuffer(buffer *buff);
void dumpBuffer(buffer buff);
void dumpBothBuffersFile();
void dumpBufferFile(buffer buff, FILE *fp);
void addToBuffer(buffer *buff, int operation, char *data, int lenData, int px, int py);
void popFromBuffer(buffer *buff); 
void popFromOneAndPushToTheOther(buffer *toPop, buffer *toPush);
void bufferOperation(int type);
void bufferExecOperation(buffer *bufferFrom, buffer *bufferTo);
int invertOperation(int operation);

// The idea is to keep 2 stack, for undo e redo
// when i push ctrl z (undo), pop one action to undo
// and push it to redo and viceversa

buffer undoStack;
buffer redoStack;