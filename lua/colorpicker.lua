--- colorpicker.lua
-- 
-- author : benjamin gerard <ben@sashipa.com>
-- date   : 2002/10/11
--
-- $Id: colorpicker.lua,v 1.1 2002-10-11 12:05:21 benjihan Exp $
--
--

-- Load required libraries
--
dolib("gui")

--
--
function colorpicker(name, color)

-- COLORPICKER LAYOUT
--
-- +------------------------------------------+<Y
-- | COLORPICKER-NAME                         | 
-- | +-------------------+ +--+ +--+ +------+ |<Y1
-- | |COLORBOX           | |C | | A| |CANCEL| |
-- | |                   | |O | | L| +------+ |
-- | |                   | |L | | P| +------+ |
-- | |                   | |O | | H| | RESET| |
-- | |                   | |R | | A| +------+ |
-- | |                   | |  | |  | +------+ |<Y6
-- | |                   | |  | |  | + ARGB | |
-- | +-------------------| +--+ +--+ +------+ |<Y5
-- | +--------------------------------------+ |<Y4
-- | | STATUS TEXT                          | | 
-- | +--------------------------------------+ |<Y3
-- +------------------------------------------+<Y2
-- ^ ^                   ^  ^ ^ ^  ^ ^      ^ ^
-- X X1                X9 X8 X7 X6 X5 X4   X3 X2

---------------------------
-- BUTTONS' EVENT HANDLERS
---------------------------
	function but_cancel_handle(but,evt)
		print("BUTTON CANCEL")
		local dial = but.owner 
		evt_shutdown_app(dial)
		return nil
	end

	function but_reset_handle(but,evt)
		print("BUTTON RESET")
		local dial = but.owner
		set_pickcolor(dial,dial.reset_color)
		gui_new_focus(dial,dial.argb)
	end

	-- Convert "XX" -> [0..1]
	function hex2n(str)
		if not str then return 0 end
		local hc,lc
		hc = strsub(str,1,1)
		lc = strsub(str,2,2)
		h = tonumber(hc)
		l = tonumber(lc)
		if not h then h = tonumber(hc,16) end
		if not l then l = tonumber(lc,16) end

		print(h)
		print(l)
		if not h then
			h = 0
		elseif l then
			h = h*16 + l
		end
		print(format("%s -> %02f",str,h))
		return h/255
	end

	-- convert n [0..1] -> string "xx"
	function n2hex(n)
		if not n then return "00" end
		return format("%02X", floor((n*255)))
	end

	function color2hex(color)
		if not color then color = {0,0,0,0} end
		return strupper(n2hex(color[1])..n2hex(color[2])
						..n2hex(color[3])..n2hex(color[4]))
	end

	function hex2color(str)
		if not str then str = "00000000" end
		str = strupper(str)
		return {	hex2n(strsub(str,1,2)),	hex2n(strsub(str,3,4)),
					hex2n(strsub(str,5,6)),	hex2n(strsub(str,7,8)) }
	end

----------------
-- BUILD LAYOUT
----------------
	local butdef = {
		{ name="CANCEL",	handle=but_cancel_handle	},
		{ name="RESET", 	handle=but_reset_handle 	},
	}

	local borderx, bordery, spanx, spany, bw, bh
	borderx = 6
	bordery = 10
	spanx=4
	spany=6
	bw=66
	bh=20

	local screenw, screenh, w, h
	screenw = 640
	screenh = 480
	w = borderx + 128 + spanx + 16 + spanx + 16 + spanx + bw + borderx
	h = bordery + 128 + spany + bh + bordery

	local x,x1,x2,x3,x4,x5,x6,x7,x8,x9
	x  = (screenw-w)/2
	x2 = x+w
	x1 = x+borderx
	x9 = x1+128
	x8 = x9+spanx
	x7 = x8+16
	x6 = x7+spanx
	x5 = x6+16
	x4 = x5+spanx
	x3 = x4+bw
	w  = x2-x

	local y,y1,y2,y3,y4,y5,y6
	y  = (screenh-h)/2-32
	y1 = y+bh
	y5 = y1+128
	y6 = y5-bh
	y4 = y5+spany
	y3 = y4+bh
	y2 = y3+bordery
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
	
	-- Get color from argb input
	function get_pickcolor(dial)
		return hex2color(dial.argb.input)
	end

	-- Set
	function set_pickcolor(dial,color)
		if not dial or not color then return end
		if type(color) == "string" then
			color = hex2color(color)
		end
		if dial.argb then
			gui_input_set(dial.argb,color2hex(color))
		end

		-- Update colorbox
		if dial.colorbox then
			dl_clear(dial.colorbox.dl)
			dl_draw_box4(dial.colorbox.dl,
						0,0,128,128,0,
						1,0,0,0,
						1,0,0,0,
						1,1,1,1,
						1,color[2],color[3],color[4])

		end

		-- Update colorbar
		if dial.colorbar then
			local colors = dial.colorbar.colors
			local i,y
			local n = getn(colors)
			local h = 128/(n-1)
			
			dl_clear(dial.colorbar.dl)
			y=0
			for i=1, n-1, 1 do
				dl_draw_box(dial.colorbar.dl,
						0,y,16,y+h,0,
						1,colors[i][2],colors[i][3],colors[i][4],
						1,colors[i+1][2],colors[i+1][3],colors[i+1][4],2)
				y = y+h
			end
		end

		-- Update alphabar
		if dial.alphabar then
			local y = color[1]*128;
			dl_clear(dial.alphabar.dl)
			dl_draw_box (dial.alphabar.dl,0,0,16,128,0, 1,0,0,0, 1,1,1,1, 2)
			dl_draw_box1(dial.alphabar.dl,0,y,16,y+1,1, 1,1,0,0)
		end
	end

	-- Main dialog event handle function
	function dial_handle(dial,evt)
		local key = evt.key
		if key == gui_input_confirm_event then
			print("FL-INPUT-CONF")
			print("->"..dial.argb.input)
			set_pickcolor(dial, dial.argb.input)
			return nil
		end
		return evt
	end

	if not name then name="Color picker" end
	if not color then color = { 1,1,1,1 } end
	dial = gui_new_dialog(evt_desktop_app,
		{x, y, x2, y2 }, nil, nil, name, { x = "left", y = "up" } )
	dial.event_table = {
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

-------------
-- COLOR BOX
-------------
	dial.colorbox = gui_new_children(dial, {x1,y1,x9,y5})
	dl_set_trans(dial.colorbox.dl, mat_trans(x1,y1,dial.colorbox.z))
	dl_set_active(dial.colorbox.dl,1)

-------------
-- COLOR BAR
-------------
	dial.colorbar = gui_new_children(dial, {x8,y1,x7,y5})
	dl_set_trans(dial.colorbar.dl, mat_trans(x8,y1,dial.colorbox.z))
	dl_set_active(dial.colorbar.dl,1)
	dial.colorbar.colors = {
			{1,1,0,0},	{1,1,1,0},	{1,0,1,0},	{1,0,1,1},
			{1,0,1,1},	{1,0,0,1},	{1,1,0,1},	{1,1,0,0}
		}


-------------
-- ALPHA BAR
-------------
	dial.alphabar = gui_new_children(dial, {x6,y1,x5,y5})
	dl_set_trans(dial.alphabar.dl, mat_trans(x6,y1,dial.colorbox.z))
	dl_set_active(dial.alphabar.dl,1)



----------
-- STATUS
----------
	-- create a text item
	dial.status = gui_new_text(dial, { x1, y4, x3, y3 }, nil, {x="left"})

--------------
-- ARGB INPUT
--------------
	dial.reset_color = color
	dial.argb = gui_new_input(dial, { x4, y6, x3, y5 }, nil,nil,
				color2hex(color))
	set_pickcolor(dial, dial.reset_color);

	return dial
end

colorpicker_loaded = 1

dial = nil
if not nil then
	print("Run test (y/n) ?")
	c = getchar()
	if c == 121 then
		dial = colorpicker("SELECT A COLOR", {0.3,0,0,1})
	end
end
