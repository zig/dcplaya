--- fileselector.lua
-- 
-- author : benjamin gerard <ben@sashipa.com>
-- date   : 2002/10/04
--
-- $Id: fileselector.lua,v 1.7 2002-10-08 08:22:34 benjihan Exp $
--
-- TODO : select item with space 
--        completion with tab        
--

if not filelist_loaded then
	dofile("lua/filelist.lua")
end

--
--
function fileselector(name,path,filename)
	local dial,but,input

-- FILESECTOR LAYOUT
--
-- +----------------------------------------------------------------+<Y
-- | FILESELECTOR-NAME                                              | 
-- | +------------------------------------------------------------+ |<Y6
-- | | COMMAND INPUT                                              | | 
-- | +------------------------------------------------------------+ |<Y7
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
-- +----------------------------------------------------------------+<Y2
-- ^ ^                                                   ^ ^      ^ ^  
-- X X1                                                 X5 X4    X3 X2


	local screenw, screenh, w, h
	screenw = 640
	screenh = 480
	w = screenw/2
	h = screenh/2

	local borderx, bordery, spanx, spany, bw, bh
	borderx = 6
	bordery = 10
	spanx=4
	spany=6
	bw=66
	bh=20

	local x,x1,x2,x3,x4,x5
	x  = (screenw-w)/2
	x2 = x+w
	x1 = x+borderx
	x3 = x2-borderx
	x4 = x3-bw
	x5 = x4-spanx

	local y,y1,y2,y3,y4,y5,y6,y7
	y  = (screenh-h)/2-32
	y6 = y + bh
	y7 = y6+bh
	y2 = y+h
	y3 = y2-bordery
	y4 = y3-bh
	y1 = y7+spany
	y5 = y4-spany
		 
----------------
-- MAIN DIALOG
----------------
	function status(dial,text)
		if dial.status then
			gui_text_set(dial.status,text)
		end
	end

	-- [0:fileselect] [1:destination-select]
	function change_mode(dial,mode)
		local old=dial.mode
		if mode == old then return old end
		print(format("Changing mode %d->%d",old,mode))
		if mode == 0 then
			status(dial,dial.action..dial.source.." cancelled")
		else
			status(dial,dial.action..dial.source.." to ")
		end
		dial.mode = mode
		return old
	end

	function change(dial)
		local entry = filelist_get_entry(dial.flist.fl)
		if entry then
			gui_input_set(dial.input, entry.full)
		end
	end

	function current(dial)
		local entry = textlist_get_entry(dial.flist.fl)
		if not entry then return "" end
		return entry.name
	end

	function dial_handle(dial,evt)
		local key = evt.key
		if key == gui_item_confirm_event then
--			print("FL-CONFIRM")
			gui_new_focus(dial, dial.input)
			return
		elseif key == gui_item_cancel_event then
--			print("FL-CANCEL")
--			change_mode(dial,0)
			gui_new_focus(dial, dial.input)
			return
		elseif key == gui_item_change_event then
--			print("FL-CHANGE")
			change(dial)
			return
		elseif key == gui_input_confirm_event then
			print("FL-INPUT-CONF")
			return
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
	dial.mode = 0

	function mkbutton(p)
		local but
		but = gui_new_button(%dial, p.box, p.name)
	  	but.event_table[gui_press_event] = p.handle
	end

	function but_cancel_handle(but,evt)
		local dial = but.owner 
		if dial.mode == 0 then
			evt_shutdown_app(dial)
		else
			change_mode(dial,0)
		end
	end

	function but_mkdir_handle(but,evt)
		local dial,fl
		dial = but.owner
		fl = dial.flist.fl

		if strlen(dial.input.input) > 0 then
			mkdir("-v",dial.input.input)
			status(dial,"Loading "..fl.pwd)
			filelist_path(fl) -- Update 
			status(dial,dial.input.input.." created")
			change(dial)
		end
	end

	function but_copy_handle(but,evt)
		local dial,fl,entry
		dial = but.owner
		fl = dial.flist.fl
		if dial.mode == 0 then
			print("COPY")
			dial.source = dial.input.input
			if strlen(dial.source) > 0 then
				dial.action="copy"
				change_mode(dial,1)
			end
		end
	end

	function but_move_handle(but,evt)
		print("MOVE")
	end

	function but_delete_handle(but,evt)
		local dial,fl
		dial = but.owner
		fl = dial.flist.fl

		if strlen(dial.input.input) > 0 then
			unlink("-v",dial.input.input)
			status(dial,"Loading "..fl.pwd)
			filelist_path(fl) -- Update
			status(dial,dial.input.input.." removed")
			change(dial)
		end
		return
	end

	function but_locate_handle(but,evt)
		local dial,fl,path,leaf
		dial = but.owner
		fl = dial.flist.fl
		path,leaf = get_path_and_leaf(dial.input.input)

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
			if textlist_find_entry_expr(fl,"^"..leaf.."$") then
				status(dial,fl.pwd..leaf.." found")
			else
				status(dial,fl.pwd..leaf.." not found")
			end
			return
		end
	end

	local butdef = {
		{ name="CANCEL",	handle=but_cancel_handle	},
		{ name="MKDIR", 	handle=but_mkdir_handle 	},
		{ name="COPY", 		handle=but_copy_handle 		},
		{ name="MOVE", 		handle=but_move_handle 		},
		{ name="DELETE", 	handle=but_delete_handle 	},
		{ name="LOCATE", 	handle=but_locate_handle 	},
	}

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

---------
-- INPUT
---------
	-- create an input item
	local iname = path
	if filename then iname = iname.."/"..filename end
	dial.input = gui_new_input(dial, { x1, y4, x3, y3 }, nil,nil,iname)

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
print("loaded fileselector.lua")
