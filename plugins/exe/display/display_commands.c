/**
 * @ingroup  exe_plugin
 * @file     display_commands.c
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/11/28
 * @brief    graphics lua extension plugin, display list built-in functions
 * 
 * $Id: display_commands.c,v 1.1 2002-12-04 11:28:24 ben Exp $
 */

#include <string.h>
#include "display_driver.h"
#include "draw/gc.h"

DL_FUNCTION_START(nop)
{
  dl_nop_command(dl);
  return 0;
}
DL_FUNCTION_END();

static int strtoop(const char *s)
{
  int op = DL_INHERIT_MUL;
  if (s) {
	if (!stricmp(s,"parent")) {
	  op = DL_INHERIT_PARENT;
	} else if (!stricmp(s,"local")) {
	  op = DL_INHERIT_LOCAL;
	} else if (!stricmp(s,"add")) {
	  op = DL_INHERIT_ADD;
	} else if (!stricmp(s,"modulate") || !stricmp(s,"mul")) {
	  op = DL_INHERIT_MUL;
	}
  }
  return op;
}

DL_FUNCTION_START(sublist)
{
  dl_list_t * subl;
  dl_runcontext_t rc;
  const char * s;
  int c, gcflags, colorop, transop;

  if (lua_tag(L, 2) != dl_list_tag) {
	printf("dl_sublist : 2nd parameter is not a list\n");
	return 0;
  }
  subl = lua_touserdata(L, 2);
  
  gcflags = GC_RESTORE_ALL;
  colorop = strtoop(lua_tostring(L, 3));
  transop = strtoop(lua_tostring(L, 4));
  s = lua_tostring(L, 5);
  if (s)  while (c = *s++, c) {
	switch (c) {

	case '~': gcflags = ~gcflags; break;
	case 'A': gcflags = GC_RESTORE_ALL; break;
	case 'a': gcflags = 0; break;

	case 'T': gcflags |=  GC_RESTORE_TEXT; break;
	case 't': gcflags &= ~GC_RESTORE_TEXT; break;
	case 'F': gcflags |=  GC_RESTORE_TEXT_FONT; break;
	case 'f': gcflags &= ~GC_RESTORE_TEXT_FONT; break;
	case 'S': gcflags |=  GC_RESTORE_TEXT_SIZE; break;
	case 's': gcflags &= ~GC_RESTORE_TEXT_SIZE; break;
	case 'K': gcflags |=  GC_RESTORE_TEXT_COLOR; break;
	case 'k': gcflags &= ~GC_RESTORE_TEXT_COLOR; break;

	case 'C': gcflags |=  GC_RESTORE_COLORS; break;
	case 'c': gcflags &= ~GC_RESTORE_COLORS; break;
	case '0': case '1': case '2': case '3':
	  gcflags ^= GC_RESTORE_COLORS_0 << (c-'0');
	  break;

	case 'B': gcflags |=  GC_RESTORE_CLIPPING; break;
	case 'b': gcflags &= ~GC_RESTORE_CLIPPING; break;

	} 
  }
  rc.color_inherit = colorop;
  rc.trans_inherit = transop;
  rc.gc_flags = gcflags;

  if (dl_sublist_command(dl, subl, &rc) < 0) {
	printf("dl_sublist : failure (may be not a valid sub-list).\n");
  }
  return 0;
}
DL_FUNCTION_END()

