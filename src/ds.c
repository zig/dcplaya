
#include <string.h>

#include "ds.h"

ds_entry_t * ds_find_entry_func(ds_structure_t * sd, const char * name)
{
  int i;

  for (i=0; sd->entries[i].name; i++) {
    if (!strcmp(name, sd->entries[i].name))
      return sd->entries+i;
  }

  return NULL;
}

void * ds_find_func(void * data, ds_structure_t * sd, const char * name)
{
  ds_entry_t * entry;

  entry = ds_find_entry_func(sd, name);

  if (entry)
    return (void *) ( ((char *) data) + entry->offset);

  return NULL;
}
