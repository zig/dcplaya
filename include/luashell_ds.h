
#ifndef _LUASHELL_H_
# error "Need to include luashell.h before"
#endif

DS_STRUCTURE_START(luashell_command_description_t)
     /* beginning of static variables */
     DS_STRING(luashell_command_description_t, name)
     DS_STRING(luashell_command_description_t, short_name)
     DS_STRING(luashell_command_description_t, topic)
     DS_STRING(luashell_command_description_t, usage)
     DS_INT(luashell_command_description_t, type)
     DS_LUACFUNCTION(luashell_command_description_t, function)

     /* beginning of dynamic variables */
     DS_INT(luashell_command_description_t, registered)
DS_STRUCTURE_END()
