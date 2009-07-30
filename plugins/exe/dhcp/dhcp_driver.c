/**
 * @ingroup  exe_plugin
 * @file     dhcp_driver.c
 * @author   Vincent Penne <ziggy@sashipa.com> 
 * @date     2009/07/26
 * @brief    DHCP implementation
 * 
 * $Id: net_driver.c,v 1.4 2007-03-17 14:40:29 vincentp Exp $
 */

#include <stdlib.h>
#include <string.h>

#include <kos.h>

#include "dcplaya/config.h"
#include "luashell.h"
#include "lef.h"
#include "driver_list.h"
#include "dhcp_driver.h"

#include "net.h"

#include "lwip/lwip.h"

#include "dhcp.h"

static int init; /* is inited */

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


int lua_dhcp_init(lua_State * L)
{
  if (init) {
    goto ok;
  }

  dhcp_cb = dhcp_handle;

  printf("dhcp_driver LUA initialized.\n");
  init = 1;
 ok:
  lua_settop(L,1);
  lua_pushnumber(L,1);
  return 1;
}

static int lua_dhcp_shutdown(lua_State * L)
{
  if (!init) {
    goto ok;
  }

  dhcp_cb = NULL;

  init = 0;
  printf("dhcp_driver LUA shutdown.\n");
 ok:
  lua_settop(L,0);
  lua_pushnumber(L,1);
  return 1;
}

int rand();
static int lua_dhcp_discover(lua_State * L)
{
  dhcp_discover(rand());

  lua_settop(L,0);
  lua_pushnumber(L,1);
  return 1;
}

static int lua_dhcp_request(lua_State * L)
{
  dhcp_request();

  lua_settop(L,0);
  lua_pushnumber(L,1);
  return 1;
}

static char *ip2str(unsigned int ip, char *res)
{
  sprintf(res, "%d.%d.%d.%d", (ip>>24)&255, (ip>>16)&255, (ip>>8)&255, (ip>>0)&255);
  return res;
}

static int lua_dhcp_conf(lua_State * L)
{
  dhcp_conf_t c;
  char s[16];
  int i;

  if (dhcp_get_conf(&c))
    return 0;

  lua_settop(L, 0);
  lua_pushnumber(L, c.valid);
  lua_pushstring(L, ip2str(c.yourip, s));
  lua_pushnumber(L, c.lease_time);
  lua_pushstring(L, ip2str(c.subnet_mask, s));
  lua_pushstring(L, ip2str(c.router, s));
  lua_pushstring(L, c.domain);
  for (i = 0; c.dns[i]; i++)
    lua_pushstring(L, ip2str(c.dns[i], s));
  return 6 + i;
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
    SHELL_COMMAND_C, lua_dhcp_init
  },

  {
    /* long names, short names and topic */
    DRIVER_NAME"_driver_shutdown", 0, 0,
    /* usage */
    DRIVER_NAME"_driver_shutdown() : "
    "INTERNAL ; shutdown lua side of " DRIVER_NAME " driver.",
    /* function */
    SHELL_COMMAND_C, lua_dhcp_shutdown
  },
 
  {
    /* long names, short names and topic */
    "dhcp_discover", 0, 0,
    /* usage */
    "dhcp_discover() : "
    "broadcast a DHCP discover query",
    /* function */
    SHELL_COMMAND_C, lua_dhcp_discover
  },
 
  {
    /* long names, short names and topic */
    "dhcp_request", 0, 0,
    /* usage */
    "dhcp_request() : "
    "broadcast a DHCP request",
    /* function */
    SHELL_COMMAND_C, lua_dhcp_request
  },
 
  {
    /* long names, short names and topic */
    "dhcp_conf", 0, 0,
    /* usage */
    "dhcp_conf() : "
    "return current DHCP conf, returns: yourip, server_identifier, lease_time, subnet_mask, router, domain, dns[, dns ...]",
    /* function */
    SHELL_COMMAND_C, lua_dhcp_conf
  },
 
  {0},                                    /* end of the command list */
};

any_driver_t dhcp_driver =
  {

    0,                     /**< Next driver                     */
    EXE_DRIVER,            /**< Driver type                     */      
    0x0100,                /**< Driver version                  */
    DRIVER_NAME,           /**< Driver name                     */
    "Vincent Penne",       /**< Driver authors                  */
    "DHCP support."        /**< Description                     */
    ,
    0,                     /**< DLL handler                     */
    driver_init,           /**< Driver init                     */
    driver_shutdown,       /**< Driver shutdown                 */
    driver_options,        /**< Driver options                  */
    driver_commands,       /**< Lua shell commands              */
  
  };

EXPORT_DRIVER(dhcp_driver)
