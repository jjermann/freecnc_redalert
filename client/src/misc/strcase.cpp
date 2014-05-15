#include <cctype>
#include <cstdlib>

int strcasecmp(const char *str1, const char *str2)
{
    while( *str1 != '\0' && *str2 != '\0' ) {
        if( toupper(*str1) != toupper(*str2) )
            return 1;
        str1++;
        str2++;
    }
    return (*str1 != *str2);
}

int strncasecmp(const char *str1, const char *str2, size_t len)
{
    while( *str1 != '\0' && *str2 != '\0' && len > 0) {
        if( toupper(*str1) != toupper(*str2) )
            return 1;
        str1++;
        str2++;
        len--;
    }
    return (len>0 && toupper(*str1) != toupper(*str2));
}
