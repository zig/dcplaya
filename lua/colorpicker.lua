--- colorpicker.lua
-- 
-- author : benjamin gerard <ben@sashipa.com>
-- date   : 2002/10/11
--
-- $Id: colorpicker.lua,v 1.2 2002-10-12 09:40:12 benjihan Exp $
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
-- | |                   | |  | |  | +------+ |<Y8
-- | |                   | |  | |  | +TGT-DL| |
-- | |                   | |  | |  | +------+ |<Y7
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

--		print(h)
--		print(l)
		if not h then
			h = 0
		elseif l then
			h = h*16 + l
		end
--		print(format("%s -> %02f",str,h))
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
	spanx=6
	spany=6
	bw=76
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

	local y,y1,y2,y3,y4,y5,y6,y7,y8
	y  = (screenh-h)/2-32
	y1 = y+bh
	y5 = y1+128
	y6 = y5-bh
	y7 = y6-spany
	y4 = y5+spany
	y3 = y4+bh
	y2 = y3+bordery
	h  = y2-y
		 
---------------
-- MAIN DIALOG
---------------

	-- Set status text
	function status(dial,text)
		if dial.status then
			gui_text_set(dial.status,text)
		end
	end

	function clip_value(v)
		if v < 0 then return 0 end
		if v > 1 then return 1 end
		return v
	end

	function copy_color(d,s,noalpha)
		if not noalpha then d[1] = s[1] end
		d[2] = s[2]
		d[3] = s[3]
		d[4] = s[4]
	end

	function clip_color(color)
		color[1] = clip_value(color[1])
		color[2] = clip_value(color[2])
		color[3] = clip_value(color[3])
		color[4] = clip_value(color[4])
	end

	function sort_color(color)
		local order = { 2, 3, 4 }
		local i,j
		for i=1, 2, 1 do
			local tmp = order[i]
			for j=i+1, 3, 1 do
				if color[order[j]] > color[tmp] then
					order[i] = order[j]
					order[j] = tmp
					tmp = order[i]
				end
			end
		end
		return order[1],order[2],order[3]
	end

	function maximize_color(color)
		local max = 2
		if color[3] > color[max] then max = 3 end
		if color[4] > color[max] then max = 4 end
		if color[max] ~= 0 then
			local scale=1/color[max]
			color[1] = 1
			color[2] = color[2]*scale
			color[3] = color[3]*scale
			color[4] = color[4]*scale
		end
	end

	function colorbar_value(cbar, color)
		local scale, i, n = getn(cbar.colors)

		maximize_color(color)
		a,b,c = sort_color(color)

		print (format("colorbar: %.3f %.3f %.3f", color[2],color[3],color[4]))
		print (format("order: %d %d %d", a,b,c))
		
		if color[a] ~= 1 or color[c] == 1 then
			print("Black or White")
			copy_color(color,cbar.colors[1])
			return 0
		end
		color[c] = 0

--			{1,0,0}, 0
--			{1,0,1}, 1
--			{0,0,1}, 2
--			{0,1,1}, 3
--			{0,1,0}, 4
--			{1,1,0}, 5
--			{1,0,0}, 6
--                       1  2  3  4  5  6 
		local lookup = { 0, 4, 5, 2, 1, 3 }

		local cnta = 2^(a-2)
		local cntb = 2^(b-2)
		if color[b] == 1 then
			print(format("bi-max:%d",cnta+cntb))
			return lookup[cnta+cntb]/6
		end
		print(format("cnta=%d, cntb=%d a+b=%d ",cnta,cntb, cnta+cntb))
		local ia=lookup[cnta]
		local ib=lookup[cntb+cnta]
		if (ia==0 and ib==5) then ia=6 end
		print(format("idx:a:%d b:%d",ia,ib))

		if (ia < ib) then
			v = ia/6 + color[b]/6
		else
			v = ib/6 + color[b]/6
		end

		print("colorbar_value->"..v)
		return v
	end

	function colorbar_color(cbar, value)
		local n = getn(cbar.colors)
		local i = floor((n-1)*value)
		if i <  0   then return cbar.colors[1] end
		if i >= n-1 then return cbar.colors[n] end
		local f,o
		f = value * (n-1) - i
		o = 1-f
--		print(format("colorbar_color(%0.3f) -> i:%.2f f:%.3f",value,i,f))
		return (cbar.colors[i+1] * {o,o,o,o}) + (cbar.colors[i+2] * {f,f,f,f})
	end		

	function set_colorbar(dial,value)
		local cbar = dial.colorbar
		if not cbar then return end
		if value < 0 then value = 0 elseif value > 1 then value = 1 end
--		print("set color: "..value)
		cbar.value = value
		local color = colorbar_color(cbar,value)
		set_colorbox(dial,color)

		dl_clear(cbar.dl)
		local n = gcbar
		local y=0
		local ys=128/6
		local col = cbar.colors
		for i=1, 6, 1 do
			dl_draw_box (cbar.dl,0,y,16,y+ys,0,
						col[i  ][1],col[i  ][2],col[i  ][3],col[i  ][4],
						col[i+1][1],col[i+1][2],col[i+1][3],col[i+1][4],
						2)
			y = y + ys
		end
		y = value*128;
		dl_draw_box1(cbar.dl,0,y-0.5,16,y+1.5,1,
				1, 0, 0, 0)
--				1, 1-color[2], 1-color[3], 1-color[4])
	end

	function set_alphabar(dial,alpha)
		local abar = dial.alphabar
		if not abar then return end
		if alpha < 0 then alpha = 0 elseif alpha > 1 then alpha = 1 end
--		print("set alpha: "..alpha)
		abar.value = alpha
		dial.value[1] = alpha
		local y = alpha*128;
		dl_clear(abar.dl)
		dl_draw_box (abar.dl,0,0,16,128,0, 1,0,0,0, 1,1,1,1, 2)
		dl_draw_box1(abar.dl,0,y-0.5,16,y+1.5,1, 1,1,0,0)
	end

	function get_colorbox_xy(dial, color)
		local cbox = dial.colorbox
		if not cbox then return end
		if not color then return cbox.x,cbox.y end
		local max = cbox.value
		local x
		local i=2
		if max[3] > max[i] then i = 3 end
		if max[4] > max[i] then i = 4 end
		x = color[i]/max[i]
		return x,x
	end

	function set_colorbox(dial, max, color)
		local cbox = dial.colorbox
		if not cbox then return end
		if not cbox.value then cbox.value = {0,0,0,0} end
		local col = cbox.value
		copy_color(col,max)
		
		local x,y
-- $$$ That functions is not good !
		x,y = get_colorbox_xy(dial, color)
		set_colorbox_xy(dial,x,y)
	end

	function set_colorbox_xy(dial, x, y)
		local cbox = dial.colorbox
		if not cbox then return end
		cbox.x = x
		cbox.y = y

		local c
		local col = cbox.value
		local yyy = {1,y,y,y}

		if abs(x-y)<0.00001 then
			c = col * yyy
		elseif x < y then
			local r = x/y
			c = yyy + ((col*yyy)-yyy) * {1,r,r,r}
		else
			c = col*yyy
		end

		copy_color(dial.value,c,1)
		dl_clear(cbox.dl)
		dl_draw_box4(cbox.dl,
					0,0,128,128,0,
					1,0,0,0,
					1,0,0,0,
					1,1,1,1,
					1,col[2],col[3],col[4])
		dl_draw_box1(cbox.dl,
					0,128*cbox.y-0.5,128,128*cbox.y+1.5,1,
					1,c[2],c[3],c[4])
		dl_draw_box1(cbox.dl,
					128*cbox.x-0.5,0,128*cbox.x+0.5,128,1,
					1,c[2],c[3],c[4])
	end

	function refresh_argb(dial)
		local c = dial.value
		dl_clear(dial.colorbox.tgtdl)
		dl_draw_box(dial.colorbox.tgtdl,
				0,0,dial.colorbox.tgt_w,dial.colorbox.tgt_h,1,
				c[1],c[2],c[3],c[4],
				1,c[2],c[3],c[4])
		gui_input_set(dial.argb, color2hex(dial.value))
	end
	
	-- Get color from argb input
	function get_pickcolor(dial)
		return hex2color(dial.argb.input)
	end

	-- Set picked color
	function set_pickcolor(dial,color)
		if not dial or not color then return end
		if type(color) == "string" then
			color = hex2color(color)
		end
		clip_color(color)
		copy_color(dial.value,color)

		local scale
		local colormax={}
		copy_color(colormax,color)
		local cbarv = colorbar_value(dial.colorbar, colormax)
		print(format("new colormax: %.3f %.3f %.3f",colormax[2],colormax[3],colormax[4]))

		print(format("CBAR=%.3f",cbarv))
		set_colorbar(dial,cbarv)

		print(format(" color: %.3f %.3f %.3f",color[2],color[3],color[4]))

		set_colorbox(dial, colormax, color)
		set_alphabar(dial,color[1])
		refresh_argb(dial)
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

	dial.value = { 0,0,0,0 }
	dial.reset_color = color

---------------
-- ALL BUTTONS
---------------
	function mkbutton(p)
		local but
		but = gui_new_button(%dial, p.box, p.name)
	  	but.event_table[gui_press_event] = p.handle
	end

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
	y8 = yb

-------------
-- COLOR BOX
-------------
	-- Common function for alpha and color bar
	function box_handle(app,evt)
		local dial = app.owner
		local key = evt.key

		if key == evt_shutdown_event then
			dl_destroy_list(app.dl)
			dl_destroy_list(app.tgtdl)
			return evt
		end

		local f = app.event_table[key]
		if f then return f(app, evt) end

		if gui_is_focus(dial, app) then
			local newx = app.x
			local newy = app.y
			if gui_keyleft[key] then
				newx = clip_value(newx-(1/255))
				evt = nil
			elseif gui_keyright[key] then
				newx = clip_value(newx+(1/255))
				evt = nil
			elseif gui_keydown[key] then
				newy = clip_value(newy+(1/255))
				evt = nil
			elseif gui_keyup[key] then
				newy = clip_value(newy-(1/255))
				evt = nil
			elseif gui_keyconfirm[key] or gui_keycancel[key] then
				gui_new_focus(dial, dial.argb)
				evt = nil
			elseif key == 9 then --KEY_TAB then
				return { key = KBD_KEY_RIGHT }
			end
			if newx ~= app.x or newy ~= app.y then
				set_colorbox_xy(dial, newx, newy)
				refresh_argb(dial)
			end
		end
		return evt
	end

	dial.colorbox = gui_new_children(dial, "color_box", box_handle,
									{x1,y1,x9,y5})
	dl_set_trans(dial.colorbox.dl, mat_trans(x1,y1,dial.colorbox.z))
	dl_set_active(dial.colorbox.dl,1)
	
	dial.colorbox.value = { 0,0,0,0 }
	dial.colorbox.x = 0
	dial.colorbox.y = 0
	dial.colorbox.tgt_w = x3-x4
	dial.colorbox.tgt_h = y7-y8
	dial.colorbox.tgtdl = dl_new_list(128)
	dl_set_trans(dial.colorbox.tgtdl,mat_trans(x4,y8,dial.colorbox.z+1))
	dl_set_active(dial.colorbox.tgtdl,1)

-------------
-- COLOR BAR
-------------

	-- Common function for alpha and color bar
	function bar_handle(app,evt)
		local dial = app.owner
		local key = evt.key

		if key == evt_shutdown_event then
			dl_destroy_list(app.dl)
			return evt
		end

		local f = app.event_table[key]
		if f then return f(app, evt) end

		if gui_is_focus(dial, app) then
			local newvalue = app.value
			if gui_keydown[key] then
				newvalue = clip_value(newvalue+(1/255))
				evt = nil
			elseif gui_keyup[key] then
				newvalue = clip_value(newvalue-(1/255))
				evt = nil
			end
			if newvalue ~= app.value then
				app.set_value(dial,newvalue)
				refresh_argb(dial)
			end
		end
		return evt
	end

	dial.colorbar = gui_new_children(dial, "color_bar", bar_handle,
									{x8,y1,x7,y5})
	dl_set_trans(dial.colorbar.dl, mat_trans(x8,y1,dial.colorbox.z))
	dl_set_active(dial.colorbar.dl,1)
	dial.colorbar.set_value = set_colorbar
	dial.colorbar.value = 0
	dial.colorbar.colors = {
			{1,1,0,0},  {1,1,0,1},	{1,0,0,1},	{1,0,1,1},
			{1,0,1,0},	{1,1,1,0},	{1,1,0,0}
		}

-------------
-- ALPHA BAR
-------------
	dial.alphabar = gui_new_children(dial, "alpha_bar", bar_handle,
									{x6,y1,x5,y5})
	dl_set_trans(dial.alphabar.dl, mat_trans(x6,y1,dial.colorbox.z))
	dl_set_active(dial.alphabar.dl,1)
	dial.alphabar.set_value = set_alphabar
	dial.alphabar.value = 0

----------
-- STATUS
----------
	-- create a text item
	dial.status = gui_new_text(dial, { x1, y4, x3, y3 }, nil, {x="left"})

--------------
-- ARGB INPUT
--------------
	dial.argb = gui_new_input(dial, { x4, y6, x3, y5 }, nil,nil,nil)

	set_pickcolor(dial, dial.reset_color)

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


