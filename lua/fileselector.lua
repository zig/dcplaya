--- fileselector.lua
-- 
-- author : benjamin gerard <ben@sashipa.com>
-- date   : 2002/10/04
--
-- $Id: fileselector.lua,v 1.6 2002-10-07 23:54:58 benjihan Exp $
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
-- | FILESELECTOR : SELECT A FILE                                   | 
-- | +---------------------------------------------------+ +------+ |<Y1
-- | |                                                   | |CANCEL| |
-- | |                                                   | +------+ |
-- | |                                                   | +------+ |
-- | |                                                   | | MKDIR| |
-- | |                                                   | +------+ |
-- | |                                                   | +------+ |
-- | |                                                   | |  COPY| | W
-- | |                                                   | +------+ |
-- | |                                                   | +------+ |
-- | |                                                   | +  MOVE| |
-- | |                                                   | +------+ |
-- | |                                                   | +------+ |
-- | |                                                   | |DELETE| |
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

	local y,y1,y2,y3,y4,y5
	y  = (screenh-h)/2
	y2 = y+h
	y3 = y2-bordery
	y4 = y3-bh
	y1 = y+bordery
	y5 = y4-spany
		 
----------------
-- MAIN DIALOG
----------------
	function fs_change_mode(dial,mode)
		if mode == dial.mode then return end
		print(format("Changing mode %d->%d",dial.mode,mode))
		dial.mode = mode
	end

	function dial_handle(dial,evt)
		local key = evt.key
		if key == gui_item_confirm_event then
			print("FL-CONFIRM")
			gui_input_set(dial.input, dial.flist.fl.pwd)
			gui_new_focus(dial, dial.input)
			return
		elseif key == gui_item_cancel_event then
			print("FL-CANCEL")
			fs_change_mode(dial,0)
			gui_new_focus(dial, dial.input)
			return
		elseif key == gui_item_change_event then
			print("FL-CHANGE")
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
		[gui_item_change_event]		= dial_handle
	}
	dial.mode = 0

	function mkbutton(p)
		local but
		but = gui_new_button(%dial, p.box, p.name)
	  	but.event_table[gui_press_event] = p.handle
	end

	function but_cancel_handle(but,evt)
		print("CANCEL")
		evt_shutdown_app(but.owner)
		return
	end

	function but_mkdir_handle(but,evt)
		local dial = but.owner
		if strlen(dial.input.input) > 0 then
			mkdir("-v",dial.input.input)
			filelist_path(dial.fl) -- Update 
		end
		return
	end

	function but_copy_handle(but,evt)
		print("COPY")
		return
	end

	function but_move_handle(but,evt)
		print("MOVE")
		return
	end

	function but_delete_handle(but,evt)
		print("DELETE")
		return
	end

	local butdef = {
		{ name="CANCEL",	handle=but_cancel_handle	},
		{ name="MKDIR", 	handle=but_mkdir_handle 	},
		{ name="COPY", 		handle=but_copy_handle 		},
		{ name="MOVE", 		handle=but_move_handle 		},
		{ name="DELETE", 	handle=but_delete_handle 	},
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
	dial.input = gui_new_input(dial, { x1, y4, x3, y3 })

------------
-- FILELIST
------------

	function confirm(fl)
		print("FILESELECTOR-CONFIRM")
		if not fl or not fl.dir or not fl.entries then return end
		local entry = fl.dir[fl.pos+1]
		if entry.size and entry.size==-1 then
			filelist_path(fl,entry.name)
			return
		else
			return 1
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
