
#include <stdio.h>
#include <string.h>
#include <dc/net/broadband_adapter.h>
#include <kos/net.h>

extern netif_t bba_if;

netif_t * netif = &bba_if;

int lef_main()
{
  return 0;
}


int driver_init()
{
  return bba_init();
}

int driver_shutdown()
{
  return bba_shutdown();
}

