/**
 * @file      ds.h
 * @author    vincent penne
 * @date      2002/09/24
 * @brief     dynamically accessible structure support
 * @version   $Id: ds.h,v 1.2 2003-03-26 23:02:47 ben Exp $
 */

#ifndef _DS_H_
#define _DS_H_


#include <stddef.h>
#ifndef offsetof
# define offsetof(s,m)   (size_t)&(((s *)0)->m)
#endif

enum ds_type_t {
  DS_TYPE_INT,
  DS_TYPE_STRING,
  DS_TYPE_LUACFUNCTION
};

#define DS_STRUCTURE_START(s) ds_structure_t s##_DSD = { #s, {
#define DS_STRUCTURE_END()    {0} } };

#define DS_ENTRY(s, type, entry)  { #entry, type, offsetof(s, entry) },
#define DS_STRING(s, entry) DS_ENTRY(s, DS_TYPE_STRING, entry)
#define DS_INT(s, entry) DS_ENTRY(s, DS_TYPE_INT, entry)
#define DS_LUACFUNCTION(s, entry) DS_ENTRY(s, DS_TYPE_LUACFUNCTION, entry)

/* entry description */
typedef struct ds_entry {
  const char * name;
  int          type;
  size_t       offset;
} ds_entry_t;

#define DS_MAX_ENTRIES 32
/* structure description */
typedef struct ds_structure {
  const char * name;
  ds_entry_t   entries[DS_MAX_ENTRIES+1];
} ds_structure_t;

#define ds_find_entry(sd, name) ds_find_entry_func(sd##_DSD, name)
ds_entry_t * ds_find_entry_func(ds_structure_t * sd, const char * name);

#define ds_find(data, sd, name) ds_find_func(sd##_DSD, name)
void * ds_find_func(void * data, ds_structure_t * sd, const char * name);

#define ds_data(data, entry) ( (void *) ( ((char *) (data)) + (entry)->offset) )

#endif // #ifdef _DS_H_
