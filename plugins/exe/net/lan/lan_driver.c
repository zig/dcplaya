
#include <stdio.h>
#include <string.h>
#include <dc/net/broadband_adapter.h>
#include <kos/net.h>

extern netif_t la_if;

netif_t * netif = &la_if;

int lef_main()
{
  return 0;
}


int driver_init()
{
  return la_init();
}

int driver_shutdown()
{
  return la_shutdown();
}

