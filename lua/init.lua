--- @ingroup  dcplaya_lua_basics
--- @file     init.lua
--- @author   vincent penne
--- @author   benjamin gerard
--- @brief    Fundamental lua stuff.
---
--- $Id: init.lua,v 1.23 2003-03-26 23:02:49 ben Exp $
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

function doshellcommand(string)
   return dostring(string)
end

end

--- @name Help functions.
--- @ingroup dcplaya_lua_basics
--- @{
--
if type(shell_help_array) ~= "table" then
   shell_help_array = {}
end
if type(shell_general_commands) ~= "table" then
   shell_general_commands = {}
end

--- Add help about a shell command.
function addhelp(fname, help_func, loc)
   if type(fname)=="function" then
      fname=getinfo(fname).name
   end
   if fname then
      shell_help_array[fname] = help_func
      if not loc then
	 shell_general_commands[fname] = help_func
      end
   else
      print ("Warning : calling addhelp on a nil fname")
   end
end

--- Help about a shell command.
function help(fname)
   local h,n,i,j,c,k,v,t,sortkey,mx

   if fname then
      h = shell_help_array[fname]
   end
   if h then
      -- Help on a command
      dostring (h)
      return
   end

   -- $$$ Reimplement froeach here for print only and without index !
   local foreach = function (t, dummy)
		      local i,v
		      for i,v in t do
			 print(v)
		      end
		   end
   
   local driver
   if fname then
      driver = driver_list[fname]
   end
   if driver then
      -- Help on a driver --
      print ([[driver:      ]] .. driver.name)
      print ([[description: ]] .. driver.description)
      print ([[authors:     ]] .. driver.authors)
      print ([[version:     ]] .. format("%d.%02d",
				  driver.version/256, mod(driver.version,256)))
      print ([[type:        ]] .. format("%s",
			       strchar(mod(driver.type,256),
				       mod(driver.type/256,256),
				       mod(driver.type/65536,256))))
      h = driver.luacommands
      if h then
	 local n = 1
	 local i, c

	 n = 1
	 sortkey = {}
	 for i, c in h do
	    sortkey[n] = c.name .. "##" .. i
	    n = n + 1
	 end
	 sort(sortkey)

	 n = 1
	 t = { }
	 mx = 0
	 for i, c in sortkey do
	    local a,b,j = strfind(c,".*##(%d+)")
	    if j then
	       local k = h[tonumber(j)]
	       t[n] = k.name
		  .. ((k.short_name and (" ("..k.short_name..")")) or "")
	       mx = max(mx, strlen(t[n]))
	       n = n+1
	    end
	 end

	 print [[commands are:]]
	 if type(ls_column) == "function" then
	    ls_column(t,mx+1,1)
	 else
	    foreach (t, print)
	 end
      end
      return
   end

   sortkey = {}
   n = 1
   mx = 0
   for i, v in shell_general_commands do
      sortkey[n] = i
      n = n + 1
      mx = max(mx,strlen(i))
   end
   sort(sortkey)

   t = { }
   n = 1
   for i, v in sortkey do
      t[n] = v
      n = n + 1
   end

   print [[general commands are:]]
   if type(ls_column) == "function" then
      ls_column(t,mx+1,1)
   else
      foreach (t, print)
   end
   
   n = 1
   sortkey = {}
   mx = 0
   for i, c in driver_list do
      sortkey[n] = i
      n = n + 1
      mx = max(mx,strlen(i))
   end
   sort(sortkey)

   t = { }
   n = 1
   for i, v in sortkey do
      t[n] = v
      n = n+1
   end

   print [[drivers are:]]
   if type(ls_column) == "function" then
      ls_column(t,mx+1,1)
   else
      foreach (t, print)
   end
end

--- Alias for help()
--: usage(what)
usage=help

addhelp(help,
[[print [[help(command_name|driver_name) : show information about a command]]]])
addhelp(addhelp,
[[print [[addhelp(command_name, string_to_execute) : add usage information about a command]]]])

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
function register_commands(dd, commands, force)
   if not commands then
      return
   end

   local i, c
   for i, c in commands do
      if force or c.registered == 0 then
	 print ("Registering new command ", c.name)
	 setglobal(c.name, c["function"])
	 addhelp(c.name, c.usage, 1)
	 if c.short_name then
	    print ("Short name ", c.short_name)
	    setglobal(c.short_name, c["function"])
	    addhelp(c.short_name, c.usage, 1)
	 end
	 c.registered = 1
      end
   end
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
	 print("Warning : replacing driver '", d.name, "' in list")
	 new = 1
      end
      --	 print (d.name, force, new)
      driver_list[d.name] = d
      --		print (d.name)

      if new then
	 -- if a shutdown function exists, then call it
	 local shut
	 shut = getglobal(d.name.."_driver_shutdown")
	 if shut then
	    shut()
	 end
	 
	 -- register commands
	 local commands = d.luacommands
	 register_commands(driver_list[d.name], commands, force)
	 
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
---   calls a dolfile({PATH}/{NAME}.lua). It stops if the library loads
---   properly and returns a non nil value.
---   On success, the loaded_libraries[{NAME}] is set to 1 and the fullpath
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
      print(format("Library %q " ..
		   ((loaded_libraries[name] == 1 and "already loaded") or
		    "currently loading"),name))
      return 1
   end
   print(format("Loading library %q",name))
   local path = loadlib(name, libpath)
   if not path then
      print(format("Load library %q failed", name))
      return
   end
   print(format("Library %q loaded from %q",name,path))
   return 1
end

