#if 0
EL_FUNCTION_DECLARE(new) {
{
  el_list_t * l;

  if (!el_tag) {
	lua_init_el_type(L);
    if (el_tag < 0) {
      printf("entrylist driver not initialized !");
      return 0;
    }
  }
  printf("Creating new entrylist\n");
  lua_settop(L, 0);
  l = el_create();
  if (l) {
    lua_pushusertag(L, l, el_tag);
    return 1;
  }
  return 0;
}

DL_FUNCTION_START(destroy)
{
  printf("destroying list %p\n", el);
  lua_settop(L, 0);
  el_destroy_list(dl);
  return 0;
}
DL_FUNCTION_END()

#endif
