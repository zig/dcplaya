/**
 * @ingroup  exe_plugin
 * @file     net_driver.c
 * @author   Vincent Penne <ziggy@sashipa.com> 
 * @date     2004/03/13
 * @brief    network extension plugin
 * 
 * $Id: net_driver.c,v 1.2 2004-07-04 14:16:45 vincentp Exp $
 */

#include <stdlib.h>
#include <string.h>

#include <kos.h>

#include "dcplaya/config.h"
#include "luashell.h"
#include "lef.h"
#include "driver_list.h"
#include "net_driver.h"

#include "net.h"

#include "lwip/lwip.h"

/** Lua side init flags. */
static int init;
static int netinit;
static int lwipinit;

static int driver_shutdown(any_driver_t * d);


static net_input_func saved_rx_callback;
net_input_func lwip_cb;
void rx_callback(netif_t * netif, uint8 *pkt, int pktsize);

netif_t * netif;

/* Driver init : not much to do. */ 
static int driver_init(any_driver_t *d)
{
  printf("%s_driver_init ... \n", d->name);
  init = 0;

  return 0;

/*  error: */
/*   driver_shutdown(d); */
/*   return -1; */
}

/* Driver shutdown */
static int driver_shutdown(any_driver_t * d)
{
  printf("%s_driver_shutdown ... \n", d->name);

  return 0;
}

static driver_option_t * driver_options(any_driver_t * d, int idx,
                                        driver_option_t * o)
{
  return o;
}


int lua_net_init(lua_State * L)
{
  if (init) {
    goto ok;
  }

  saved_rx_callback = net_input_target;

  printf("net_driver LUA initialized.\n");
  init = 1;
 ok:
  lua_settop(L,1);
  lua_pushnumber(L,1);
  return 1;
}

static int lua_net_shutdown(lua_State * L)
{
  int n;

  if (!init) {
    goto ok;
  }

  fs_shutdown();

  if (lwipinit) {
    lwip_shutdown_all();
    lwipinit = 0;
  }

  if (netinit) {
    net_input_set_target(0);
    netinit = 0;
  }

  init = 0;
  printf("net_driver LUA shutdown.\n");
 ok:
  lua_settop(L,0);
  lua_pushnumber(L,1);
  return 1;
}

extern uint8 eth_mac[6];

static int lua_net_connect(lua_State * L)
{
  int res = -1;
  struct netif_list * listhead;

  dbglog_set_level(7);

  if (netinit)
    return 0;

  printf("calling eth_init\n");

  net_init();
  netinit = 1;
  
  // Find a device for us
  listhead = net_get_if_list();
  LIST_FOREACH(netif, listhead, if_list) {
    if (netif->flags & NETIF_RUNNING)
      break;
  }
  if (netif == NULL) {
    printf("can't find an active KOS network device\n");
    return 0;
  }

  if (netif) {
    uint8 mac[6];
    int i;

    printf("Ethernet adapter initialized succesfully !\n");
    res = 0;

    memcpy(eth_mac, netif->mac_addr, sizeof(eth_mac));
    printf("MAC address : ");
    for (i=0; i<6; i++)
      printf("%x ", (int) eth_mac[i]);
    printf("\n");

    printf("Set IP : %d.%d.%d.%d\n", (int) lua_tonumber(L, 1), 
	   (int) lua_tonumber(L, 2), 
	   (int) lua_tonumber(L, 3), 
	   (int) lua_tonumber(L, 4));
    eth_setip(lua_tonumber(L, 1), 
	      lua_tonumber(L, 2), 
	      lua_tonumber(L, 3), 
	      lua_tonumber(L, 4));

    net_input_set_target(rx_callback);

    if (!fs_init())
      printf("NFS initialised succesfully\n");
  }

  lua_pushnumber(L, res);

  return 1;
}

static int lua_net_disconnect(lua_State * L)
{
  printf("calling eth_shutdown\n");

  if (netinit) {
    net_input_set_target(0);
    netinit = 0;
  }

  return 0;
}

static int lua_net_print(lua_State * L)
{
  const char * msg = lua_tostring(L, 1);
  net_lock();
  sc_write(0, msg, strlen(msg));
  net_unlock();

  return 0;
}

static int lua_lwip_init(lua_State * L)
{
  struct ip_addr ip;
  struct ip_addr mask;
  struct ip_addr gw;

  IP4_ADDR(&ip, 192, 168, 1, 9);
  IP4_ADDR(&mask, 255, 255, 255, 0);
  IP4_ADDR(&gw, 192, 168, 1, 2);

  //lwip_init_mac();
  lwip_init_all_static(&ip, &mask, &gw);
  //lwip_init_common("test");

  //lwip_init_all();

  lwip_cb = net_input_target;
  net_input_set_target(rx_callback);

  lwipinit = 1;

  return 0;
}

#include "lwip/sockets.h"
static int lua_gethostbyname(lua_State * L)
{
  struct sockaddr_in dnssrv;
  uint8 ip[4];
  
  dnssrv.sin_family = AF_INET;
  dnssrv.sin_port = htons(53);
  if (dns("212.198.2.51", &dnssrv.sin_addr.s_addr))
    return 0;
  //dnssrv.sin_addr.s_addr = ntohl(dnssrv.sin_addr.s_addr);
  printf("server ip %x\n", dnssrv.sin_addr.s_addr);

  if (lwip_gethostbyname(&dnssrv, lua_tostring(L, 1), ip) < 0)
  //if (lwip_gethostbyname2("212.198.2.51", lua_tostring(L, 1), ip) < 0)
    printf("Can't look up name");
  else {
    printf("ip of '%s' is %d.%d.%d.%d\n", lua_tostring(L, 1),
	   ip[0], ip[1], ip[2], ip[3]);
  }

  return 0;
}

static luashell_command_description_t driver_commands[] = {

  /* Internal commands */
  {
    /* long names, short names and topic */
    DRIVER_NAME"_driver_init", 0, 0,
    /* usage */
    DRIVER_NAME"_driver_init() : "
    "INTERNAL ; initialize lua side of " DRIVER_NAME " driver.",
    /* function */
    SHELL_COMMAND_C, lua_net_init
  },

  {
    /* long names, short names and topic */
    DRIVER_NAME"_driver_shutdown", 0, 0,
    /* usage */
    DRIVER_NAME"_driver_shutdown() : "
    "INTERNAL ; shutdown lua side of " DRIVER_NAME " driver.",
    /* function */
    SHELL_COMMAND_C, lua_net_shutdown
  },

  {
    /* long names, short names and topic */
    DRIVER_NAME"_lwip_init", 0, 0,
    /* usage */
    DRIVER_NAME"_lwip().",
    /* function */
    SHELL_COMMAND_C, lua_lwip_init,
  },

  {
    /* long names, short names and topic */
    DRIVER_NAME"_connect", 0, 0,
    /* usage */
    DRIVER_NAME"_connect(ip0, ip1, ip2, ip3) : "
    "Connect the BBA and set its ip (four parameters in order).",
    /* function */
    SHELL_COMMAND_C, lua_net_connect
  },

  {
    /* long names, short names and topic */
    DRIVER_NAME"_disconnect", 0, 0,
    /* usage */
    DRIVER_NAME"_disconnect() : "
    "Disconnect the BBA.",
    /* function */
    SHELL_COMMAND_C, lua_net_disconnect
  },

  {
    /* long names, short names and topic */
    DRIVER_NAME"_print", 0, 0,
    /* usage */
    DRIVER_NAME"_print(message) : "
    "Send a console message through the BBA.",
    /* function */
    SHELL_COMMAND_C, lua_net_print
  },

  {
    /* long names, short names and topic */
    DRIVER_NAME"_gethostbyname", 0, 0,
    /* usage */
    DRIVER_NAME"_gethostbyname(name) : "
    "Query DNS for ip adress.",
    /* function */
    SHELL_COMMAND_C, lua_gethostbyname
  },


 
  {0},                                    /* end of the command list */
};

any_driver_t net_driver =
  {

    0,                     /**< Next driver                     */
    EXE_DRIVER,            /**< Driver type                     */      
    0x0100,                /**< Driver version                  */
    DRIVER_NAME,           /**< Driver name                     */
    "Vincent Penne",       /**< Driver authors                  */
    "Network support."     /**< Description                     */
    ,
    0,                     /**< DLL handler                     */
    driver_init,           /**< Driver init                     */
    driver_shutdown,       /**< Driver shutdown                 */
    driver_options,        /**< Driver options                  */
    driver_commands,       /**< Lua shell commands              */
  
  };

EXPORT_DRIVER(net_driver)