--- @ingroup dcplaya_lua_colorpicker_gui
--- @file    colorpicker.lua
--- @author  benjamin gerard <ben@sashipa.com>
--- @date    2002/10/11
--- @brief   Colorpicker GUI.
---
--- $Id: colorpicker.lua,v 1.7 2002-12-01 19:19:14 ben Exp $

-- Load required libraries
--
if not dolib("gui") then return end
if not dolib("color") then return end

--- @defgroup dcplaya_lua_colorpicker_gui Colorpicker GUI
--- @ingroup  dcplaya_lua_gui

--- Create a colorpicker dialog.
--- @ingroup dcplaya_lua_colorpicker_gui
--- @param  owner
--- @param  name
--- @param  pos
--- @param  color
---
--- @return dialog application
--- @retval nil Error
---
function colorpicker(owner, name, pos, color)

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

   -- -----------------------
   -- BUTTONS' EVENT HANDLERS
   -- -----------------------
   function but_cancel_handle(but,evt)
	  local dial = but.owner 
	  evt_shutdown_app(dial)
	  return nil
	end

   function but_reset_handle(but,evt)
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
	   
	   if not h then
		  h = 0
	   elseif l then
		  h = h*16 + l
	   end
	   return h/255
	end

	-- convert n [0..1] -> string "xx"
	function n2hex(n)
	   if not n then return "00" end
	   return format("%02X", floor((n*255)))
	end

	function color2hex(color)
	   color = color or color_new()
	   return strupper(n2hex(color[1])..n2hex(color[2])
					   ..n2hex(color[3])..n2hex(color[4]))
	end

	function hex2color(str)
		if not str then str = "" end
		str = strupper(str)
		local l = strlen(str)
		if l < 6 then
			str = strrep("0",6-l)..str
			l = 6
		end
		if l < 8 then
			str = strrep("F",8-l)..str
		end
		return color_new({	hex2n(strsub(str,1,2)),
							hex2n(strsub(str,3,4)),
							hex2n(strsub(str,5,6)),
							hex2n(strsub(str,7,8)) })
	end

-- -------------
-- BUILD LAYOUT
-- -------------
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
	if pos and pos.x then x = pos.x	else x = (screenw-w)/2 end
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
	if pos and pos.y then y = pos.y	else y = (screenh-h)/2-32 end
	y1 = y+bh
	y5 = y1+128
	y6 = y5-bh
	y7 = y6-spany
	y3 = y5
	y2 = y3+bordery
	h  = y2-y
		 
-- -----------
-- MAIN DIALOG
-- -----------

	-- Set alphabar value (y) and refresh it
	--
	function set_alphabar(dial,alpha)
		local abar = dial.alphabar
		if not abar then return end
		alpha = clip_value(alpha,0,1)
		abar.value = alpha
		dial.value[1] = alpha
	end

	-- Refresh alpha-bar
	--
	function refresh_alphabar(dial,alpha)
		local abar = dial.alphabar
		if not abar then return end
		local y = abar.value*128;
		dl_clear(abar.dl)
		dl_draw_box (abar.dl,0,0,16,128,0, 1,0,0,0, 1,1,1,1, 2)
		dl_draw_box1(abar.dl,0,y-0.5,16,y+1.5,1, 1,1,0,0)
	end

	function change_alphabar(dial,value)
		set_alphabar(dial,value)
		refresh_alphabar(dial)
		refresh_colortgt(dial)
		refresh_argb(dial)
	end

	-- Get colorbar value (y) from a color
    --  The color 
	function colorbar_value(cbar, color)
		local scale, i, n = getn(cbar.colors)
		a,b,c = color_sort(color)
		if color[a] < 1 or color[c] > 0 then
			return 0
		end
		local lookup = { 0, 4, 5, 2, 1, 3 }

		local cnta = 2^(a-2)
		local cntb = 2^(b-2)
		local ia=lookup[cnta]
		local ib=lookup[cntb+cnta]
		if (ia==0 and ib==5) then ia=6 end
		if (ia < ib) then
			v = (ia+color[b])/6
		else
			v = (ib+color[b])/6
		end
		return v
	end

	-- Get colorbar
	function colorbar_color(cbar, value)
		local n = getn(cbar.colors)
		local i = floor((n-1)*value)
		if i <  0   then return cbar.colors[1] end
		if i >= n-1 then return cbar.colors[n] end
		local f,o
		f = value * (n-1) - i
		o = 1-f
		return (cbar.colors[i+1] * o) + (cbar.colors[i+2] * f)
	end

	function set_colorbar(dial,color)
		local cbar = dial.colorbar
		if not cbar then return end
		local value
		if type(color) == "table" then
			value = colorbar_value(cbar,color)
		else
			value = color
			color = colorbar_color(cbar, value)
		end
		cbar.value = clip_value(value,0,1)
		color_copy(cbar.color,color)
	end

	function refresh_colorbar(dial)
		local cbar = dial.colorbar
		if not cbar then return end

		dl_clear(cbar.dl)
		local n = gcbar
		local y=0
		local ys=128/6
		local col = cbar.colors
		for i=1, 6, 1 do
			dl_draw_box (cbar.dl,0,y,16,y+ys,0,
						1,col[i  ][2],col[i  ][3],col[i  ][4],
						1,col[i+1][2],col[i+1][3],col[i+1][4],
						2)
			y = y + ys
		end
		
		col = color_distant(cbar.color)
		y = cbar.value*128;
		dl_draw_box1(cbar.dl,0,y-0.5,16,y+1.5,1,
			1,col[2],col[3],col[4])
	end

	function change_colorbar(dial,value)
		set_colorbar(dial,value)
		set_colorbox_corner(dial,dial.colorbar.color)
		refresh_colorbar(dial)
		refresh_colorbox(dial)
		refresh_colortgt(dial)
		refresh_argb(dial)
	end


-- QUADRATIC INTERPOLATION
-- 
--  AB
--  CD
-- 
-- (AB) = (1-X)*A + X*B
-- (CD) = (1-X)*C + X*D
-- 
-- (ABCD) = (1-Y)*(AB) + Y*(CD)
-- (ABCD) = (1-Y)*((1-X)*A+X*B) + Y*((1-X)*C+X*D)
-- (ABCD) = (1-Y)*(1-X)*A + (1-Y)*X*B + Y*(1-X)*C + Y*X*D
-- (ABCD) = (1-Y-X+XY)*A + (X-XY)*B + (Y-XY)*C + XY*D
-- 
-- (001D) = [Y-XY] + XY*[D]
-- (001D) = XY ( [1/X-1] + [D] )
-- 
-- [Z] = [(Y-XY)] + XY*[D]

-- [Z1 Z2 Z3] = [(Y-XY)] + XY*[1 F 0]

-- Z1 = (Y-XY) + XY * 1
-- Z2 = (Y-XY) + XY * F
-- Z3 = (Y-XY) + XY * 0

-- Z1 = (Y-XY) + XY
-- Z2 = (Y-XY) + XY * F = Y(1-X) + YXF = Y(1-X+XF) = Y(1-X(1+F))
-- Z3 = Y-XY

-- Z1 = Y
-- Z2 = Y(1-X(1+F))
-- Z3 = Y * (1-X)
-- => Y = Z3 / (1-X)
--    X = 1-Z3/Y

-- Z2 = (Y-XY) + XY * F
-- F = (Z2-Y+XY) / XY

	function get_colorbox_xy(dial, color)
		local cbox = dial.colorbox
		if not cbox then return end
		if not color then return cbox.x,cbox.y end
		local max = cbox.value
		local x,y

		a,b,c = color_sort(color)

		y = color[2] + 1 - color[c] - max[2] + max[2] * color[3]
		y = color[a]
		
		x = 1-color[3]/y

		return x,y
	end

	function get_corner(color)
		local x,y
		local corner
		corner = color_new(color)
		color_maximize(corner)
		a,b,c = color_sort(corner)
		
		if corner[c] > 0.9999 then
			return 0, color[a], color_new(1,1,0,0)
		end

		y = color[a]
		x = 1-color[c]/y
		corner[b] = (color[b]-y+x*y)/(x*y)
		corner[c] = 0

		return x,y,corner
	end
		

	-- Set colorbox value (color)
    --  calculates corner color and (x,y) position
	function set_colorbox_value(dial, color)
		local cbox = dial.colorbox
		if not cbox then return end
		local x,y,c
		x,y,c = get_corner(color)
		color_copy(cbox.corner,c)
		color_copy(cbox.restore,color)
		set_colorbox_xy(dial,x,y)
	end

	-- Set colorbox corner
    --  calculates color from new corner with current (x,y) position
	function set_colorbox_corner(dial, color)
		local cbox = dial.colorbox
		if not cbox then return end
		color_copy(cbox.corner,color)
		set_colorbox_xy(dial,cbox.x,cbox.y)
	end

	-- Set (x,y) position
    --  calculates new value
	function set_colorbox_xy(dial, x, y)
		local cbox = dial.colorbox
		if not cbox then return end
		cbox.x = x
		cbox.y = y
		local xy = x*y
		local yxy = y-xy
		local c
		local corner = cbox.corner
		c = {1,yxy,yxy,yxy} + xy * corner
		color_copy(dial.value,c,1)
	end

	-- Redraw colorbox. value, corner and (x,x) must be valid
	--
	function refresh_colorbox(dial)
		local cbox = dial.colorbox
		if not cbox then return end
		local corner = cbox.corner
		c = color_distant(cbox.value,{1,0,0,1})

		dl_clear(cbox.dl)
		dl_draw_box4(cbox.dl,
					0,0,128,128,0,
					1,0,0,0,
					1,0,0,0,
					1,1,1,1,
					1,corner[2],corner[3],corner[4])
		dl_draw_box1(cbox.dl,
					0,128*cbox.y-0.5,128,128*cbox.y+1.5,1,
					1,c[2],c[3],c[4])

		dl_draw_box1(cbox.dl,
					128*cbox.x-0.5,0,128*cbox.x+0.5,128,1,
					1,c[2],c[3],c[4])
	end

	function refresh_colortgt(dial)
		local c = dial.value
		dl_clear(dial.tgtdl)
		dl_draw_box(dial.tgtdl,
				0,0,dial.tgt_w,dial.tgt_h,1,
				c[1],c[2],c[3],c[4],
				1,c[2],c[3],c[4])
	end

	function refresh_argb(dial)
		evt_send(dial.owner,
			{ 	key = gui_color_change_event,
				color = color_new(dial.value) })
		gui_input_set(dial.argb, color2hex(dial.value))
	end
	
	-- Get current color 
	--
	function get_pickcolor(dial)
		local c = {}
		color_copy(c,dial.value)
		return c;
	end

	-- Set picked color from either a string or a color
	--
	function set_pickcolor(dial,color)
		if not dial or not color then return end
		if type(color) == "string" then
			color = hex2color(color)
		end
		color_clip(color)
		color_copy(dial.value,color)
		set_colorbox_value(dial,color)
		set_alphabar(dial,color[1])
		set_colorbar(dial,dial.colorbox.corner)

		refresh_colorbar(dial)
		refresh_colorbox(dial)
		refresh_colortgt(dial)
		refresh_alphabar(dial)
		refresh_argb(dial)
	end

	-- Main dialog event handle function
	function dial_handle(dial,evt)
		local key = evt.key
		if key == gui_input_confirm_event then
			set_pickcolor(dial, dial.argb.input)
			return nil
		end
		return evt
	end

	if not gui_color_change_event then
		gui_color_change_event	= evt_new_code()
	end
	if not name then name="Color picker" end
	color = color_new(color)
	if not owner then owner = evt_desktop_app end
	dial = gui_new_dialog(owner,
		{x, y, x2, y2 }, nil, nil, name, { x = "left", y = "up" } )
	dial.event_table = {
		[gui_input_confirm_event]	= dial_handle
	}
	dial.value = color_new(color)
	dial.reset_color = color_new(color)

-- -----------
-- ALL BUTTONS
-- -----------
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

-- ---------
-- COLOR BOX
-- ---------
	-- Common function for alpha and color bar
	function box_handle(cbox,evt)
		local dial = cbox.owner
		local key = evt.key
		local need_update

		if key == evt_shutdown_event then
			dl_destroy_list(cbox.dl)
			-- I kick this here. Not very clean.
			dl_destroy_list(dial.tgtdl)
			return evt
		end

		local f = cbox.event_table[key]
		if f then return f(cbox, evt) end

		if gui_is_focus(dial, cbox) then
			local newx = cbox.x
			local newy = cbox.y
			if gui_keyleft[key] then
				newx = clip_value(newx-(1/255),0,1)
				evt = nil
			elseif gui_keyright[key] then
				newx = clip_value(newx+(1/255),0,1)
				evt = nil
			elseif gui_keydown[key] then
				newy = clip_value(newy+(1/255),0,1)
				evt = nil
			elseif gui_keyup[key] then
				newy = clip_value(newy-(1/255),0,1)
				evt = nil
			elseif gui_keyconfirm[key] then
				color_copy(cbox.restore,dial.value)
				gui_new_focus(dial, dial.argb)
				evt = nil
			elseif gui_keycancel[key] then
				set_pickcolor(dial,cbox.restore)
				gui_new_focus(dial, dial.argb)
				return nil
			elseif key == 9 then --KEY_TAB then
				return { key = KBD_KEY_RIGHT }
			end

			if newx ~= cbox.x or newy ~= cbox.y then
				set_colorbox_xy(dial, newx, newy)
				need_update = 1
			end

			if need_update then
				refresh_colorbox(dial)
				refresh_colortgt(dial)
				refresh_argb(dial)
			end
		end
		return evt
	end

	dial.colorbox = gui_new_children(dial, "color_box", box_handle,
									{x1,y1,x9,y5})
	dl_set_trans(dial.colorbox.dl, mat_trans(x1,y1,dial.colorbox.z))
	dl_set_active(dial.colorbox.dl,1)
	dial.colorbox.value  = dial.value
	dial.colorbox.restore = color_new()
	dial.colorbox.corner = color_new()
	dial.colorbox.x = 0
	dial.colorbox.y = 0

-- ------------
-- TARGET COLOR
-- ------------
	dial.tgt_w = x3-x4
	dial.tgt_h = y7-y8
	dial.tgtdl = dl_new_list(128)
	dl_set_trans(dial.tgtdl,mat_trans(x4,y8,dial.colorbox.z+1))
	dl_set_active(dial.tgtdl,1)

-- ---------
-- COLOR BAR
-- ---------

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
				newvalue = clip_value(newvalue+(1/255),0,1)
				evt = nil
			elseif gui_keyup[key] then
				newvalue = clip_value(newvalue-(1/255),0,1)
				evt = nil
			end
			if newvalue ~= app.value then
				app.set_value(dial,newvalue)
			end
		end
		return evt
	end

	dial.colorbar = gui_new_children(dial, "color_bar", bar_handle,
									{x8,y1,x7,y5})
	dl_set_trans(dial.colorbar.dl, mat_trans(x8,y1,dial.colorbox.z))
	dl_set_active(dial.colorbar.dl,1)
	dial.colorbar.set_value = change_colorbar
	dial.colorbar.value = 0
	dial.colorbar.color = color_new()
	dial.colorbar.colors = {
			color_new(1,1,0,0), color_new(1,1,0,1),
			color_new(1,0,0,1),	color_new(1,0,1,1),
			color_new(1,0,1,0),	color_new(1,1,1,0),
			color_new(1,1,0,0) 
		}

-- ---------
-- ALPHA BAR
-- ---------
	dial.alphabar = gui_new_children(dial, "alpha_bar", bar_handle,
									{x6,y1,x5,y5})
	dl_set_trans(dial.alphabar.dl, mat_trans(x6,y1,dial.colorbox.z))
	dl_set_active(dial.alphabar.dl,1)
	dial.alphabar.set_value = change_alphabar
	dial.alphabar.value = 0

-- ----------
-- ARGB INPUT
-- ----------
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
		dial = colorpicker(nil, "SELECT A COLOR", nil, {0.3,0,0,1})
		if dial and fftvlr_setdirectionnal then

		fftvlr_setbordertex(1)
		fftvlr_setambient(0,0,0,0.5)

		dial2 = gui_new_dialog(evt_desktop_app,
			{0, 0, 100, 100 }, nil, nil, "test", { x = "left", y = "up" } )
		dial2.event_table = {
		[gui_color_change_event] =
			function(app,evt)
				local c = evt.color
				if c then
					fftvlr_setdirectionnal(c[2],c[3],c[4],c[1])
				end
				return
			end
		}
		end 
	end
end

function kill()
	if dial then evt_shutdown_app(dial) dial = nil end
	if dial2 then evt_shutdown_app(dial2) dial2 = nil end
end

return colorpicker_loaded
