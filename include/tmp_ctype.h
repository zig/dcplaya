// VP : complete missing KOS ctype
// TEMPORARY --> Update to a new KOS instead ...

#include <ctype.h>

#define isspace(c) ( (c) == ' ' || (c) == '\n' || (c) == '\t' || (c) == 'f' || (c) == '\v' )

#define isalpha(c) (islower(c) || isupper(c))
#define isalnum(c) (isalpha(c) || isdigit(c))

// VP : ?
#define iscntrl(c) ((c) < 32 || (c) > 127)

#define ispunct(c) ( !isspace(c) && !isalnum(c) )

// VP : hexadecimal characters
#define isxdigit(c) ( isdigit(c) || ( (c) >= 'a' && (c) <= 'h') || ( (c) >= 'A' && (c) <= 'H') )

