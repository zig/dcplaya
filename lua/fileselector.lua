--- fileselector.lua
-- 
-- author : benjamin gerard <ben@sashipa.com>
-- date   : 2002/10/04
--
-- $Id: fileselector.lua,v 1.10 2002-10-14 19:10:06 benjihan Exp $
--
-- TODO : select item with space 
--        completion with tab        
--

-- 
fileselector_loaded=nil

-- Load required libraries
--
dolib("gui")
dolib("textlist") -- Bug when nested !!
dolib("filelist")

--
--
function fileselector(name,path,filename)
	local dial,but,input

-- FILESECTOR LAYOUT
--
-- +----------------------------------------------------------------+<Y
-- | FILESELECTOR-NAME                                              | 
-- | +---------------------------------------------------+ +------+ |<Y1
-- | |                                                   | |CANCEL| |
-- | |                                                   | +------+ |
-- | |                                                   | +------+ |
-- | |                                                   | | MKDIR| |
-- | |                                                   | +------+ |
-- | |                                                   | +------+ |
-- | |                                                   | |  COPY| |
-- | |                                                   | +------+ |
-- | |                                                   | +------+ |
-- | |                                                   | +  MOVE| |
-- | |                                                   | +------+ |
-- | |                                                   | +------+ |
-- | |                                                   | |DELETE| |
-- | |                                                   | +------+ |
-- | |                                                   | +------+ |
-- | |                                                   | |LOCATE| |
-- | +---------------------------------------------------+ +------+ |<Y5
-- | +------------------------------------------------------------+ |<Y4
-- | | INPUT                                                      | |
-- | -------------------------------------------------------------+ |<Y3
-- | +------------------------------------------------------------+ |<Y8
-- | | COMMAND INPUT                                              | | 
-- | +------------------------------------------------------------+ |<Y9
-- | +------------------------------------------------------------+ |<Y6
-- | | STATUS TEXT                                                | | 
-- | +------------------------------------------------------------+ |<Y7
-- +----------------------------------------------------------------+<Y2
-- ^ ^                                                   ^ ^      ^ ^  
-- X X1                                                 X5 X4    X3 X2

---------------------------
-- BUTTONS' EVENT HANDLERS
---------------------------
	function but_cancel_handle(but,evt)
--		print("BUTTON CANCEL")
		local dial = but.owner 
		local com = get_command(dial)
		if not com then
			evt_shutdown_app(dial)
		else
			status(dial,com.." cancelled")
			set_command(dial,nil)
		end
		return nil
	end

	function but_mkdir_handle(but,evt)
--		print("BUTTON MKDIR")
		local dial = but.owner
		set_command(dial,[[mkdir("-v")]])
		gui_new_focus(dial,dial.input)

--		local dial,fl
--		dial = but.owner
--		fl = dial.flist.fl
--		if strlen(dial.input.input) > 0 then
--			mkdir("-v",dial.input.input)
--			status(dial,"Loading "..fl.pwd)
--			filelist_path(fl) -- Update 
--			status(dial,dial.input.input.." created")
--			change(dial)
--		end
	end

	function but_copy_handle(but,evt)
--		print("BUTTON COPY")
		local dial = but.owner
		set_command(dial,[[copy()]])
		gui_new_focus(dial,dial.input)
	end

	function but_move_handle(but,evt)
--		print("BUTTON MOVE")
		local dial = but.owner
		set_command(dial,[[copy("-u")]])
		gui_new_focus(dial,dial.input)
	end

	function but_delete_handle(but,evt)
--		print("BUTTON DELETE")
		local dial = but.owner
		set_command(dial,[[unlink()]])
		gui_new_focus(dial,dial.input)

--		fl = dial.flist.fl
--		if strlen(dial.input.input) > 0 then
--			unlink("-v",dial.input.input)
--			status(dial,"Loading "..fl.pwd)
--			filelist_path(fl) -- Update
--			status(dial,dial.input.input.." removed")
--			change(dial)
--		end
		return
	end

	function wildcard_to_regexpr(wc)
		if not wc then return end
		local res = wc
		res = gsub(res,"%%","%%")
		res = gsub(res,"%.","\%.")
		res = gsub(res,"%*",".*")
		res = gsub(res,"?",".")
		res = "^"..res.."$"
--		print("wildcard:"..wc.."->"..res)
		return res
	end

	function locate(dial,text)
		local path,leaf,fl
		if not dial then return end
		fl = dial.flist.fl
		path,leaf = get_path_and_leaf(text)
		if path then
--			print("path="..path.."  "..fl.pwd="..fl.pwd)
			if path ~= fl.pwd then
				-- Not same path, try to load new one
				status(dial,"Loading "..path)
				if not filelist_path(fl,path) then
					status(dial,"Error loading "..path)
					return
				end
				status(dial,path.." loaded")
			end
		end

		if leaf then
--			print(format("try to locate '%s' in '%s'",leaf, fl.pwd))
			if textlist_find_entry_expr(fl,wildcard_to_regexpr(leaf)) then
				status(dial,fl.pwd..leaf.." found")
			else
				status(dial,fl.pwd..leaf.." not found")
			end
			return
		end
		return
	end


	function but_locate_handle(but,evt)
--		print("BUTTON LOCATE")
		local dial,fl
		dial = but.owner
		fl = dial.flist.fl
		locate(dial,dial.input.input)
	end

----------------
-- BUILD LAYOUT
----------------
	local butdef = {
		{ name="CANCEL",	handle=but_cancel_handle	},
		{ name="MKDIR", 	handle=but_mkdir_handle 	},
		{ name="COPY", 		handle=but_copy_handle 		},
		{ name="MOVE", 		handle=but_move_handle 		},
		{ name="DELETE", 	handle=but_delete_handle 	},
		{ name="LOCATE", 	handle=but_locate_handle 	},
	}

	local screenw, screenh, w, h
	screenw = 640
	screenh = 480
	w = screenw/2

	local borderx, bordery, spanx, spany, bw, bh
	borderx = 6
	bordery = 10
	spanx=4
	spany=6
	bw=66
	bh=20
	h = 2*bordery + (getn(butdef)+3) * (bh+spany) - spany

	local x,x1,x2,x3,x4,x5
	x  = (screenw-w)/2
	x2 = x+w
	x1 = x+borderx
	x3 = x2-borderx
	x4 = x3-bw
	x5 = x4-spanx

	local y,y1,y2,y3,y4,y5,y6,y7,y8,y9
	y  = (screenh-h)/2-32
	y1 = y + bh
	y5 = y1 + getn(butdef) * (bh+spany)
	if y5 - y1 < 128 then y5 = y1 + 128 end
	y4 = y5+spany
	y3 = y4+bh
	y8 = y3+spany
	y9 = y8+bh
	y6 = y9+spany
	y7 = y6+bh
	y2 = y7+spany
	h  = y2-y
		 
----------------
-- MAIN DIALOG
---------------

	-- Set status text
	function status(dial,text)
		if dial.status then
			gui_text_set(dial.status,text)
		end
	end
	
	-- Get command name from command input
	function get_command(dial)
		local start,stop,com
		com = nil
		if dial.command then
			start,stop,com = strfind(dial.command.input,"^([%w_]+)%(.*%)")
--			if com then print ("COM:"..com) else print("COM:nil") end
		end
		return com
	end

	-- Set command input text
	function set_command(dial,com)
		if not dial.command then return end
		local start, stop
		if com then
--			print("SET-COMMAND:"..com)
			start,stop = strfind(com,")",1,1)
--			if start then print("POS:"..start) end
			gui_input_set(dial.command, com, start)
			local c = get_command(dial)
			if c then
				status(dial,"Editing '"..c.."' command")
			else
				status(dial, format("Wrong command syntax '%s'",com))
				gui_input_set(dial.command)
			end
		else
--			print("CLR-COMMAND")
			gui_input_set(dial.command)
		end
	end

	-- Set file input text form current filelist entry
	function change(dial)
		local entry = filelist_get_entry(dial.flist.fl)
		if entry then
			gui_input_set(dial.input, entry.full)
		end
	end

	-- Get name of  current filelist entry
	function current(dial)
		local entry = textlist_get_entry(dial.flist.fl)
		if not entry then return "" end
		return entry.name
	end

	-- Main dialog event handle function
	function dial_handle(dial,evt)
		local key = evt.key
		if key == gui_item_confirm_event then
--			print("FL-CONFIRM")
			gui_new_focus(dial, dial.input)
			return
		elseif key == gui_item_cancel_event then
--			print("FL-CANCEL")
			gui_new_focus(dial, dial.input)
			return
		elseif key == gui_item_change_event then
--			print("FL-CHANGE")
			change(dial)
			return
		elseif key == gui_input_confirm_event then
--			print("FL-INPUT-CONF")
			local com = get_command(dial)

			if evt.input == dial.input then
--				print("-->INPUT")
				local com = get_command(dial)
				if not com then
					print("-->INPUT SELECTED:"..dial.input.input)
				else
					local command = dial.command
					local input = dial.input
					local fl = dial.flist.fl
					local col = command.input_col-1
					local f = "%q"
					local c = strsub(command.input, col, col)
					print("char:"..c)
					if c == [["]] then
						f = ",%q"
					end
					local path,leaf
					path,leaf = get_path_and_leaf(input.input)
					if not path then path = fl.pwd end
					if leaf then path=path..leaf end
					gui_input_insert(command,format(f,path))
				end
				return
			elseif evt.input == dial.command then
--				print("-->COMMAND")
				if com then
					local result,fl
					status(dial,format("Execute '%s'",dial.command.input))
					result = dostring(dial.command.input)
					fl = dial.flist.fl
					status(dial,format("Loading '%s'",fl.pwd))
					filelist_path(dial.flist.fl)
					if result then
						status(dial, com.." success")
					else
						status(dial, com.." failure")
					end
					set_command(dial,nil)
				end
				return
			end
			print("-->???")
			return evt
		end
		return evt
	end

	if not name then name="File Selector" end
	if not path then path=PWD end
	dial = gui_new_dialog(evt_desktop_app,
		{x, y, x2, y2 }, nil, nil, name, { x = "left", y = "up" } )
	dial.event_table = {
		[gui_item_confirm_event]	= dial_handle,
		[gui_item_cancel_event]		= dial_handle,
		[gui_item_change_event]		= dial_handle,
		[gui_input_confirm_event]	= dial_handle
	}

	function mkbutton(p)
		local but
		but = gui_new_button(%dial, p.box, p.name)
	  	but.event_table[gui_press_event] = p.handle
	end

---------------
-- ALL BUTTONS
---------------
	dial.buttons = {}
	local i, b, yb
	i = 1
	yb = y1
	while butdef[i] do
		butdef[i].box = {x4,yb,x3,yb+bh}
		dial.buttons[i] = mkbutton(butdef[i])
		yb = yb + bh + spany;
		i = i + 1
	end

--------------
-- FILE INPUT
--------------
	local iname = path
	if filename then iname = iname.."/"..filename end
	dial.input = gui_new_input(dial, { x1, y4, x3, y3 }, nil,nil,iname)

-----------------
-- COMMAND INPUT
-----------------
	dial.command = gui_new_input(dial, { x1, y8, x3, y9 })

----------
-- STATUS
----------
	-- create an input item
	dial.status = gui_new_text(dial, { x1, y6, x3, y7 }, nil,
		{x="left"})

------------
-- FILELIST
------------

	function confirm(fl)
--		print("FILESELECTOR-CONFIRM")
		if not fl or not fl.dir or fl.entries < 1 then return end
		local entry = fl.dir[fl.pos+1]
		if not entry then return end
		if entry.size and entry.size==-1 then
			local action
			
			action = filelist_path(fl,entry.name)
			if not action then
				return
			end
			return 2
		else
			return 3
		end
	end

	dial.flist = gui_filelist(dial,
		{	pos={x1,y1},
			pwd=path,
			confirm=confirm,
			box={x5-x1, y5-y1, x5-x1, y5-y1}
		})

	return dial
end

fileselector_loaded = 1

dial = nil
if not nil then
	print("Run test (y/n) ?")
	c = getchar()
	if c == 121 then
		dial = fileselector("SELECT A FILE", "/")
	end
end

return fileselector_loaded
