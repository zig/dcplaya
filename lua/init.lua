--- @ingroup  dcplaya_lua_basics
--- @file     init.lua
--- @author   vincent penne
--- @author   benjamin gerard
--- @brief    Fundamental lua stuff.
---
--- $Id: init.lua,v 1.26 2004-07-31 22:55:18 vincentp Exp $
---

--- @defgroup  dcplaya_lua_basics_library  LUA libraries
--- @ingroup   dcplaya_lua_basics
--- @brief     LUA libraries.
---
---   LUA libraries are LUA scripts or chunks (lua compiled files) executed
---   with the dofile() function.
---   To complete successfully a library must return 1.
---   Loading a lua library is performed by the dolib() function.
---   This function  handles proper library loading. It consists on :
---     - checking already loaded library
---     - avoiding infinite loop this circular reference
---     - searching library file in library pathes.
---
--- @author   benjamin gerard
--

--- Default lua library pathes.
--- @ingroup   dcplaya_lua_basics_library
--: string LIBRARY_PATH[];

--- Global loaded library table.
--- @ingroup   dcplaya_lua_basics_library
--: string loaded_libraries[];

-- Set default LIBRARY_PATH
if not LIBRARY_PATH then
   LIBRARY_PATH = { "/ram/dcplaya/lua", home .. "lua", "/rd" }
end

if type(doshellcommand) ~= "function" then

--- Simple doshellcommand (reimplemented in shell.lua).
--- @ingroup dcplaya_lua_basics
--- @param string lua command
--- @code
function doshellcommand(string)
   return dostring(string)
end
--- @endcode
--: doshellcommand(string);

end

--- @defgroup  dcplaya_lua_basics_help  Help system
--- @ingroup   dcplaya_lua_basics
--- @brief     Dynamic help for shell commands
---
---  The help system is hierarchic system. On the top of which stands
---  topics. Each topics holds commands. Each command has at least a name
---  and a usage (help) message. Commands may have a optional short name.
---  Their is a special topic for the drivers named "drivers". Help on drivers
---  give the list of all loaded driver. Help on a driver name display
---  information on that driver as well as the commands associate to it.
---  Driver commands are stored by default in a topic of the name of the
---  driver, but driver may choose other topic for each command it provides.
---
---  It is possible to have a the same name for different command usage in
---  different topic. To access a command in a given topic help() command
---  allow to specify a full qualified command name `topic/command`.
---  The "*" star may be use as a wild-card on either topic or command.
---  A single "*" or a "* / *" will display topics list and the listing of
---  command for each topic. 
---  The wild-card only works for regular topics not for "drivers".
---
--- @author    benjamin gerard
--- @{
---

--- Add help about a shell command.
--- @param  name        name of command to add to the help (mandatory)
--- @param  short_name  optional short name for the command
--- @param  topics      help topics (defaulted to "general")
--- @param  help        help string (mandatory). 
--- @return error-code
--- @retval nil on error
--
function addhelp(name, short_name, topic, help)
   if type(help)  ~= "string" then return end
   if type(name)=="function" then -- $$$ ben : little bit hacky, isn't it ?
      fname=getinfo(name).name
   end
   if type(name) ~= "string" then return end
   if type(topic) ~= "string" then topic = "general" end
   if type(help_topics[topic]) ~= "table" then
      if __DEBUG then
	 print("Creating new topic : "..topic)
      end
      help_topics[topic] = {}
   end
   if __DEBUG then
      print("Add "..name.." to "..topic)
   end
   help_topics[topic][name] = {
      short_name = short_name,
      help = help
   }
   if short_name then 
      help_topics[topic][short_name] = name
      local n,s = getglobal(name), getglobal(short_name)
      if type(n) == "function" and type(s) == "nil" then
	 if __DEBUG then
	    print("Create " .. short_name .. " alias for " .. name)
	 end
	 setglobal(short_name,n)
      end
   end
   return 1
end

--- Remove help.
---
---   The remove_help() function removes help message for given commands.
---   Either long or short name or both can be removed. If topic is nil
---   "general" topic is used. If no more command remains in topic, the
---   topic is removed.
---
--- @param  name        long name of command to remove
--- @param  short_name  short name  of command to remove
--- @param  topic       topic in which is command has been add
---                     (defaut "general")
--- @return error-code
--- @retval nil on error
function remove_help(name, short_name, topic)
   if type(name)=="function" then -- $$$ ben : little bit hacky, isn't it ?
      fname=getinfo(name).name
   end
   if type(topic) ~= "string" then topic = "general" end

   if type(help_topics[topic]) ~= "table" then return end
   local r
   if type(name) == "string" then
      if __DEBUG then
	 print("Remove "..name.." from "..topic)
      end
      r = r or help_topics[topic][name] ~= nil
      help_topics[topic][name] = nil
   end
   if type(short_name) == "string" then 
      if __DEBUG then
	 print("Remove "..short_name.." from "..topic)
      end
      r = r or help_topics[topic][short_name] ~= nil
      help_topics[topic][short_name] = nil
   end
   local used
   local i,v
   for i,v in help_topics[topic] do
      used = 1
      break
   end
   if not used then
      if __DEBUG then
	 print("Remove topics "..topic)
      end
      help_topics[topic] = nil
   end
   return 1
end

--- Display a list formatted list of commands
--- @param  tbl         table
--- @param  max_width   length of the longest name in the table
--- @param  wait_key    wait key stroke if listing too long.
--- @see ls_column()
--- @internal
function help_list(tbl, max_width, wait_key)
   -- Simple replacement function in case ls_column is not defined.
   local foreach = function (t, dummy) local i,v
		      for i,v in t do
			 print(v)
		      end
		   end
   if type(ls_column) == "function" then
      ls_column(tbl,max_width+1, not wait_key)
   else
      foreach (tbl, print)
   end
end

--- Build a sorted table of name (shortname) entry
--- @param  tbl  table to sort
--- @return a sorted table
--- @internal
function help_sort_key(tbl)
   if type(tbl) ~= "table" then return end
   local sorted = {}
   local i,v
   for i,v in tbl do
      local name = nil
      if tag(v) == driver_tag then
	 name = i
      elseif type(v) == "table" then -- Skip short name entry
	 name = i
	 if v.short_name then
	    name = name .. "(" .. v.short_name .. ")"
	 end
      end
      if name then tinsert(sorted, name) end
   end
   sort(sorted)
   return sorted
end

--- Process listing of a table
---
---   The help_list_key_table() function sort the given table with
---   the help_sort_key() function, finds the longest name and 
---   display the list with the help_list() function.
--- @param  tbl       topic ot driver table 
--- @param  label     optional message to display before the listing
--- @param  wait_key  wait for a key stroke if listing too long
--- @return error-code
--- @retval nil on error
--- @see help_sort_key()
--- @see help_list()
--- @internal
function help_list_key_table(tbl, label, wait_key)
   local sorted = help_sort_key(tbl)
   if not sorted then return end
   local n,maxi,i = getn(sorted),0
   for i=1,n do
      maxi = max(maxi,strlen(sorted[i]))
   end
   if n > 0 then
      if label then
	 print(label)
      end
      help_list(sorted, maxi, wait_key)
   end
   return 1
end

--- Display a list of all topics.
--- @param wait_key  wait for a key stroke if listing too long
--- @return error-code
--- @retval nil on error
function help_on_topics(wait_key)
   return help_list_key_table(help_topics, "topics:", wait_key)
end

--- Display a list of all drivers.
--- @param wait_key  wait for a key stroke if listing too long
--- @return error-code
--- @retval nil on error
function help_on_drivers(wait_key)
   return help_list_key_table(driver_list, "drivers:", wait_key)
end

--- Display a list of all commands in given topics or the list of drivers.
--- @param  topic    topic name or special "drivers" topic
--- @param  wait_key  wait for a key stroke if listing too long
--- @return error-code
--- @retval nil on error
function help_on_topic(topic, wait_key, no_topics)
   if type(help_topics) ~= "table" then return end
   if type(topic) ~= "string" then
      help_on_topics(wait_key)
      return
   elseif not help_topics[topic] then
      if not no_topics then
	 print(topic .. " : no such topic")
	 help_on_topics(wait_key)
      end
      return
   end
   if topic == "drivers" then
      return help_on_drivers(wait_key)
   end
   help_list_key_table(help_topics[topic], "topic "..topic..":", wait_key)
   return 1
end

--- Display help a given driver.
--- @param  driver_name  name of driver
--- @param  wait_key  wait for a key stroke if listing too long
--- @param  do not fall to display driver list if the driver is not found.
--- @return error code
--- @retval nil on error (not found)
function help_on_driver(driver_name, wait_key, no_drivers)
   if type(driver_list) ~= "table" then return end
   local driver = driver_list[driver_name]
   if not driver then
      if not no_drivers then
	 print(driver_name .. " : no such driver")
	 help_on_drivers(wait_key)
      end
      return
   end
   print("driver " .. driver_name .. ":")
   print ([[name:        ]] .. driver.name)
   print ([[description: ]] .. driver.description)
   print ([[authors:     ]] .. driver.authors)
   print ([[version:     ]]
	  .. format("%d.%02d", driver.version/256, mod(driver.version,256)))
   print ([[type:        ]] .. format("%s",
				      strchar(mod(driver.type,256),
					      mod(driver.type/256,256),
					      mod(driver.type/65536,256))))
   local h = driver.luacommands
   if h then
      local tbl,n,i,c,l = {}, 1
      for i,c in h do
	 local name, short_name = c.name, c.short_name 
	 if not name then name, short_name = short_name, name end
	 if name then
	    tbl[name] = { short_name = short_name }
	 end
      end
      print()
      help_list_key_table(tbl, driver_name .. " commands:", wait_key)
   end
   return 1
end

--- Display usage for a command in a given topic.
--- @param  command  command name (either short or long name)
--- @param  topic    topic the command belong to or "*" for all
--- @return number of matching comands 
--- @retval nil on error (not found)
function help_command_usage(command, topic, only_one)
   if type(command) ~= "string" or
      type(topic) ~= "string" or
      type(help_topics) ~= "table" then return end
   
   local topics = help_topics
   if topic ~= "*" then
      topics = { [topic] = help_topics[topic] }
   end
   local n,i,t = 0
   for topic,t in topics do
      if type(t) == "table" then
	 local c = t[command]
	 if type(c) == "string" then command,c = c,t[c] end
	 if type(c) == "table" then
	    local help = topic .. "/" .. command
	    if c.short_name then
	       help = help .. "(" .. c.short_name .. ")"
	    end
	    print(help)
	    print(c.help)
	    if only_one then return 1 end
	 else n = n + 1 end
      end
   end
   return n > 0 and n
end

--- Display help on a command.
---
---   This function is a alias for
--- @code
---  help_command_usage(command_name, "*", 1)
--- @endcode
---
--- @param  command_name  command name (either short or long name)
--- @return error code
--- @return on error (not found)
--- @see help_command_usage()
function help_on_command(command_name) --, wait_key, no_commands)
   return help_command_usage(command_name, "*", 1)
--  then
--     if type(command_name) ~= "string" or 
--       type(help_topics) ~= "table" then return end
--    local topic,v
--    for topic,v in help_topics do
--       if help_command_usage(command_name, topic) then
-- 	 return 1
--       end
--    end
end

--- Help about a shell command.
--- @param  name      command name
--- @param  wait_key  wait for on keystroke if listing is too long
---
---  Help hierarchy:
---   - name is nil or "topics",  display the list of topics
---   - name is "drivers", display the list of drivers
---   - name is "*" display topics and topics command list (each of them)
---   - name is "topic/command", display help on this fully qualified command
---              topic and command an be "*".
---   - name is a topic, display the list of commands in this topic
---   - name is a driver, display driver info and its commands
---   - name is a command, display usage of the first matching command on
---     on all topics.
---
function help(name, wait_key)
   local h,n,i,j,c,k,v,t,sortkey,mx
   if type (name) ~= "string" or name == "topics" then
      return help_on_topics(wait_key)
   end
   if type(help_topics) ~= "table" then return end
   if name == "drivers" then
      return help_on_drivers(wait_key)
   end

   local a,b,topic,command = strfind(name,"^(.*)/(.*)$")
   if a then
      if topic == "" then topic = "*" end
      if command == "" then command = "*" end
      name = topic .. "/" .. command
   end

   if name == "*" or name == "*/*" then
      help_on_topics(wait_key)
      print()
      local i,v
      for i,v in help_topics do
	 help_on_topic(i,wait_key,1)
	 print()
      end
      help_on_drivers(wait_key)
      return 1
   end

   if a then
      if command == "*" then
	 return help_on_topic(topic,wait_key, 1) or
	    help_on_driver(topic, wait_key, 1)
      else
	 return help_command_usage(command, topic)
      end
   end

   return help_on_topic(name, wait_key, 1)
      or help_on_driver(name, wait_key, 1)
      or help_on_command(name, wait_key, 1)
end

--- Initialize the help system.
--- @param  force  force reinitialization if already initialized (discard
---                all registrered commands, topics and drivers.
function help_init(force)
   if force or type(help_topics) ~= "table" then
      help_topics = {}
   end
end

--- Shutdown the help system.
--- @code
function help_shutdown()
   help_init(force)
end
--- @endcode

help_init()

--- Alias for help().
--- @see help()
--: usage(what, wait_key);
usage=help
setglobal("?",help)

addhelp("addhelp",nil,"helps",
[[addhelp(nil|*|"topics"|drivers|driver-name|command-name,short-name|nil,topics|nil,message)
      Add usage information about a command. short-name is optional. If it is given but undefined and command-name is a function addhelp will create it. topics is defaulted to "general". If a command is defined in multiple topics, you can qualify it by a slash /. e.g help("helps/addhelp") will display this text even if another addhelp commands have been added to another topic. If you don\'t the result is undefined.]])

addhelp("help","?","helps",
[[help("topics"|topic-name|"drivers"|driver-name|command-name [, wait_key])
      Show help on various subject. wait_key waits key stroke if display is too long.]])

addhelp("syntax",nil,"helps",
	[[Syntax of command usage := command-name(a1|b1 [,b2 [,b3 [,...] ] ])
the `|` means OR. the [ ] mean optional. Commands are displayed in fully qualified form (topic/command). short name are prefixed without topic qualifier between parenthesis]])

--
--- @}
---

--- @defgroup  dcplaya_lua_basics_driver Driver Support
--- @ingroup   dcplaya_lua_basics
--- @brief     driver support
---
--- @author   vincent penne
---

--- Register driver commands.
--- @ingroup dcplaya_lua_basics_driver
--- @internal
--
function register_commands(driver, force)
   if tag(driver) ~= driver_tag then
      print("[register_commands] : not a driver")
      return
   end
   local commands = driver.luacommands
   if not commands then return end

   local topic = driver.name
   local i, c
   for i, c in commands do
      if force or c.registered == 0 then
	 if __DEBUG then
	    print ("Register new command ", c.name)
	 end
	 setglobal(c.name, c["function"])
	 if c.short_name then setglobal(c.short_name, c["function"]) end
	 topic = c.topic or topic
	 addhelp(c.name, c.short_name, topic, c.usage)
	 c.registered = 1
      end
   end
end

function unregister_commands(driver, force)
   if tag(driver) ~= driver_tag then
      print("[unregister_commands] : not a driver")
      return
   end
   local commands = driver.luacommands
   if not commands then return end

   local topic = driver.name
   local i, c
   for i, c in commands do
      if force or c.registered ~= 0 then
	 if __DEBUG then
	    print ("Unregister command ", c.name)
	 end
	 setglobal(c.name, nil)
	 if c.short_name then setglobal(c.short_name, nil) end
	 topic = c.topic or topic
	 remove_help(c.name, c.short_name, topic)
	 c.registered = 0
      end
   end

   -- hopefully this may cause the driver to be released at once
   collect(300000)
end

--- Update the driver list.
--- @ingroup dcplaya_lua_basics_driver
--- @internal
--
function update_driver_list(list, force)
   local i
   local d

   local n=list.n
   for i=1, n, 1 do

      d = list[i]
      local old_d = driver_list[d.name]
      local new=force
      if not old_d then
	 new = 1
      end
      if old_d and not driver_is_same(old_d, d) then
	 if __DEBUG then
	    print("Warning : replacing driver '", d.name, "' in list")
	 end
	 new = 1
      end

      driver_list[d.name] = d
      if new then
	 -- if a shutdown function exists, then call it

	 if old_d then
	    local shut
	    shut = getglobal(old_d.name.."_driver_shutdown")
	    unregister_commands(old_d, force)
	    if shut then
	       shut()
	    end
	    old_d = nil
	 end

	 -- register commands
	 register_commands(d, force)
	 
	 -- if an init function exists, then call it
	 local init
	 init = getglobal(d.name.."_driver_init")
	 if init then
	    init()
	 end
      end
   end
end

--- Update all driver lists.
--- @ingroup dcplaya_lua_basics_driver
--- @internal
--
function update_all_driver_lists(force)
   if type(driver_list)~="table" then
      driver_list = {}
   end

   -- get all types of driver lists
   local lists = get_driver_lists()
   local name, list
   for name,list in lists do
      if name ~= "obj" then -- (skip obj plugins, not relevant here ...)
	 update_driver_list(list, force)
      end
   end
end


--- this function is called when the shell is shutting down.
--- @ingroup dcplaya_lua_basics_driver
--
function dynshell_shutdown()
   if type(driver_list)=="table" then
      local i
      for i, _ in driver_list do
	 local f = getglobal(i.."_driver_shutdown")
	 if f then
	    f()
	 end
      end
   end
end

-- do this file only once !
if not init_lua then
   init_lua=1

   update_all_driver_lists(1)

--- Reimplement driver_load() so that it calls update_all_driver_lists()
--- automatically after.
--- @ingroup dcplaya_lua_basics_driver
--
   function driver_load(...)
      local r = call(%driver_load, arg)
      update_all_driver_lists()
      return r
   end
   dl=driver_load

end -- if not init_lua then

--
--- Load a lua library.
---
--- @ingroup dcplaya_lua_basics_library
---
---   For each path stored in the libpath table, the loadlib function
---   calls a dofile({PATH}/{NAME}.lua). It stops if the library loads
---   properly and returns a non nil value.
---   On success, the loaded_libraries[{NAME}] is set to 1 and the full-path
---   is returned.
---   On failure, the loaded_libraries[{NAME}] is unset and the function
---   returns nil.
---   Before starting the search, loaded_libraries[{NAME}] is set to 2. This
---   is special value used by dolib() function to avoid circular reference.
---
--- @param  name     lua library nude name (no path, no extension).
--- @param  libpath  table containing librairy search path.
---
--- @return  library  full pathname.
--- @retval  nil      on error.
---
--- @internal
--- @see dolib()
--
function loadlib(name, libpath)
   local i,p
   loaded_libraries[name] = 2
   for i,p in libpath do
      if type(p) == "string" then
	 p = canonical_path(p .. "/" .. name .. ".lua")
--	 print(format("searching in %q", tostring(p)))
	 if type(p) == "string" and type(dofile(p)) == "number" then
	    loaded_libraries[name] = 1
	    return p
	 end
      end
   end
   loaded_libraries[name] = nil
end

--
--- Load a lua library.
---
--- @ingroup dcplaya_lua_basics_library
---  
---  @param  name     nude filename of lua library to load.
---  @param  force    library is reload even if library is loading or loaded.
---  @param  libpath  path or pathes table. nil defaulted to LIBRARY_PATH.
---
---  @return boolean
---  @retval nil on error
---
--- @see loadlib()
--
function dolib(name,force,libpath)
   -- Create global loaded library table if not exists.
   if not loaded_libraries then
      loaded_libraries = {}
   end

   if not libpath then
      libpath=LIBRARY_PATH
   end
   if type(libpath) == "string" then
      libpath = { libpath }
   end
   if type(name) ~= "string" or type(libpath) ~= "table" then
      print("dolib : bad arguments")
      return
   end
   
   if loaded_libraries[name] and not force then
      if __DEBUG then
	 print(format("Library %q " ..
		      ((loaded_libraries[name] == 1 and "already loaded") or
		       "currently loading"),name))
      end
      return 1
   end
   if __DEBUG then
      print(format("Loading library %q",name))
   end
   local path = loadlib(name, libpath)
   if not path then
      print(format("Load library %q failed", name))
      return
   end
   if __DEBUG then
      print(format("Library %q loaded from %q",name,path))
   end
   return 1
end

return 1