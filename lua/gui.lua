--
-- gui lua library on top of evt system
--
-- author : Vincent Penne
--
-- $Id: gui.lua,v 1.3 2002-09-30 02:51:15 vincentp Exp $
--

--
-- a gui item is an application (see in evt.lua for the definition) with
-- additionnal informations :
--
-- box : the box of the item { x1, y1, x2, y2 } used for focusing
-- dl  : the default display list used to draw inside
-- z   : the z position of the item for reference to draw upon it
-- event_table : table of function(app, evt) indexed by event ID
-- flags : an array whose entry describe some functionalities
--

-- flags can have :
--
-- "modal"    : the item (usually a dialog box) eat all events except shutdown
--              (can be dangerous !)
-- "inactive" : the item cannot be focused


-- IDEAS :
--
-- * Auto layout of items based on a generalized text justifying algorithm
--
-- * A general gui_ask function to build easy choose dialog boxes
--
-- * A text input item
--
-- * A browser item
--
-- * A valuator item
--
-- * Allowing nested display list, every items could use its own display list,
--   contained into the parent's display list,
--   focusing would then be able to modify color/placement of items 
--   individually with respect to its focused or active state ...
--

gui_box_color1 = { 0.8, 0.2, 0.3, 0.3 }
gui_box_color2 = { 0.8, 0.1, 0.1, 0.1 }
gui_button_color1 = { 0.8, 0.2, 0.7, 0.7 }
gui_button_color2 = { 0.8, 0.1, 0.3, 0.3 }
gui_text_color = { 0.9, 1.0, 1.0, 0.7 }
gui_text_shiftbox = { 5, 5, -5, -5 }
gui_input_cursor_color1 = { 1, 1, 0.5, 0 }
gui_input_cursor_color2 = { 1, 0.5, 1.0, 0 }

gui_keyup = { 
	[KBD_KEY_UP] = 1, 
	[KBD_CONT1_DPAD_UP] = 1, [KBD_CONT1_DPAD2_UP] = 1, 
	[KBD_CONT2_DPAD_UP] = 1, [KBD_CONT2_DPAD2_UP] = 1, 
	[KBD_CONT3_DPAD_UP] = 1, [KBD_CONT3_DPAD2_UP] = 1, 
	[KBD_CONT4_DPAD_UP] = 1, [KBD_CONT4_DPAD2_UP] = 1
}
gui_keydown = { 
	[KBD_KEY_DOWN] = 1, 
	[KBD_CONT1_DPAD_DOWN] = 1, [KBD_CONT1_DPAD2_DOWN] = 1, 
	[KBD_CONT2_DPAD_DOWN] = 1, [KBD_CONT2_DPAD2_DOWN] = 1, 
	[KBD_CONT3_DPAD_DOWN] = 1, [KBD_CONT3_DPAD2_DOWN] = 1, 
	[KBD_CONT4_DPAD_DOWN] = 1, [KBD_CONT4_DPAD2_DOWN] = 1
}
gui_keyleft = { 
	[KBD_KEY_LEFT] = 1, 
	[KBD_CONT1_DPAD_LEFT] = 1, [KBD_CONT1_DPAD2_LEFT] = 1, 
	[KBD_CONT2_DPAD_LEFT] = 1, [KBD_CONT2_DPAD2_LEFT] = 1, 
	[KBD_CONT3_DPAD_LEFT] = 1, [KBD_CONT3_DPAD2_LEFT] = 1, 
	[KBD_CONT4_DPAD_LEFT] = 1, [KBD_CONT4_DPAD2_LEFT] = 1
}
gui_keyright = { 
	[KBD_KEY_RIGHT] = 1, 
	[KBD_CONT1_DPAD_RIGHT] = 1, [KBD_CONT1_DPAD2_RIGHT] = 1, 
	[KBD_CONT2_DPAD_RIGHT] = 1, [KBD_CONT2_DPAD2_RIGHT] = 1, 
	[KBD_CONT3_DPAD_RIGHT] = 1, [KBD_CONT3_DPAD2_RIGHT] = 1, 
	[KBD_CONT4_DPAD_RIGHT] = 1, [KBD_CONT4_DPAD2_RIGHT] = 1
}
gui_keyconfirm = { 
	[KBD_ENTER] = 1, 
	[KBD_CONT1_A] = 1, 
	[KBD_CONT2_A] = 1, 
	[KBD_CONT3_A] = 1, 
	[KBD_CONT4_A] = 1, 
}
gui_keycancel = { 
	[KBD_ESC] = 1, 
	[KBD_CONT1_B] = 1, 
	[KBD_CONT2_B] = 1, 
	[KBD_CONT3_B] = 1, 
	[KBD_CONT4_B] = 1, 
}

-- compute an automatic guess if none is given
function gui_orphanguess_z(z)
	if not z then
		z = gui_curz
		gui_curz = gui_curz + 100
	end
	return z
end

-- compute an automatic guess if none is given, using parent's z
function gui_guess_z(owner, z)
	if not z then
		z = owner.z + 10
	end
	return z
end


-- change focused item
function gui_new_focus(app, f)
	if f and f ~= app.sub then
		evt_send(app.sub, { key = gui_unfocus_event })
		evt_app_remove(f)
		evt_app_insert_first(app, f)
		app.focus_time = 0
		evt_send(app.sub, { key = gui_focus_event })
	end
end


function gui_less(b1, b2, i)
	return b1[i] < b2[i]
end
function gui_more(b1, b2, i)
--	print (b1[i], b2[i])
	return b1[i] > b2[i]
end

-- find closest item to given box, satisfying given condition
gui_closestcoef = { 1, 1, 1, 1 }
gui_closestcoef_horizontal = { 1, 5, 1, 5 }
gui_closestcoef_vertical = { 5, 1, 5, 1 }
function gui_closest(app, box, coef, cond, condi)

--	if not coef then
--		coef = ke_closest_coef
--	end
	box = box * coef
	local i = app.sub
	if not i then
		return
	end
	local imin = nil
	local min = 1000000000
	repeat
		local b = i.box * coef
		if not cond or cond(b, box, condi) then
			local d = b ^ box
			if d < min then
				imin = i
				min = d
			end
		end

		i = i.next
	until not i

--	print (min, app.sub, imin)
	
	return imin
	
end


-- dialog
function gui_dialog_shutdown(app)
	if app.sub then
		evt_send(app.sub, { key = gui_unfocus_event })
	end
	dl_destroy_list(app.dl)
	dl_destroy_list(app.focus_dl)
	evt_app_remove(app)
	cond_connect(1)
end

function gui_dialog_handle(app, evt)
	local key = evt.key

	if key == evt_shutdown_event then
		gui_dialog_shutdown(app)
		return evt -- pass the shutdown event to next app
	end

	local f = app.event_table[key]
	if f then
		return f(app, evt)
	end

	local focused = app.sub
	if focused then
--		print (key, gui_keyright[key])
		if gui_keyconfirm[key] then
			evt_send(focused, { key = gui_press_event })
			return
		end

		if gui_keyup[key] then
			gui_new_focus(app, 
				gui_closest(app, focused.box, 
					gui_closestcoef_vertical, gui_less, 4))
			return
		end

		if gui_keydown[key] then
			gui_new_focus(app, 
				gui_closest(app, focused.box, 
					gui_closestcoef_vertical, gui_more, 2))
			return
		end

		if gui_keyleft[key] then
			gui_new_focus(app, 
				gui_closest(app, focused.box, 
					gui_closestcoef_horizontal, gui_less, 3))
			return
		end

		if gui_keyright[key] then
			gui_new_focus(app, 
				gui_closest(app, focused.box, 
					gui_closestcoef_horizontal, gui_more, 1))
			return
		end

	end

	if app.flags.modal then
		return nil
	end

	return evt
end

function gui_dialog_update(app, frametime)
	local focused = app.sub
	if focused then
		-- handle focus cursor
		dl_set_active(app.focus_dl, 1)

		-- converge to focused item box
		app.focus_box = app.focus_box + 
			20 * frametime * (focused.box - app.focus_box)

		-- set focus cursor position and color
		dl_set_trans(app.focus_dl, 
			mat_scale(app.focus_box[3] - app.focus_box[1], 
				  app.focus_box[4] - app.focus_box[2], 1) *
			mat_trans(app.focus_box[1], app.focus_box[2], 0))

		app.focus_time = app.focus_time + frametime
		local ci = 0.5+0.5*cos(360*app.focus_time*2)
		dl_set_color(app.focus_dl, ci, 1, ci, ci)

	else
		-- no focus cursor
		dl_set_active(app.focus_dl, nil)
	end

end

function gui_new_dialog(owner, box, z, dlsize, text, mode)
	local dial

	z = gui_orphanguess_z(z)

	if not dlsize then
		dlsize = 10*1024
	end
	dial = { 

		name = "gui_dialog",
		version = "0.9",

		handle = gui_dialog_handle,
		update = gui_dialog_update,

		dl = dl_new_list(dlsize, 1),
		box = box,
		z = z,

		focus_dl = dl_new_list(1024, 0),
		focus_box = box,
		focus_time = 0,  -- blinking time

		event_table = { },
		flags  = { }

	}


	-- draw surrounding box
	dl_draw_box(dial.dl, box, z, gui_box_color1, gui_box_color2)

	-- draw the focus cursor
	dl_draw_box(dial.focus_dl, { -0.1, -0.1, 1.1, 0 }, z+0.5, { 1, 1, 1, 1 }, { 1, 1, 1, 1 })
	dl_draw_box(dial.focus_dl, { -0.1, 1, 1.1,  1.1 }, z+0.5, { 1, 1, 1, 1 }, { 1, 1, 1, 1 })
	dl_draw_box(dial.focus_dl, { -0.1, 0, 0, 1 }, z+0.5, { 1, 1, 1, 1 }, { 1, 1, 1, 1 })
	dl_draw_box(dial.focus_dl, { 1, 0,  1.1, 1 }, z+0.5, { 1, 1, 1, 1 }, { 1, 1, 1, 1 })

	if text then
		gui_label(dial, text, mode)
	end

	evt_app_insert_first(owner, dial)

	-- disconnect joypad for main app
	cond_connect(nil)

	return dial
end



-- button
function gui_button_shutdown(app)
end

function gui_button_handle(app, evt)
	local key = evt.key

	if key == evt_shutdown_event then
		gui_button_shutdown(app)
		return evt -- pass the shutdown event to next app
	end

	local f = app.event_table[key]
	if f then
		return f(app, evt)
	end

	return evt
end

-- warning : owner must be a gui item (we use its dl)
function gui_new_button(owner, box, text, mode, z)
	local app

	z = gui_guess_z(owner, z)

	app = { 

		name = "gui_button",
		version = "0.9",

		handle = gui_button_handle,
		update = gui_button_update,

		dl = owner.dl,
		box = box,
		z = z,

		event_table = { },
		flags = { }

	}


	dl_draw_box(app.dl, app.box, z, gui_button_color1, gui_button_color2)

	if text then
		gui_label(app, text, mode)
	end

	evt_app_insert_last(owner, app)

	return app
end



-- input
function gui_input_shutdown(app)
	dl_destroy_list(app.input_dl)
end

gui_input_edline_set = {
[KBD_KEY_HOME] = 1,
[KBD_KEY_END] = 1,
[KBD_KEY_LEFT] = 1,
[KBD_KEY_RIGHT] = 1,
[KBD_KEY_DEL] = 1,
[KBD_BACKSPACE] = 1,

}
function gui_input_handle(app, evt)
	local key = evt.key

	if key == evt_shutdown_event then
		gui_input_shutdown(app)
		return evt -- pass the shutdown event to next app
	end

	local f = app.event_table[key]
	if f then
		return f(app, evt)
	end

	if not app.prev and ((key >= 32 and key < 128) or gui_input_edline_set[key]) then
		app.input, app.input_col = zed_edline(app.input, app.input_col, key)
		gui_input_display_text(app)
		return nil
	end

	if key == gui_focus_event and ke_set_active then
		ke_set_active(1)
		return nil
	end
	if key == gui_unfocus_event and ke_set_active then
		ke_set_active(nil)
		return nil
	end

	return evt
end

function gui_input_display_text(app)
	local w, h = dl_measure_text(app.input_dl, app.input)
	local x = app.box[1]
	local y = (app.box[2] + app.box[4] - h) / 2
	local z = app.z + 1

	dl_clear(app.input_dl)
	dl_draw_text(app.input_dl, x, y, z, gui_text_color, app.input)

	w, h = dl_measure_text(app.input_dl, strsub(app.input, 1, app.input_col-1))
	dl_draw_box(app.input_dl, x+w, y, x+w+2, y+h, z, gui_input_cursor_color1, gui_input_cursor_color2)
--	print (x+w, y, x+w+2, y+h)
end

-- set the input text
function gui_input_set(app, string, col)
	if not string then
		string = ""
	end
	if not col then
		col = strlen(string)+1
	end
	app.input = string
	app.input_col = col
	gui_input_display_text(app)
end

-- warning : owner must be a gui item (we use its dl)
function gui_new_input(owner, box, text, mode, string, z)
	local app

	z = gui_guess_z(owner, z)

	app = { 

		name = "gui_input",
		version = "0.9",

		handle = gui_input_handle,
		update = gui_input_update,

		dl = owner.dl,
		box = box,
		z = z,

		event_table = { },
		flags = { },

		input_dl = dl_new_list(1024, 1)

	}


	dl_draw_box(app.dl, app.box, z, gui_input_color1, gui_input_color2)

	gui_input_set(app, string)

	if text then
		gui_label(app, text, mode)
	end

	evt_app_insert_last(owner, app)

	-- if we are the focused widget, then show the keyboard
	if app.sub == app and ke_set_active then
		ke_set_active(1)
	end

	return app
end



-- display justified text into given box
function gui_justify(dl, box, z, color, text, mode)
	local w = 0
	local h = 0
	local list = { }
	local wlist = { }
	local hlist = { }
	local bw = box[3] - box[1] -- box width

	if not mode then
		mode = { }
	end
	
	-- split the text into portions that fits into the width of the box
	-- TODO handle \n character
	local start = 1
	local len = strlen(text)
	
	while start <= len do
		local e1, e2 = strfind(text, "[%s%p]+", start)
		if not e2 then
			e2 = len
		end
		local last = e2
		while dl_measure_text(dl, strsub(text, start, e2)) < bw do
			last = e2
			e1, e2 = strfind(text, "[%s%p]+", e2+1)
			if not e2 then
				last = len
				e2 = len
				break
			end
		end
		local s = strsub(text, start, last)
		tinsert(list, strsub(text, start, last))
		local cw, ch = dl_measure_text(dl, s)
		tinsert(wlist, cw)
		tinsert(hlist, ch)
		w = max(w, cw)
		h = h + ch
		start = last+1
	end

	-- display the text
	local x, y
	if mode.y == "down" then
		y = box[4] - h
	elseif mode.y == "downout" then
		y = box[4]
	elseif mode.y == "up" then
		y = box[2]
	elseif mode.y == "upout" then
		y = box[2] - h
	else
		y = (box[2] + box[4] - h) / 2
	end
	local i, n
	n = list.n
	for i=1, n, 1 do
		local cw = wlist[i]
		local s = list[i]

		if mode.x == "right" then
			x = box[3] - cw
		elseif mode.x == "rightout" then
			x = box[3]
		elseif mode.x == "left" then
			x = box[1]
		elseif mode.x == "leftout" then
			x = box[1] - cw
		else
			x = (box[1] + box[3] - cw) / 2
		end

		dl_draw_text(dl, x, y, z, color, s)

		y = y + hlist[i]
	end

	-- TODO return bounding box of text
end

-- add a label to a gui item
function gui_label(app, text, mode)
--	gui_justify(app.dl, app.box + gui_text_shiftbox, app.z+1, gui_text_color, text, mode)
	gui_justify(app.dl, app.box, app.z+1, gui_text_color, text, mode)
	-- TODO use an optional mode.boxcolor and render a box around text if it is set ...
end


function gui_shutdown()
end

function gui_init()
	gui_curz = 1000
	gui_press_event = evt_new_code()
	gui_focus_event = evt_new_code()
	gui_unfocus_event = evt_new_code()
end



gui_init()



-- little test
if 1 then

-- create a dialog box with a label outside of the box
dial = gui_new_dialog(evt_desktop_app, { 100, 100, 400, 300 }, 2000, nil, "My dialog box", { x = "left", y = "upout" } )

-- add some text inside the dialog box
gui_label(dial, 
[[
Hello World ! 
Ceci est un tres long texte on purpose !!!!
]]
, { y="up" } )


-- create a few buttons with labels
but = gui_new_button(dial, { 150, 200, 200, 220 }, "OK")

-- add a gui_press_event response
but.event_table[gui_press_event] = function(but, evt)
	print [[OK !!]]
	evt_shutdown_app(but.owner)
	return nil -- block the event
end

but = gui_new_button(dial, { 250, 200, 340, 220 }, "CANCEL")
but.event_table[gui_press_event] = function(but, evt)
	print [[CANCEL !!]]
	evt_shutdown_app(but.owner)
	return nil -- block the event
end

but = gui_new_button(dial, { 150, 250, 200, 270 }, "TITI")
but.event_table[gui_press_event] = function(but, evt)
	print [[TITI !!]]
	return nil -- block the event
end

but = gui_new_button(dial, { 250, 250, 300, 270 }, "TOTO")
but.event_table[gui_press_event] = function(but, evt)
	print [[TOTO !!]]
	return nil -- block the event
end

-- create an input item
input = gui_new_input(dial, { 120, 160, 380, 190 }, "Login :", { x = "left", y="upout" }, "ziggy")


end