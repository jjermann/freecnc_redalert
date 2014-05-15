#include <stdlib.h>
#include "xmms/util.h"

int main(int argc, char** argv)
{
    void (*f)(gint) = (void (*)(gint))xmms_usleep;
    if (getenv("TEST_FAKENOXMMS")) {
        return 1;
    }
    return f != (void (*)(gint))xmms_usleep;
}

