--- @ingroup  dcplaya_lua_zed
--- @file     zed.lua
--- @author   vincent penne
--- @brief    ZED, The Ziggy's Editor
---
--- $Id: zed.lua,v 1.12 2003-03-26 23:02:50 ben Exp $
--
-- (C) 2002 Vincent Penne (aka Ziggy Stardust)
--
--

--- @defgroup  dcplaya_lua_zed  Zed, The Ziggy's Editor
--- @ingroup   dcplaya_lua_basics
--- @brief     zed, the ziggy's editor
---
--- Zed is a console based text editor.
---
--- @par Key binding
---
---   Key should have standard meaning. Currently terminal escape sequence
---   can not be used. Function key used are :
---     - @b F10 : Quit
---     - @b F2  : Save file
---     - @b F3  : Load new file
---
--- @author vincent penne
---
--- @{
---

rp ("Initializing ZED ... ")

-- Unload library
zed_loaded = nil

-- Load required libraries
if not dolib("mu_term",1) then return end
if not dolib("keydefs",1) then return end

zed_w, zed_h	=	consolesize()

--- put cursor at x y in console.
--- @internal
function zed_gotoxy(x, y)
   --	rp (MT_POS..format("%c%c", 32+y, 32+x))
   rp (MT_POS..strchar(32+y, 32+x))
end

--- clear screen.
--- @internal
function zed_cls()
   local	i
   for i=0, zed_h-1, 1 do	
      zed_gotoxy(0, i)
      rp (MT_CLRLINE)
   end
end

--- clear current line.
--- @internal
function zed_cll()
   rp (MT_CLRLINE)
end

zed_print=rp

--- print at specified position.
--- @internal
function zed_printat(x, y, ...)
   zed_gotoxy(x, y)
   call(zed_print, arg)
end

--- print at specified line, erasing it before.
--- @internal
function zed_pline(y, ...)
   zed_gotoxy(0, y)
   zed_cll()
   call(zed_print, arg)
end


--- save cursor position.
--- @internal
function zed_savecursorpos()
   rp(MT_CURSAVE)
end


--- restore cursor position.
--- @internal
function zed_restorecursorpos()
   rp(MT_CURREST)
end


--- initialize console (clear screen, wrap off).
--- @internal
--- @warning not reentrant (global variable !)
--
function zed_initconsole()
   zed_w, zed_h	= consolesize()
   rp(MT_WRAPOFF)
   zed_cls()
   -- warning : not reentrant (global variable !)
   zed_oldconsole = (showconsole() ~= 0)
end


--- desinitialize console (clear screen, wrap on).
--- @internal
--- @warning not reentrant (global variable !)
function zed_desinitconsole()
   rp(MT_WRAPON)
   zed_cls()

   -- warning : not reentrant (global variable !)
   if zed_oldconsole then
      showconsole()
   else
      hideconsole()
   end
end


--- Edit a line.
---
--- @param string  string  buffer to edit
--- @param number  col     position of the cursor in the line
--- @param keycode key     input key event to handle
---
--- @return string,col 
--- (modified string taking in account key event, modified cursor position)
---
function zed_edline(string, col, key)

   if key == KBD_KEY_LEFT then
      return string, max(col - 1, 1)
   elseif key == KBD_KEY_RIGHT then
      return string, min(col + 1, strlen(string)+1)
   elseif key == KBD_KEY_DEL then
      return strsub(string, 1, col-1)..strsub(string, col+1), col
   elseif key == KBD_BACKSPACE then
      return strsub(string, 1, max(col-2, 0))..strsub(string, col), max(col-1, 1)
   elseif key == KBD_KEY_HOME then
      return string, 1
   elseif key == KBD_KEY_END then
      return string, strlen(string)+1
   elseif key < 256 and key >= 32 then
      return strsub(string, 1, col-1)..strchar(key)..strsub(string, col), col+1
   end

   -- unhandled event
   return string, col
end


--- function to input a string on one line.
function zed_input(string, y)

   if not string then
      string = ""
   end

   local done
   local col=strlen(string)+1
   local scrollx=1

   repeat

      -- update scroll position if necessary
      if col > scrollx + zed_w - 2 then
	 scrollx = col-zed_w+2
      end

      if col < scrollx + 2 then
	 scrollx = max(1, col-2)
      end

      -- display input
      zed_pline(y, strsub(string, scrollx, scrollx+zed_w-1))

      -- place cursor
      zed_gotoxy(col-scrollx, y)

      -- hande keys
      local key
      key = getchar();

      if key == KBD_ENTER then
	 done=1
      elseif key == KBD_ESC then
	 return nil
      else
	 string, col = zed_edline(string, col, key)
      end

   until done

   return string

end

--- Read a file.
---
--- @param  filename  file to read.
--- @param  buffer    line buffer to insert file in (created if nil).
---
--- @return buffer
--- @retval nil on error
--
function zed_readfile(filename, buffer)
   local handle = readfrom(filename)
   if not handle then
      print ("[zed] : could not open file ["..filename.."] for reading.")
      zed_message="*** Could not read file"
      return nil
   end

   if type(buffer) ~= "table" then
      buffer = {}
   end
   print ("[zed] : reading file ["..filename.."]")

   local n = getn(buffer)
   local line = read("*l")
   while line do
      tinsert(buffer, line)
      line = read("*l")
   end
   zed_message=format("*** #%d line(s) read", getn(buffer) - n)
   closefile(handle)

   return buffer
end


--- Write a file.
--- @param filename  name of file to write
--- @param buffer    line buffer
--- @return error-code
--- @return nil on error
function zed_writefile(filename, buffer)

   local handle = writeto(filename)
   if not handle then
      print ("[zed] : could not open file ["..filename.."] for writing.")
      zed_message="*** Could not write to file"
      return nil
   end

   print ("[zed] : writing file ["..filename.."]")

   local i=0
   if type(buffer) == "table" then
      while buffer[i] do
	 i = i+1
	 write(buffer[i].."\n")
      end
   end
   zed_message=format("*** #%d line(s) written",i)

   closefile(handle)
   return 1

end


function zed_pathsplit(filename)
   local i
   local j

   i = 0
   repeat
      i = strfind(filename, "/", i+1)
      if i then
	 j = i
      end
      --		print(filename, " ", i)
   until not i

   if j then
      return strsub(filename, 1, j), strsub(filename, j+1)
   else
      return nil, filename
   end
   
end


--- ZED editor.
--- @param  filename  optional filename
---
function zed(filename)

   local	line=1
   local	col=1
   local	scroll=1
   local	scrollx=1
   local	oldscroll=-1
   local	oldscrollx=-1
   local	buffer = {}
   local	dir=""
   local	file=""

   if filename then
      zed_readfile(filename, buffer)
      dir, file = zed_pathsplit(filename)
   end


   zed_initconsole()
   
   --zed_gotoxy(0, 0)
   --print("ZED - Ziggy's Editor")

   local update_from = scroll
   local update_to = scroll + zed_h-2

   local done
   repeat

      -- make sure current line exists
      if not buffer[line] then
	 buffer[line] = ""
      end

      -- update scroll position if necessary
      if line > scroll + zed_h - 2 then
	 scroll = line-zed_h+2
      end

      if line < scroll + 2 and line > 2 then
	 scroll = line-2
      end

      if col > scrollx + zed_w - 2 then
	 scrollx = col-zed_w+2
      end

      if col < scrollx + 2 then
	 scrollx = max(1, col-2)
      end

      if scroll ~= oldscroll or scrollx ~= oldscrollx then
	 update_from = scroll
	 update_to = scroll + zed_h-2
	 oldscroll = scroll
	 oldscrollx = scrollx
      end

      -- -------------
      -- update screen
      -- -------------

      -- update text lines
      if update_to >= update_from then
	 local i
	 for i=update_from, update_to, 1 do
	    if (buffer[i]) then
	       zed_pline(i-scroll, strsub(buffer[i], scrollx, scrollx+zed_w-1))
	    else
	       zed_pline(i-scroll, "")
	    end
	 end
      end

      -- info line
      if zed_message then
	 zed_pline(zed_h-1, zed_message)
	 zed_message = nil
      else
	 zed_pline(zed_h-1, format("ZED - %s - line %2d    col %2d (%d %d)", file, line, col, scroll, scrollx))
      end

      -- place cursor
      zed_gotoxy(col-scrollx, line-scroll)
      

      -- ------------------
      -- handle user event
      -- -------------------

      -- get next key event
      local key
      key=getchar()

      --zed_pline(2, key)

      update_from = line
      update_to = line-1

      if key == KBD_KEY_F10 then

	 zed_pline(zed_h-1, "Quit (Y/N) ? ")
	 local answer = strchar(max(1, min(255, getchar())))
	 if answer == "Y" or answer == "y" then
	    done = 1
	 end
	 zed_cll()

      elseif key == KBD_KEY_F2 then

	 zed_pline(zed_h-2, "Name of file to write to :")
	 local answer = zed_input(filename, zed_h-1)

	 if answer then
	    zed_writefile(answer, buffer)
	    filename = answer
	    dir, file = zed_pathsplit(filename)
	 end

	 -- force screen refresh
	 oldscroll = -1

      elseif key == KBD_KEY_F3 then

	 zed_pline(zed_h-2, "Name of file to read :")
	 local answer = zed_input(filename, zed_h-1)

	 if answer then
	    buffer = {}
	    zed_readfile(answer, buffer)
	    filename = answer
	    dir, file = zed_pathsplit(filename)

	    -- place cursor position
	    line = 1
	    col = 1
	    scroll = 1
	    scrollx = 1
	 end

	 -- force screen refresh
	 oldscroll = -1

      elseif key == KBD_ENTER then

	 tinsert(buffer, line+1, strsub(buffer[line], col))
	 buffer[line] = strsub(buffer[line], 1, col-1)
	 line = line+1
	 col = 1
	 update_to = scroll + zed_h-2

      elseif key == KBD_KEY_UP then

	 line = max(1, line-1)

      elseif key == KBD_KEY_DOWN then

	 line = min(getn(buffer), line+1)

      elseif key == KBD_KEY_PGUP then

	 line = max(1, line-(zed_h-1))
	 scroll = max(1, scroll-(zed_h-1))

      elseif key == KBD_KEY_PGDOWN then

	 line = min(getn(buffer), line+(zed_h-1))
	 scroll = min(getn(buffer), scroll+(zed_h-1))

      else

	 -- line level editing

	 -- make sure current line exists
	 --			if not buffer[line] then
	 --				buffer[line] = ""
	 --			end

	 local l = strlen(buffer[line])+1
	 if col > l then
	    col = l
	 end

	 if line > 1 and col == 1 and key == KBD_KEY_LEFT then
	    line = line-1
	    col = strlen(buffer[line])+1
	 elseif buffer[line+1] and col == l and key == KBD_KEY_RIGHT then
	    line = line+1
	    col = 1
	 elseif buffer[line+1] and col == l and key == KBD_KEY_DEL then
	    buffer[line] = buffer[line]..buffer[line+1]
	    tremove(buffer, line+1)
	    col = l
	    update_to = scroll + zed_h-2
	 elseif line > 1 and col == 1 and key == KBD_BACKSPACE then
	    line = line-1
	    l = strlen(buffer[line])+1
	    buffer[line] = buffer[line]..buffer[line+1]
	    tremove(buffer, line+1)
	    col = l
	    update_from = line
	    update_to = scroll + zed_h-2
	 else
	    buffer[line], col = zed_edline(buffer[line], col, key)
	    update_to = line
	 end

      end

   until done

   zed_desinitconsole()

   print("ZED is done")

end

--
--- @}
---

addhelp(zed,[[print
[[zed(filename) : This is ZED (Ziggy's Editor).

 Usage is pretty standard. 

   F10 : Quit
   F2  : Save file
   F3  : Load new file
]]]])

zed_loaded=1
return zed_loaded
