#ifndef _ANY_DRIVER_H_
# error "Need to include any_driver.h"
#endif

/* Dynamic structure declaration for any_driver_t */
DS_STRUCTURE_START(any_driver_t)
     DS_INT   (any_driver_t, type)
     DS_INT   (any_driver_t, version)
     DS_STRING(any_driver_t, name)
     DS_STRING(any_driver_t, authors)
     DS_STRING(any_driver_t, description)
DS_STRUCTURE_END()

