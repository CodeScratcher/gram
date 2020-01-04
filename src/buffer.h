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
    char *data; // data removed or added
} opstruct;

typedef struct buffer {
    int bufsize;
    opstruct oplist[MAX_BUFSIZE];
} buffer;

void initBuffer();
void freeBuffer();
void dumpBuffer();
void addOperationToBuffer(int operation, char *data, int px, int py);

buffer buff;