--
-- keyboard emulation lua module
--
-- author : Vincent Penne
--
-- $Id: keyboard_emu.lua,v 1.2 2002-09-27 06:33:54 vincentp Exp $
--


if not keydefs_included then
	dofile(home.."lua/keydefs.lua")
end
if not init_display_driver then
	dofile(home.."lua/display_init.lua")
end

ke_box 		= { 20, 330, 640-20, 330+140 }
ke_z		= 150
ke_cursorz	= ke_z + 20
ke_textz	= ke_z + 30

if not ke_theme then
	ke_theme = 2	-- currently 1 .. 3
end

ke_themes = {
	function()
	ke_boxcolor	= { 0.8, 0.7, 0.7, 0.2 }
	ke_keycolor1	= { 0.8, 0.2, 0.2, 0.7 } 
	ke_keycolor2	= ke_keycolor1
	ke_textcolor	= { 1.0, 0.2, 0.2, 0.2 } 
	end,

	function()
	ke_boxcolor	= { 0.4, 0.5, 1.0, 0.8 }
	ke_keycolor1	= { 0.6, 0.2, 0.7, 0.7 } 
	ke_keycolor2	= { 0.6, 0.2, 0.2, 0.4 } 
	ke_textcolor	= { 1.0, 0.9, 0.9, 0.2 } 
	ke_textcolor	= { 1.0, 0.2, 0, 0 } 
	end,

	function()
	ke_boxcolor	= { 0.4, 0.5, 1.0, 0.8 }
	ke_keycolor1	= { 0.8, 0.2, 0.2, 0.7 } 
	ke_keycolor2	= { 0.8, 0.2, 0.2, 0.7 } 
	ke_textcolor	= { 1.0, 0.9, 0.9, 0.2 } 
	end
}

ke_themes[ke_theme]()

ke_keyup	= { [KBD_CONT1_DPAD2_UP]=1 }
ke_keydown	= { [KBD_CONT1_DPAD2_DOWN]=1 }
ke_keyleft	= { [KBD_CONT1_DPAD2_LEFT]=1 }
ke_keyright	= { [KBD_CONT1_DPAD2_RIGHT]=1 }
ke_keyselect	= { [KBD_CONT1_X]=1 }
ke_keyconfirm	= { [KBD_CONT1_A]=1 }
ke_keycancel	= { [KBD_CONT1_B]=1 }
ke_keynext	= { [KBD_CONT1_C]=1 }
ke_keyprev	= { [KBD_CONT1_D]=1 }
ke_keyactivate	= { [KBD_CONT1_START]=1 }

ke_translate	= { 
	[KBD_CONT1_DPAD_UP] = KBD_KEY_UP,
	[KBD_CONT1_DPAD_DOWN] = KBD_KEY_DOWN,
	[KBD_CONT1_DPAD_LEFT] = KBD_KEY_LEFT,
	[KBD_CONT1_DPAD_RIGHT] = KBD_KEY_RIGHT
}

function table_sqrdist(t1, t2)
	local sum = 0
	local i, v
	for i, v in t1 do
		local d = v - t2[i]
		sum = sum + d*d
	end
	
	return sum
end

function table_add(t1, t2)
	local r = {}
	local i, v
	for i, v in t1 do
		r[i] = v + t2[i]
	end
	
	return r
end

function table_sub(t1, t2)
	local r = {}
	local i, v
	for i, v in t1 do
		r[i] = v - t2[i]
	end
	
	return r
end

function table_mul(a, t)
	local r = {}
	local i, v
	if type(a) == "table" then
		a, t = t, a
	end
	if type(a) == "table" then
		-- two tables case
		for i, v in a do
			r[i] = v * t[i]
		end
	else
		-- number * table case
		for i, v in t do
			r[i] = v * a
		end
	end
	
	return r
end

function ke_addkeypos(x, y)
	if not ke_addkeycurpos then
		ke_addkeycurpos = {}
	end
	if x > 0 then
		ke_addkeycurpos[1] = ke_box[1] + x
	else
		ke_addkeycurpos[1] = ke_addkeycurpos[1] - x
	end
	if y > 0 then
		ke_addkeycurpos[2] = ke_box[2] + y
	else
		ke_addkeycurpos[2] = ke_addkeycurpos[2] - y
	end
end

debug = nil
if debug then
	function dl_draw_box(dl, ...)
		call(print, list_expand(arg))
	end
end

function ke_addsinglekey(text, code, dl, list, spacing)
	if not code then
		if strlen(text) > 1 then
			code = getglobal("KBD_KEY_"..strupper(text))
			if not code then
				print ("could not find key", "KBD_KEY_"..strupper(text))
			end
		else
			code = strbyte(text, 1)
		end
	end

	local w, h, w2
	local key = {}
	w, h = dl_measure_text(dl, text)
	w = w+6
	w2 = max(w, 18+6)
	if spacing then 
		w2 = w2 + spacing
	end
	h = h+6

	key.box = { ke_addkeycurpos[1], ke_addkeycurpos[2], ke_addkeycurpos[1] + w2, ke_addkeycurpos[2] + h }
	key.text = text
	key.code =  code

	dl_draw_box(dl, key.box, ke_z+10, ke_keycolor1, ke_keycolor2)
	dl_draw_text(dl, key.box[1]+3+(w2-w)/2, key.box[2]+3, ke_textz, ke_textcolor, key.text)
	tinsert(list, key)

	return w2, h

end

function ke_addkey(down, downcode, up, upcode, spacing)
	if not up then
		if strlen(down) == 1 then
			up = strupper(down)
		else
			up = down
		end
		upcode = downcode
	end

	local w1, w2, h1, h2
	w1, h1 = ke_addsinglekey(down, downcode, ke_downarray_dl, ke_downarray, spacing)
	w2, h2 = ke_addsinglekey(up, upcode, ke_uparray_dl, ke_uparray, spacing)

	ke_addkeycurpos = ke_addkeycurpos + { max(w1, w2) + 5, 0 }
--	call (print, ke_addkeycurpos)

end

function ke_shutdown()
	if ke_arrays then
		local array
		for _, array in ke_arrays do
			if type(array)=="table" then
				dl_destroy_list(array.dl)
			end
		end
		ke_arrays = { }
	end
	if ke_cursor_dl then
		dl_destroy_list(ke_cursor_dl)
		ke_cursor_dl = nil
	end
	ke_arrays = nil

	if ke_origgetchar then
		getchar = ke_origgetchar
		peekchar = ke_origpeekchar
		framecounter = ke_origframecounter
	end
end

function ke_set_keynum(n)
	ke_keynum = n
	ke_key = ke_array[n]
end

ke_closest_coef = { 1, 1, 1, 1 }
ke_closest_vertical_coef = { 1, 5, 1, 5 }
function ke_closest_key(array, box, coef)

	if not coef then
		coef = ke_closest_coef
	end
	box = box * coef
	local i
	local imin = 1
	local min = array[1].box ^ box
	for i=2, array.n, 1 do
		local k = array[i]
		local b = k.box * coef
		local d = b ^ box
		if d < min then
			imin = i
			min = d
		end
	end
	
	return imin
end

function ke_set_active_array(n)
	dl_set_active(ke_arrays[ke_arraynum].dl, 0)
	ke_arraynum = n
	ke_array = ke_arrays[n]
	dl_set_active(ke_arrays[ke_arraynum].dl, 1)
	ke_set_keynum(ke_closest_key(ke_array, ke_cursorbox))
end


function ke_framecounter()
	return ke_curframecounter
end

function ke_handle(frametime, key)
	if ke_keynext[key] then
		local n = ke_arraynum+1
		if n > ke_arrays.n then
			n = 1
		end
		ke_set_active_array(n)

		return nil
	end

	if ke_keyprev[key] then
		local n = ke_arraynum-1
		if n < 1 then
			n = ke_arrays.n
		end
		ke_set_active_array(n)

		return nil
	end

	if ke_keyright[key] then
		local n = ke_keynum+1
		if n > ke_array.n then
			n = 1
		end
		ke_set_keynum(n)

		return nil
	end

	if ke_keyleft[key] then
		local n = ke_keynum-1
		if n < 1 then
			n = ke_array.n
		end
		ke_set_keynum(n)

		return nil
	end

	if ke_keyup[key] then
		local box = ke_cursorbox - { 0, 25, 0, 25 }
		ke_set_keynum(ke_closest_key(ke_array, box, ke_closest_vertical_coef))

		return nil
	end

	if ke_keydown[key] then
		local box = ke_cursorbox + { 0, 25, 0, 25 }
		ke_set_keynum(ke_closest_key(ke_array, box, ke_closest_vertical_coef))

		return nil
	end

	if ke_keyselect[key] then
		return ke_key.code
	end

	if ke_keyconfirm[key] then
		return KBD_ENTER
	end

	if ke_keycancel[key] then
		return KBD_ESCAPE
	end

	local trans = ke_translate[key]
	if trans then
		return trans
	end

	return key
end

function ke_set_active(s)
	ke_active = s
	cond_connect(not s)
	dl_set_active(ke_array.dl, s)
	dl_set_active(ke_cursor_dl, s)
end

function ke_update(key)

	ke_curframecounter = ke_origframecounter(1)

	if key and ke_keyactivate[key] then
		ke_set_active(not ke_active)
		return nil
	end

	if not ke_active then
		return key
	end

	local frametime = ke_curframecounter/60
	ke_time = ke_time + frametime
--	rp (frametime .. "\n")
	if frametime > 0.1 then
		frametime = 0.1
	end

	-- do collect garbage once per frame for smoother animation
	collectgarbage()

	dl_set_trans(ke_cursor_dl, mat_scale(ke_cursorbox[3] - ke_cursorbox[1], ke_cursorbox[4] - ke_cursorbox[2], 1) * mat_trans(ke_cursorbox[1], ke_cursorbox[2], 0))
	dl_set_color(ke_cursor_dl, 0.5+0.5*sin(360*ke_time*2), 1, 1, 1)

	if key then
		key = ke_handle(frametime, key)
	end

	-- update cursor position
	ke_cursorbox = ke_cursorbox + 
			20 * frametime * (ke_key.box - ke_cursorbox)

	return key
end

function ke_peekchar()
	local key

	key = ke_origpeekchar()

	key = ke_update(key)

	return key
end

function ke_getchar()
	local key
	repeat
		key = ke_peekchar()
	until key

	return key
end

function ke_init()

	ke_shutdown()

	if not ke_origgetchar then
		-- get original getchar and peekchar
		-- the keyboard emulator will replace these two variables later
		ke_origgetchar = getchar
		ke_origpeekchar = peekchar
		ke_origframecounter = framecounter
	end

	ke_time = 0

	ke_active = 1

	settagmethod(tag( {} ), "add", table_add)
	settagmethod(tag( {} ), "sub", table_sub)
	settagmethod(tag( {} ), "mul", table_mul)
	settagmethod(tag( {} ), "pow", table_sqrdist)

	if not check_display_driver or not check_display_driver() then
		return
	end

	ke_uparray_dl	= dl_new_list(10*1024, 0)
	ke_downarray_dl	= dl_new_list(10*1024, 0)
	ke_cursor_dl	= dl_new_list(1*1024, 1)

	dl_clear(ke_cursor_dl)
	dl_draw_box(ke_cursor_dl, 0, 0, 1, 1, ke_cursorz, 1, 1, 1, 1, 1, 1, 1, 1)
	ke_cursorbox = ke_box

	local dl
	for _, dl in { ke_downarray_dl, ke_uparray_dl} do

		dl_clear(dl)
		dl_draw_box(dl, ke_box, ke_z, ke_boxcolor, ke_boxcolor)
	
	end

	ke_arrays = { }
	ke_downarray = { dl = ke_downarray_dl }
	tinsert(ke_arrays, ke_downarray)
	ke_uparray = { dl = ke_uparray_dl }
	tinsert(ke_arrays, ke_uparray)

	ke_addkeypos(5, 5)
	ke_addkey("ESC", 33)
	ke_addkey("F1")
	ke_addkey("F2")
	ke_addkey("F3")
	ke_addkey("F4")
	ke_addkey("F5")
	ke_addkey("F6")
	ke_addkey("F7")
	ke_addkey("F8")
	ke_addkey("F9")
	ke_addkey("F10")
	ke_addkey("F11")
	ke_addkey("F12")
	ke_addkey("Print")
	ke_addkey("ScrLock")
	ke_addkey("Pause")

	ke_addkeypos(5, -25)
	ke_addkey("`", nil, "~")
	ke_addkey("1", nil, "!")
	ke_addkey("2", nil, "@")
	ke_addkey("3", nil, "#")
	ke_addkey("4", nil, "$")
	ke_addkey("5", nil, "%")
	ke_addkey("6", nil, "^")
	ke_addkey("7", nil, "&")
	ke_addkey("8", nil, "*")
	ke_addkey("9", nil, "(")
	ke_addkey("0", nil, ")")
	ke_addkey("-", nil, "_")
	ke_addkey("=", nil, "+")
	ke_addkey("Backspace", KBD_BACKSPACE)

	ke_addkeypos(505, 0)
	ke_addkey("Del")
	ke_addkey("Insert")

	ke_addkeypos(5, -25)
	ke_addkey("Tab", nil, nil, nil, 20)
	ke_addkey("q")
	ke_addkey("w")
	ke_addkey("e")
	ke_addkey("r")
	ke_addkey("t")
	ke_addkey("y")
	ke_addkey("u")
	ke_addkey("i")
	ke_addkey("o")
	ke_addkey("p")
	ke_addkey("[", nil, "{")
	ke_addkey("]", nil, "}")
	ke_addkey("\\", nil, "|")

	ke_addkeypos(490, 0)
	ke_addkey("End")
	ke_addkey("PgDown")


	ke_addkeypos(5, -25)
	ke_addkey("CapsLock")
	ke_addkey("a")
	ke_addkey("s")
	ke_addkey("d")
	ke_addkey("f")
	ke_addkey("g")
	ke_addkey("h")
	ke_addkey("j")
	ke_addkey("k")
	ke_addkey("l")
	ke_addkey(";", nil , ":")
	ke_addkey("'", nil, "\"")
	ke_addkey("Enter", KBD_ENTER)

	ke_addkeypos(495, 0)
	ke_addkey("Home")
	ke_addkey("PgUp")

	ke_addkeypos(5, -25)
	ke_addkey("", 32, nil, nil, 80)
	ke_addkey("z")
	ke_addkey("x")
	ke_addkey("c")
	ke_addkey("v")
	ke_addkey("b")
	ke_addkey("n")
	ke_addkey("m")
	ke_addkey(",", nil, "<")
	ke_addkey(".", nil, ">")
	ke_addkey("/", nil, "?")

	ke_addkeypos(420, 0)
	ke_addkey("Up")
	ke_addkey("Left")
	ke_addkey("Right")
	ke_addkey("Down")

	ke_arraynum = 1 -- set it to any valid value for start ...
	ke_set_active_array(1)
	ke_set_keynum(1)

	ke_set_active(1)

	-- last step : replace getchar and cie
	getchar = ke_getchar
	peekchar = ke_peekchar
	framecounter = ke_framecounter

	return 1
end

function keyboard_emu()

	print [[keyboard_emu started ...]]

	if not ke_init() then
		return
	end

end

keyboard_emu()

