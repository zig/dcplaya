--
-- shell extension
--
-- author : Vincent Penne
--
-- $Id: shell.lua,v 1.13 2003-03-12 22:02:00 ben Exp $
--

-- Added by ben :

-- Keywords that could begin a lua sentence.
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

-- Default table of variable type that the shell is allowed to substitut.
shell_default_substitut = {
   ["string"] = 1,
   ["number"] = 1,
}

-- Current table for shell substitution
shell_substitut = nil --shell_default_substitut

-- We reimplement the doshellcommand function to extend the shell syntax.
-- The new syntax is backward compatible with lua normal syntax (? I hope !)
-- and add possibility to type more usual command like "ls" without trailing ()
-- or "cd plugins" without the string quotes.
-- This only replace syntax at shell level, the lua syntax remains unchanged
-- so lua scripts should use normal syntax ...
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
      local st,sp,fct

      if sec == '=' or strfind(list[1], "=") then
	 -- affectation : do it as is too !
	 return dostring(string)
      end

      -- Check for function call and get its name.
      st,sp,fct = strfind(list[1], "[%s]*([%w_]+)[%s]*[(]")
      if not fct and sec == '(' then
	 fct = list[1]
      end

      if fct then
	 -- function call , check if it exist and if it is a function.
	 local f = getglobal(fct)
	 if not f then
	    print(format("Undefined command %q", fct))
	    return
	 end
	 if type(f) ~= "function" then
	    print(format("Not a function %q : %d", fct, type(f)))
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

-- function to input a string on one line
-- this handles command recall too
shell_recalltable = {}
shell_maxrecall = 20         -- maximum number of recallable commands
shell_toggleconsolekey = 96  -- configurable ... ( currently = ` )
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



-- error exception handling : relaunch the shell !
-- does not seem necessary actually ... NOT USED
--function shell_error(...)
--	shell_olderror(arg)
function shell_error(err)
   print(err)
   _ERRORMESSAGE=shell_olderror
   print [[Returning to shell ...]]
   shell()
   print [[Return from shell_error]]
end


-- Call a new shell with enhanced line editing.
-- Type "exit" to quit.
-- Also, since it uses getchar, it will dispatch events to other applications.
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

shell_loaded=1
