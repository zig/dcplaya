#include <stdio.h>

extern char toto_str[];
void ko_main()
{
  printf("Hello World !\n");

  toto(toto_str);
}

