// VP : complete missing KOS ctype
// TEMPORARY --> Update to a new KOS instead ...

#include <ctype.h>

#define isspace(c) ( (c) == ' ' || (c) == '\n' || (c) == '\t' )



#define isalpha(c) (islower(c) || isupper(c))
#define isalnum(c) (isalpha(c) || isdigit(c))

// VP : ?
#define iscntrl(c) ((c) != ' ' && !isprint(c))

#define ispunct(c) ( (c)=='.' || (c)==';' || (c)==',' || (c)==':' || (c)=='?' || (c)=='!' )

// VP : hexa ?
#define isxdigit(c) ( isdigit(c) || ( (c) >= 'a' && (c) <= 'h') || ( (c) >= 'A' && (c) <= 'H') )

