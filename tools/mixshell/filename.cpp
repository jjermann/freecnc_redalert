#include "SDL.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filename.h"

FData translateID(Uint32 id)
{
    FILE *databas;
    FData result;
    char line[1024];
    databas = fopen("mixfiles.txt", "r");
    if( databas == NULL ) {
        fprintf(stderr, "Unable to find namedb\n");
        exit(1);
    }
    while( fgets(line, 1024, databas) != NULL ) {
        sscanf(line, "%u %s", &(result.id), result.filename);
        if( result.id == id ) {
            return result;
        }
    }
    //printf("%d not found\n", id);
    result.id = id;
    strcpy(result.filename,"[UNKNOWN]");
    return result;
}
