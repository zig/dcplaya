--
-- gui lua library on top of evt system
--
-- author : Vincent Penne
--
-- $Id: gui.lua,v 1.1 2002-09-29 06:35:12 vincentp Exp $
--

--
-- a gui item is an application (see in evt.lua for the definition) with
-- additionnal informations :
--
-- box : the box of the item { x1, y1, x2, y2 } used for focusing
-- dl  : the default display list used to draw inside
-- z   : the z position of the item for reference to draw upon it
-- event_table : table of function(app, evt) indexed by event ID
--

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




-- dialog
function gui_dialog_shutdown(app)
	dl_destroy_list(app.dl)
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

	return evt
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

		event_table = { }

	}


	dl_draw_box(dial.dl, box, z, gui_box_color1, gui_box_color2)

	if text then
		gui_label(dial, text, mode)
	end

	evt_app_insert_first(owner, dial)

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

		event_table = { }

	}


	dl_draw_box(app.dl, app.box, z, gui_button_color1, gui_button_color2)

	if text then
		gui_label(app, text, mode)
	end

	evt_app_insert_first(owner, app)

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
	gui_justify(app.dl, app.box + gui_text_shiftbox, app.z+1, gui_text_color, text, mode)
	-- TODO use an optional mode.boxcolor and render a box around text if it is set ...
end


function gui_shutdown()
end

function gui_init()
	gui_curz = 1000
end



gui_init()



-- little test
if 1 then

dial = gui_new_dialog(evt_desktop_app, { 100, 100, 400, 300 }, 2000, nil, "My dialog box", { x = "left", y = "upout" } )

but = gui_new_button(dial, { 150, 200, 200, 220 }, "OK")
but.event_table[KBD_ENTER] = function(but, evt)
	print [[ENTER PRESSED !!]]
	evt_shutdown_app(but.owner)
	return nil -- block the event
end

but = gui_new_button(dial, { 250, 200, 340, 220 }, "CANCEL")
but.event_table[KBD_ESC] = function(but, evt)
	print [[ESCAPE PRESSED !!]]
	evt_shutdown_app(but.owner)
	return nil -- block the event
end

--dl_draw_text(dial.dl, dial.box[1] + 50, dial.box[2] + 50, dial.z+1, gui_text_color, "Hello World !")

gui_justify(dial.dl, dial.box + { 20, 20, -20, -20 }, dial.z+1, gui_text_color, 
[[
Hello World ! 
Ceci est un tres long texte on purpose !!!!
]]
, { y="up" } )

--getchar()

--evt_shutdown_app(dial)

end
