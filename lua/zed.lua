--
-- this is ZED : The Ziggy's Editor
--
-- (C) 2002 Vincent Penne (aka Ziggy Stardust)
--
-- $Id: zed.lua,v 1.1 2002-09-15 15:31:03 zig Exp $
--

rp ("Initializing ZED ... ")

dofile (home.."mu_term.lua")
dofile (home.."keydefs.lua")

zed_w	=	60
zed_h	=	20

-- put cursor at x y in console
function zed_gotoxy(x, y)
--	rp (MT_POS..format("%c%c", 32+y, 32+x))
	rp (MT_POS..strchar(32+y, 32+x))
end

-- clear screen
function zed_cls()
	local	i
	for i=0, zed_h-1, 1 do	
		zed_gotoxy(0, i)
		rp (MT_CLRLINE)
	end
end

-- clear current line
function zed_cll()
	rp (MT_CLRLINE)
end

zed_print=rp

-- print at specified position
function zed_printat(x, y, ...)
	zed_gotoxy(x, y)
	call(zed_print, arg)
end

-- print at specified line, erasing it before
function zed_pline(y, ...)
	zed_gotoxy(0, y)
	zed_cll()
	call(zed_print, arg)
end


-- initialize console (clear screen, wrap off)
function zed_initconsole()
	rp(MT_WRAPOFF)
	zed_cls()
end


-- function to edit a line
--
-- string : buffer to edit
-- col    : position of the cursor in the line
-- key    : input key event to handle
--
-- return : string, col 
-- (modified string taking in account key event, modified cursor position)
--
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


function zed_readfile(filename, buffer)

	local handle = readfrom(filename)
	if not handle then
		print ("Could not open file '", filename, "' for reading ...")
		return nil
	end

	print ("Reading file '", filename, "' ...")

	local line = read("*l")

	while line do
		tinsert(buffer, line)
		line = read("*l")
	end

	closefile(handle)

	return buffer

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


-- this is ZED editor, call it with an optional filename
function zed(filename)

	local	line=1
	local	col=1
	local	scroll=1
	local	oldscroll=-1
	local	buffer = {}
	local	dir=""
	local	file=""

	if (filename) then
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
--		if not buffer[line] then
--			buffer[line] = ""
--		end

		-----------------
		-- update screen
		-----------------

		-- update text lines
		if update_to >= update_from then
			local i
			for i=update_from, update_to, 1 do
				if (buffer[i]) then
					zed_pline(i-scroll, strsub(buffer[i], 1, zed_w))
				else
					zed_pline(i-scroll, "")
				end
			end
		end

		-- info line
		zed_pline(zed_h-1, format("ZED - Ziggy's Editor - %s - line %2d    col %2d", file, line, col))

		-- place cursor
		zed_gotoxy(col-1, line-scroll)
	

		local key
		key=getchar()

		--zed_pline(2, key)

		update_from = line
		update_to = line-1

		if key == KBD_KEY_F1 then

			zed_pline(zed_h-1, "Quit (Y/N) ? ")
			local answer = strchar(max(1, min(255, getchar())))
			if answer == "Y" or answer == "y" then
				done = 1
			end
			zed_cll()

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

			-- make sure current line exists
			if not buffer[line] then
				buffer[line] = ""
			end

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


		-- update scroll position if necessary
		if line > scroll + zed_h - 2 then
			scroll = line-zed_h+2
		end

		if line < scroll + 2 and line > 2 then
			scroll = line-2
		end

		if scroll ~= oldscroll then
			update_from = scroll
			update_to = scroll + zed_h-2
			oldscroll = scroll
		end



	until done

	zed_cls()

	print("ZED is done")

end

print ("ZED ready !")

