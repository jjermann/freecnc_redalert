#include <stdlib.h>
#include <zlib.h>

int main(int argc, char** argv)
{
    char (*f)() = (char (*)())gzread;
    if (getenv("TEST_FAKENOZLIB")) {
        return 1;
    }
    return f != (char (*)())gzread;
}
