--- @ingroup  dcplaya_lua_enhanced_shell
--- @file     shell.lua
--- @author   vincent penne
--- @author   benjamin gerard
--- @brief    LUA enhanced shell
---
--- $Id: shell.lua,v 1.19 2004-06-30 15:17:36 vincentp Exp $
--

--- @defgroup  dcplaya_lua_enhanced_shell  LUA enhanced shell
--- @ingroup   dcplaya_lua_shell
--- @brief     LUA enhanced shell
---
--- @par Introduction
---
---   LUA enhanced shell improved basic shell. It adds common syntax to
---   shell commands, a smartest line edition with command historic recall.
---
--- @par Edition enhancement
---
---   LUA enhanced shell use @link dcplaya_lua_zed zed @endlink to edit
---   the command line. left / right / home / end keys (may be others ?)
---   could be use with great advantage here.
---
--- @par Syntax enhancement
---
---   LUA enhanced shell adds UNIX like shell command syntax. Since LUA shell
---   only understand LUA statement, this shell transforms UNIX like commands
---   into LUA statement. To do this the enhanced shell parse the command line
---   end tries to determine if it is a LUA syntax or not. If it is a LUA
---   syntax the shell will perform some supplemental checks like function
---   or object:method existence. For enhanced commands the same checks are
---   performed and other substition can be done depending on the
---   shell_substitut variable. By default this variable is unset so that
---   any substition is disable and all parameters will be sent as string.
---
--- @par Historic enhancement
---
---   LUA enhanced shell allow to recall a previous command and to edit
---   it in a new line buffer. The historic is stored in shell_recalltable
---   table and its maximum size is set in shell_toggleconsolekey.
---
--- @warning The enhanced shell is in charge to call the getchar() function
---   which responsible of @link dcplaya_lua_event event @endlink dispatching.
---   Sometime the shell is shutdowned by LUA error at shell level (typically
---   when a application handler or update function failed). In that case
---   the shell should fallback to basic shell, and enhanced shell should
---   be start again by typing `shell()`. If the error occurs in update
---   function you probably have to kill the application that fault this
---   the evt_shutdown_app() function. If the error appends in the handle
---   function, sometime the application could continue to work properly
---   depending on which event causes the fault.
---
---
--- @todo historic search.
--- @todo completion.
--- @todo handling CTRL for terminal control codes.
---
--- @author   vincent penne
--- @author   benjamin gerard
--

--- Keywords that could begin a lua sentence.
--- @ingroup dcplaya_lua_enhanced_shell
--: boolean shell_lua_key_words[keyword];
shell_lua_key_words = {
   ["function"] = 1,
   ["if"] = 1,
   ["while"] = 1,
   ["do"] = 1,
   ["for"] = 1,
   ["repeat"] = 1,
   ["return"] = 1,
   ["local"] = 1,
   ["break"] = 1,
}

--- Default table of variable type that the shell is allowed to substitut.
--- @ingroup dcplaya_lua_enhanced_shell
--- @deprecated Not used.
--: boolean shell_default_substitut[type_name];
-- shell_default_substitut = {
--    ["string"] = 1,
--    ["number"] = 1,
-- }

--- Current table for shell substitution.
--- @ingroup dcplaya_lua_enhanced_shell
---
---   If this variable is set, it must be a table indexed by type name
---   which are allowed to be substitued from the shell command line
---   in shell enhanced syntax mode.
---
---   shell_substitut is unset by default.
---
--: table shell_substitut[typename];
shell_substitut = nil

--- Enhanced doshellcommand() reimplementation.
--- @ingroup dcplaya_lua_enhanced_shell
---
--- Reimplementation of the doshellcommand function to extend the shell syntax.
--- Hopefully this new syntax should be backward compatible with lua normal
--- syntax. It adds possibility to type more usual command like "ls" without
--- brackets () or a `cd plugins` without quoting.
---
--- This only replace syntax at shell level, the lua syntax remains unchanged
--- so lua scripts should use normal syntax ...
---
function doshellcommand(string)
   list = {}
   force_string = {}
   local i,j
   j=0
   repeat
      i, j = strfind(string, [[[^ ]+]], j+1)
      if i then
	 local beg = strsub(string, i, i)
	 if beg == "\"" or (beg == "[" and strsub(string, i+1, i+1) == "[") then
	    -- this is a quoted argument (a string), reparse because we may have space into
	    local n = 1
	    local e = beg
	    if beg == "[" then
	       n = 2
	       e = "]"
	    end
	    --print (i, j, beg, e)
	    i, j = strfind(string, [[%b]]..beg..e, i)
	    if i then
	       tinsert(list, strsub(string, i+n, j-n))
	       force_string[getn(list)] = 1
	    else
	       print ("unbalanced quotes")
	    end
	 else
	    tinsert(list, strsub(string, i, j))
	 end
	 --print (getn(list), list[getn(list)])
      end
   until not i

   if list[1] then
      local sec
      if list[2] then sec=strsub(list[2], 1, 1) end

      if shell_lua_key_words[list[1]] then
	 -- lua keyword detected : do it as is
         -- plain LUA command
	 return dostring(string)
      end
      local st,sp,fct,obj,method

      if sec == '=' or strfind(list[1], "=") then
	 -- affectation : do it as is too !
	 return dostring(string)
      end

      -- Check for function call and get its name.
      st,sp,fct = strfind(list[1], "[%s]*([%w_:]+)[%s]*[(]")
      if not fct and sec == '(' then
	 fct = list[1]
      end

      if fct then
	 st,sp,obj,method=strfind(fct, "([%w_]+):([%w_]+)")
	 if st then
	    -- method call , check 
	    local o = getglobal(obj)
	    if not o then
	       print(format("Undefined object %q", obj))
	       return
	    elseif type(o) ~= "table" then
	       print(format("Not an object %q : %q", obj, type(o)))
	       return
	    elseif type(o[method]) ~= "function" then
	       print(format("Object %q as no %q method", obj, method))
	       return
	    else
	       -- function call : do it as is too !
	       return dostring(string)
	    end
	 end

	 -- function call , check if it exist and if it is a function.
	 local f = getglobal(fct)
	 if not f then
	    print(format("Undefined command %q", fct))
	    return
	 end
	 if type(f) ~= "function" then
	    print(format("Not a function %q : %q", fct, type(f)))
	    return
	 end
	 -- function call : do it as is too !
	 return dostring(string)
      end

      -- shellized command
      fct = list[1]
      local f = getglobal(fct)
      if not f then
	 print(format("Undefined command %q", fct))
	 return
      end
      if type(f) ~= "function" then
	 print(format("Not a function %q : %q", fct, type(f)))
	 return
      end

      if type(shell_substitut) == "table" then
	 -- shell has a substitution table, just perform them.
	 local i
	 for i=2, getn(list) do
	    -- 	 print(format("arg[%d] = %q %s",
	    -- 		      i,list[i],
	    -- 		      (force_string[i] and "force-string") or "variable"))
	    if not force_string[i] then
	       local val = getglobal(list[i])
	       if val and shell_substitut[type(val)] then
		  -- 	       print(format("substitut %q to %s type=%s",
		  -- 			    list[i], tostring(val), type(val)))
		  list[i] = val
	       end
	    end
	 end
      end
      tremove(list, 1)
      return call (f, list)
   end
end

function check_zed()
   if not zed_loaded and not dolib("zed") then
      print ("You need to install ZED before using the shell")
      return nil
   end
   return 1
end

--- Enhanced shell command historic table.
--- @ingroup dcplaya_lua_enhanced_shell
--: string shell_recalltable[];
shell_recalltable = {}

--- Enhanced shell command historic table maximum size (default:20).
--- @ingroup dcplaya_lua_enhanced_shell
--: number shell_maxrecall;
shell_maxrecall = 20

--- Enhanced shell toggle key.
--- @ingroup dcplaya_lua_enhanced_shell
--- @warning Do not redefine it : too many hard codes depend on it.
--: number shell_toggleconsolekey;
shell_toggleconsolekey = 96  -- configurable ... ( currently = ` )


--- Read a string shell command.
--- @ingroup dcplaya_lua_enhanced_shell
---  
---   Do line edition, command historic and more ...
---
---  @param  string  Initial string (typically "").
---  @return read string
--
function shell_input(string)

   if not check_zed() then
      return nil
   end

   if not string then
      string = ""
   end

   local done
   local col=strlen(string)+1
   local scrollx=1
   local w, h, y
   local line

   w, h = consolesize()
   w = w-2
   y = h-1

   -- make sure the recall table does not get too big
   while getn(shell_recalltable)>shell_maxrecall do
      tremove(shell_recalltable, 1)
   end

   -- copy recall table in temporary editing buffer
   local t = {}
   local i
   for i=1, getn(shell_recalltable), 1 do
      t[i] = shell_recalltable[i]
   end
   tinsert(t, string)
   line = getn(t)

   -- main loop
   repeat

      -- update scroll position if necessary
      if col > scrollx + w - 2 then
	 scrollx = col-w+2
      end

      if col < scrollx + 2 then
	 scrollx = max(1, col-2)
      end

      rp(MT_WRAPOFF)

      -- display input
      zed_pline(y, "> "..strsub(t[line], scrollx, scrollx+w-1))

      -- place cursor
      zed_gotoxy(col-scrollx+2, y)

      rp(MT_WRAPON)

      -- hande keys
      local key
      key = getchar();

      if key == KBD_ENTER then
	 done=1
      elseif key == KBD_KEY_UP then
	 line = max(1, line-1)
	 col = strlen(t[line])+1
	 scrollx = 1
      elseif key == KBD_KEY_DOWN then
	 line = min(getn(t), line+1)
	 col = strlen(t[line])+1
	 scrollx = 1
      elseif key == KBD_KEY_F1 then
	 return "exit"
      elseif key == KBD_KEY_F2 then
	 dofile (home.."autorun.lua")
      elseif key == KBD_KEY_F3 then
	 dofile (home.."autorun3.lua")
      elseif key == KBD_KEY_F4 then
	 dofile (home.."autorun4.lua")
      elseif key == shell_toggleconsolekey then
	 toggleconsole()
      else
	 t[line], col = zed_edline(t[line], col, key)
      end

   until done

   string = t[line]
   -- add new command in recall table if not empty
   if strlen(string) > 0 then
      tinsert(shell_recalltable, string)
   end
   return string

end

--
--- Shell error exception handling.
--- @ingroup dcplaya_lua_enhanced_shell
---
---  This function relaunch the shell !
---
--- @deprecated Does not seem necessary actually.
--
function shell_error(err)
   print(err)
   _ERRORMESSAGE=shell_olderror
   print [[Returning to shell ...]]
   shell()
   print [[Return from shell_error]]
end

--- Call a new shell with enhanced line editing and more.
--- @ingroup dcplaya_lua_enhanced_shell
---
---   Just type `exit` to quit that shell.
---   
function shell()

   local command=""
   local done = nil

   if not check_zed() then
      return
   end

   repeat

      command = shell_input("")
      print ()

      if command == "exit" then
	 done = 1
      else
	 -- hook our error handling function
	 --			if _ERRORMESSAGE~=shell_error then
	 --				shell_olderror=_ERRORMESSAGE
	 --				_ERRORMESSAGE=shell_error
	 --			end

	 doshellcommand(command)

	 --			_ERRORMESSAGE=shell_olderror
      end

   until done

end

return 1
