#ifndef FILENAME_H
#define FILENAME_H

typedef struct
{
    Uint32 id;
    char filename[32];
}
FData;

extern FData translateID(Uint32 id);

#endif

