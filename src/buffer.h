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
    int flag; // -1 if undo +1 if redo 0 nothing
    char *data; // data removed or added
} opstruct;

typedef struct buffer {
    int bufsize;
    int position; // position of the last operation
    opstruct oplist[MAX_BUFSIZE];
} buffer;

void initBuffer();
void freeBuffer();
void dumpBuffer();
void addOperationToBuffer(int operation, char *data, int lenData, int px, int py);
void bufferOperation (int redo);
void bufferUndoOperation();
void bufferRedoOperation();

buffer buff;