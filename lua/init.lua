--- @ingroup  lua_devel
--- @file     init.lua
--- @author   vincent penne <ziggy@sashipa.com>
--- @author   benjamin gerard <ben@sashipa.com>
--- @brief    Fundamental lua stuff.
---
--- $Id: init.lua,v 1.18 2003-03-11 15:07:58 zigziggy Exp $
---

-- do this file only once !
if not init_lua then
   init_lua=1

   -- Set default LIBRARY_PATH
   if not LIBRARY_PATH then
      LIBRARY_PATH = { home, "/ram/", "/rd/" }
   end

   --- Simple doshellcommand (reimplemented in shell.lua).
   function doshellcommand(string)
      return dostring(string)
   end

   --- @name Help functions.
   --- @{
   --
   shell_help_array = {}
   shell_general_commands = {}

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

   --- Display help on a command.
   function help(fname)
      local h

      if fname then
	 h = shell_help_array[fname]
      end
      if h then
	 dostring (h)
	 return
      end

      local driver
      if fname then
	 driver = driver_list[fname]
      end
      if driver then
	 print ([[driver:]], driver.name)
	 print ([[description:]], driver.description)
	 print ([[authors:]], driver.authors)
	 print ([[version:]], format("%x", driver.version))
	 print ([[type:]], format("%d", driver.type))
	 h = driver.luacommands
	 if h then
	    print [[commands are:]]
	    local t = { }
	    local n = 1
	    local i, c
	    for i, c in h do
	       t[n] = c.name .. [[, ]]
	       if c.shortname then
		  t[n] = c.shortname .. [[, ]]
	       end
	       n = n+1
	    end
	    call (print, t)
	 end
	 return
      end

      print [[general commands are:]]
      local t = { }
      local n = 1
      local i, v
      for i, v in shell_general_commands do
	 local name=i
	 t[n] = name .. [[, ]]
	 n = n+1
      end
      call (print, t)
      print [[drivers are:]]
      t = { } 
      n = 1
      for i, v in driver_list do
	 t[n] = i .. ", "
	 n = n+1
      end
      call (print, t)
   end


   usage=help

   addhelp(help, [[print [[help(command_name|driver_name) : show information about a command]]]])
   addhelp(addhelp, [[print [[addhelp(command_name, string_to_execute) : add usage information about a command]]]])

   --- @}

   --- @name Drivers support.
   --- @{
   --

   --- Register driver commands.
   ---
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

   --- @}


   -- this function is called when the shell is shutting down
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

   update_all_driver_lists(1)

   -- reimplement driver_load so that it calls update_all_driver_lists()
   -- automatically after
   function driver_load(...)
      call(%driver_load, arg)
      update_all_driver_lists()
   end
   dl=driver_load

end -- if not init_lua then

--- Load a lua library. The file lua/{NAME}.lua will be loaded via dofile()
--- in pathes stored in LIBRARY_PATH.
--- Library must set the {NAME}_loaded variable when loaded successfully
--- If force parameter is set, the function ignore the {NAME}_loaded value
--- and try to reload the library anyway.
--- the libpath parameter allows to override the default LIBRARY_PATH
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

   function loadlib()
      local i,p
      loaded_libraries[%name] = 2
      for i,p in %libpath do
	 --			print(format("searching in %q", tostring(p)))
	 if type(p) == "string" then
	    p = canonical_path(p.."lua/"..%name..".lua")
	    if dofile(p) then
	       loaded_libraries[%name] = 1
	       return p
	    end
	 end
      end
      loaded_libraries[%name] = 1
   end
   
   if loaded_libraries[name] and not force then
      print(format("Library %q " ..
		   ((loaded_libraries[name] == 1 and "already loaded") or
		    "currently loading"),name))
      return 1
   end
   print(format("Loading library %q",name))
   local path = loadlib()
   if not path then
      print(format("Load library %q failed", name))
      return
   end
   print(format("Library %q loaded from %q",name,path))
   return 1
end

