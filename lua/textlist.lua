--- textlist.lua
-- 
-- author : benjamin gerard <ben@sashipa.com>
-- date   : 2002/10/04
--
-- $Id: textlist.lua,v 1.4 2002-10-08 20:48:34 benjihan Exp $
--

--- textlist object - Display a textlist from a given dir
--
-- components :
-- "dir"     	Current fllist object
-- "pos"     	Current select item
-- "top"     	Top displayed line
-- "lines"   	Max displayed lines
-- "entries" 	Number of entry in "dir"
--
-- "dl"      	Current display list
-- "cdl"     	Cursor display list
-- "mtx"     	Current transform matrix
-- "cmtx"    	Cursor transform matrix (local, need to be multiply by mtx)
--
-- "box"     	Display bounding box {x1,y1,x2,y2}
-- "bo2"        Extended box info {w,h,z}
-- "border"  	Size of border. Default 3.
-- "span"    	Size of separator line (up and down). Default 1.
--
-- "filecolor"	Color to display flentry with size >= 0
-- "dircolor"	Color to display flentry with size < -1
-- "bkgcolor"	Color of background box
-- "curcolor"	Color of cursor box
--
-- "confirm"	bool function(textlist). This function is call when confirm
--              button is pressed.
--              Returns :	- nil if action failed
--							- 0 ok but no change
--							- 1 if entry is "confirmed"
--							- 2 if entry is "not confirmed" but change occurs
--                          - 3 if entry is "confirmed" and change occurs
-- "flags"		{ no_bkg align=["left"|"center"|"right"] }
--              	no_bkg : Do not display background box
--               	align  : Entry text horizontal alignment. default="left"



--- fllist object - List of object to display in textlist.
--  fllist := { flentry ... }

--- flentry - Element displayed in textlist
--  flentry := { name=STRING ... }


function textlist_dump(fl)
	if not fl then print("textlist : nil") end

	print("Boxes:")
	print(format(" box {x1:%d y1:%d x2:%d y2:%d w:%d h:%d z:%d}",
		fl.box[1],fl.box[2],fl.box[3],fl.box[4],
		fl.bo2[1],fl.bo2[2],fl.bo2[3]))
	print(format(" minmax {minw:%d minh:%d maxw:%d maxh:%d}",
		fl.minmax[1], fl.minmax[2], fl.minmax[3], fl.minmax[4]))
	print(format(" border:%d span:%d",fl.border, fl.span))

	print("Controls:")
	print(format(" top:%d pos:%d lines:%d entries:%d",
		fl.top, fl.pos, fl.lines, fl.entries))
	if (fl.dir) then
		print(format(" dir: %d entries", getn(fl.dir)))
	else
		print(format(" dir: nil"))
	end
end

function textlist_set_box(fl,x,y,w,h,z)
	if not fl then return end
	if not fl.box then fl.box = {} end
	if not fl.bo2 then fl.bo2 = {} end

--	print("textlist_set_box...")

-- [1]:x1 [2]:y1 [3]:x2 [4]:y2 [5]:w [6]:h [7]:z
	if x then fl.box[1] = x else x = fl.box[1] end
	if y then fl.box[2] = y else y = fl.box[2] end
	if w then fl.bo2[1] = w else w = fl.bo2[1] end
	if h then fl.bo2[2] = h else h = fl.bo2[2] end
	if z then fl.bo2[3] = z else z = fl.bo2[3] end

--	print(format("box-before-minmax %d %d %d %d %d %d", x,y,x+w,y+h,w,h))

-- Check min/max box
	if fl.minmax then

--	print(format("minmax %d %d - %d %d",
--		fl.minmax[1],fl.minmax[2],fl.minmax[3],fl.minmax[4] ))

		if 		w < fl.minmax[1] then w = fl.minmax[1]
		elseif	w > fl.minmax[3] then w = fl.minmax[3] end
		if		h < fl.minmax[2] then h = fl.minmax[2]
		elseif	h > fl.minmax[4] then h = fl.minmax[4] end

--		print(format("new size: %d %d", w,h))
	end
	fl.bo2[1] = w
	fl.bo2[2] = h
	fl.box[3] = x + w
	fl.box[4] = y + h
	fl.bo2[3] = z
	fl.lines  = floor( (fl.bo2[2]-2*fl.border) / (fl.font_h+2*fl.span))
	if fl.lines < 0 then fl.lines = 0 end
--	print(format("LINES=%d B:%d H:%d FH:%d",fl.lines, fl.border, fl.bo2[2], fl.font_h));

--	print(format("setbox {x1:%d y1:%d x2:%d y2:%d w:%d h:%d z:%d lines=%d}",
--		fl.box[1],fl.box[2],fl.box[3],fl.box[4],
--		fl.bo2[1],fl.bo2[2],fl.bo2[3], fl.lines))
--	print("...textlist_set_box")
		
end

function textlist_set_pos(fl,x,y,z)
	if not fl then return end
--	print("textlist_set_pos...")
	textlist_set_box(fl,x,y,nil,nil,z)
	fl.mtx = mat_trans(fl.box[1],fl.box[2],fl.bo2[3])
	if fl.dl  then dl_set_trans(fl.dl, fl.mtx) end
	if fl.cdl then
		if not fl.cmtx then fl.cmtx = mat_new() end
		dl_set_trans(fl.cdl, mat_mult(fl.mtx,fl.cmtx))
	end
--	print("...textlist_set_pos")
end

function textlist_center(fl, x, y, w, h, z)
	if x and w then
		x = x + (w-fl.bo2[1]) * 0.5
	end
	if y and h then
		y = y + (h-fl.bo2[1]) * 0.5
	end
	textlist_set_pos(fl,x,y,z)
end


-- confirm default action : return current entry
--
function file_list_default_confirm(fl)
	if not fl or not fl.dir or fl.entries < 1 then return end
	return 1
end

--
-- Set textlist default values
--
function textlist_default(fl)
	if not fl then return end

--	print("textlist_default...")

	-- Control
	fl.dir = nil
	fl.entries = 0;

	-- Border size
	fl.border	= 3
	fl.span		= 1
	fl.font_h	= 0

	-- Colors
	fl.filecolor = { 0.7, 1, 1, 1 }
	fl.dircolor  = { 1.0, 1, 1, 1 }
	fl.bkgcolor  = { 0.6, 0, 0, 0, 0.6, 0, 0, 0 }
	fl.curcolor  = { 0.5, 1, 1, 0, 0.5, 0.5, 0.5, 0 }

	-- confirm function
	fl.confirm = file_list_default_confirm

	-- Set display box to min.
	fl.minmax = nil;
	textlist_set_box(fl,100,100,0,0,200)

--	print("...textlist_default")

end

-- Mesure maximum width and height for an entry
--
function textlist_measure(fl)
	if not fl or not fl.dir or fl.entries < 1 then return {0,0} end

	-- Measure text --
	local i, w, h
	w,h = dl_measure_text(fl.dl, fl.dir[1].name)
	for i=2, fl.entries, 1 do
		local w2,h2
		w2,h2 = dl_measure_text(fl.dl, fl.dir[i].name)
		if w2 > w then w = w2 end
		if h2 > h then h = h2 end
	end
	fl.font_h = h;
--	print(format("measure=%d %d",w,h))
	return { w, h }
end	

-- Reset textlist
--
function textlist_reset(fl)
	if not fl then end
	local w

--	print("textlist_reset...")

	-- Control
	fl.pos		= 0
	fl.top		= 0
	fl.entries	= 0
	if fl.dir then fl.entries = getn(fl.dir) end

	-- Display lists
	if fl.dl  then dl_shutdown(fl.dl)  end
	if fl.cdl then dl_shutdown(fl.cdl) end
	fl.dl  		= dl_new_list(2048,0)
	fl.cdl 		= dl_new_list(128,0)
	w,fl.font_h	= dl_measure_text(fl.dl, "|")
--	print(format("font_h:%d",fl.font_h))

	if not fl.minmax then
		-- min_width, min_height, max_width, max heigth
		fl.minmax = {fl.border*2, fl.border*2,
					400, fl.border*2 + 8*(2*fl.span+fl.font_h) }
	end

--	print(format("minmax {minw:%d minh:%d maxw:%d maxh:%d}",
--		fl.minmax[1], fl.minmax[2], fl.minmax[3], fl.minmax[4]))

	-- Compute max lines
	textlist_set_pos(fl,nil,nil,nil)
	
	local dim = textlist_measure(fl)
	textlist_set_box(fl,nil,nil,dim[1] + 2*fl.border,
					2*fl.border + (dim[2]+2*fl.span)*fl.entries, nil)

	fl.pos = -1
	fl.top = 1
	textlist_movecursor(fl,1)

--	print("...textlist_reset")
end

-- Textlist create function :
-- 
-- flparm : Optionnal creation structure, with optionnal fields. Most fields
--          are the same than fllist ones.
--
--  Exceptions are:
--
--        pos : { x, y, z }. Each component is optionnal. Default {100,100,100}
--        box : { min_width, max_width, min_heigth, max_height }
--				display box min/max Default={0,0,400,size for 8 entries)
--
function textlist_create(flparm)
	local fl = {}

-- Set default value
	textlist_default(fl)

-- Overrides default by user's ones supply parms
	if flparm then
		fl.dir   = flparm.dir
		fl.flags = flparm.flags;

		if flparm.pos then
			textlist_set_pos(fl, flparm.pos[1], flparm.pos[2], flparm.pos[3])
--			print(format("SET POS:%d %d",
--				flparm.pos[1], flparm.pos[2]))
		end
		
		if flparm.box		then fl.minmax		= flparm.box		end
		if flparm.confirm 	then fl.confirm		= flparm.confirm 	end
		if flparm.border 	then fl.border		= flparm.border		end
		if flparm.span 		then fl.border		= flparm.span		end
		if flparm.filecolor then fl.filecolor	= flparm.filecolor	end
		if flparm.dircolor  then fl.dircolor	= flparm.dircolor	end
		if flparm.bkgcolor  then fl.bkgcolor	= flparm.bkgcolor	end
		if flparm.curcolor  then fl.curcolor	= flparm.curcolor	end
	end

	textlist_reset(fl)
--	textlist_dump(fl)
	return fl
end

-- Textlist shutdown:
-- 
function textlist_shutdown(fl)
	if not fl then return end
	if fl.dl  then dl_destroy_list(fl.dl)  end
	if fl.cdl then dl_destroy_list(fl.cdl) end
	fl.mtx 		= nil
	fl.cmtx 	= nil
	fl.dir  	= nil
	fl.dl		= nil
	fl.cdl		= nil
	fl.entries	= 0;
end

-- Allocate display_list, try to make an approx of the needed size
-- 
function textlist_create_dl(fl)
	if not fl then return end

	-- Get current size
	local cursize=0
	if fl.dl then cursize = dl_heap_size(fl.dl) end
	
	-- Compute an approx size for the new display list
	local i,i
	local dlsize = 128 -- For the dl_draw_box

	local max=fl.top+fl.lines
	if max > fl.entries then max = fl.entries end
	for i=fl.top+1, max, 1 do
		dlsize = dlsize + 64 + strlen(fl.dir[i].name);
	end
--	print("compute dlsize:"..dlsize)

	if dlsize > cursize then
		-- Need more room
		if fl.dl then dl_destroy_list(fl.dl) end
		fl.dl = dl_new_list(dlsize,0)
		dl_set_trans(fl.dl, fl.mtx)
	else
		dl_clear(fl.dl)
	end
	dl_set_active(fl.dl,0)
end

function textlist_updatecursor(fl)
	if not fl then return end
	if fl.entries > 0 then
		dl_clear(fl.cdl)
		dl_set_trans(fl.cdl, mat_mult(fl.mtx,fl.cmtx)) -- $$$ order ?
		dl_draw_box(fl.cdl,
				0, 0, fl.bo2[1], fl.font_h+2*fl.span, 0,
				fl.curcolor[1], fl.curcolor[2],	fl.curcolor[3], fl.curcolor[4],
				fl.curcolor[5], fl.curcolor[6],	fl.curcolor[7], fl.curcolor[8])
		dl_set_active(fl.cdl,1)
	else
		dl_set_active(fl.cdl,0)
	end
--	print("..textlist_updatecursor")

end

function textlist_update(fl)
	if not fl then return end

	textlist_create_dl(fl)
	if not fl.dl then return end

--	print("textlist_update...")

	if not (fl.flags and fl.flags.no_bkg) then
--		print(" > draw_box")

		dl_draw_box(fl.dl,
				0, 0, fl.bo2[1], fl.bo2[2],	0,
				fl.bkgcolor[1],fl.bkgcolor[2],fl.bkgcolor[3],fl.bkgcolor[4],
				fl.bkgcolor[5],fl.bkgcolor[6],fl.bkgcolor[7],fl.bkgcolor[8])
	end

	local i,y,h
	local max=fl.top+fl.lines
	if max > fl.entries then max = fl.entries end
	y = fl.span + fl.border
	h = fl.font_h+2*fl.span

	for i=fl.top+1, max, 1 do
		local color = fl.dircolor
--		print(format(" > draw_text #%d at %d '%s'", i, y, fl.dir[i].name))

		if fl.dir[i].size and fl.dir[i].size > 0 then
			color = fl.filecolor
		end

		dl_draw_text(fl.dl,
					fl.border, y, 0.1,
					color[1],color[2],color[3],color[4],
					fl.dir[i].name)
		y = y + h
	end
	dl_set_active(fl.dl,1)
--	print("..textlist_update")
end

-- Textlist move cursor from 'mov'
--
function textlist_movecursor(fl,mov)
	if not fl then return end
	local top,pos,action
	action=0
	top = fl.top
	pos = fl.pos + mov
	if pos >= fl.entries then pos = fl.entries-1 end
	if pos < 0 then pos = 0 end
	if pos < top then
		top = pos
	elseif pos >= top+fl.lines then
		top = pos - fl.lines + 1
		if top < 0 then top = 0 end
	end

	if pos ~= fl.pos then
		local y
		y = fl.border + (pos-top) * (fl.font_h + 2*fl.span)
--		print(format("cursor y = %d",y))
		fl.cmtx = mat_trans(0,y,0.1)
		fl.pos = pos
		textlist_updatecursor(fl)
		action = 1
	end

	if top ~= fl.top then
--		print(format("textlist : top '%d'",top))
		fl.top = top
		textlist_update(fl)
		action = action + 2
	end
--	print(format("action:%d",action))
	return action
end

function textlist_get_entry(fl)
	if not fl or not fl.dir or fl.entries < 1 then return end
	local entry = fl.dir[fl.pos+1]
	return { name=entry.name, size=entry.size }
end

function textlist_find_entry_expr(fl,regexpr)
	if not fl or not fl.dir or fl.entries < 1 or not regexpr then return end
	local i
	for i=1, fl.entries, 1 do
		if strfind(fl.dir[i].name,regexpr) then
--			print(format("textlist_locate_entry_expr(%s) found %s at %d",
--				regexpr, fl.dir[i].name,i))
			return textlist_movecursor(fl,i-1-fl.pos)
		end
	end
	return
end

function textlist_locate_entry(fl,name)
	if not fl or not fl.dir or fl.entries < 1 or not name then return end
	local i
	for i=1, fl.entries, 1 do
		if fl.dir[i].name == name then
--			print(format("textlist_locate_entry(%s) found at %d",name,i))
			return textlist_movecursor(fl,i-1-fl.pos)
		end
	end
	return
end

-- textlist application event handler
--
function textlist_gui_handle(app,evt)
	local key = evt.key
	local fl = app.fl
	local dir = nil

	if fl then dir = fl.dir end

	if key == evt_shutdown_event then
--		print("textlist : handle shutdown")
		textlist_shutdown(fl)
		app.done = 1
		return evt
	end

	if not dir or not gui_is_focus(app.owner,app) then
		-- No dir loaded or no focus, ignore event --
		return evt;
	elseif gui_keyup[key] then
		local code = textlist_movecursor(fl,-1)
		if code and code > 0 then
			evt_send(app.owner, { key = gui_item_change_event })
			return nil
		end
		return nil --evt
	elseif gui_keydown[key] then
		local code = textlist_movecursor(fl,1)
		if code and code > 0 then
--			print("down")
			evt_send(app.owner, { key = gui_item_change_event })
			return nil
		end
		return nil --evt
	elseif gui_keycancel[key] then
		evt_send(app.owner, { key = gui_item_cancel_event })
		return nil
	elseif gui_keyconfirm[key] then
		local action = fl.confirm(fl)
		if not action then return end
		if action == 1 then
			evt_send(app.owner, { key = gui_item_confirm_event })
		elseif action == 2 then
			evt_send(app.owner, { key = gui_item_change_event })
		else
			evt_send(app.owner, { key = gui_item_change_event })
			evt_send(app.owner, { key = gui_item_confirm_event })
		end
	else
		return evt
	end
end

function textlist_create_app(fl,owner)
	app = {	name = "textlist",
			version = "0.1",
			handle = textlist_app_handle,
--			update = textlist_app_update,
			fl = fl}
	evt_app_insert_first(owner, app)
	return app
end

function textlist_create_gui(fl, owner)
	if not fl or not owner then return nil end
	local app = textlist_create_app(fl,owner)
	if not app then return nil end

	if not gui_textlist_event then gui_textlist_event = evt_new_code() end

	app.z		= gui_guess_z(owner, nil)
--	textlist_set_pos(fl, owner.box[1], owner.box[2], gui_guess_z(app,nil))
	textlist_set_pos(fl, nil, nil, gui_guess_z(app,nil))
	app.fl		= fl
	app.name	= "gui "..app.name
	app.box		= app.fl.box
	app.dl		= nil
	app.heritedhandle = app.handle
	app.handle = textlist_gui_handle
	app.event_table = {}
	app.flags 		= {}
	return app
end

--- Create textlist gui application.
--
function gui_textlist(owner, flparm)
	if not owner then return nil end
	local fl = textlist_create(flparm)
	if not fl then return nil end
	return textlist_create_gui(fl, owner)
end


--- Run a standalone textlist. Do not need event system.
--
function textlist_standalone_run(fl)
	if not fl then return nil end
	local done = nil
	local result = nil
	local actions = {
		[KBD_CONT1_DPAD_UP] 	= 1,	[KBD_CONT1_DPAD2_UP] 	= 1,
		[KBD_CONT2_DPAD_UP] 	= 1,	[KBD_CONT2_DPAD2_UP] 	= 1,
		[KBD_CONT3_DPAD_UP] 	= 1,	[KBD_CONT3_DPAD2_UP] 	= 1,
		[KBD_CONT4_DPAD_UP] 	= 1,	[KBD_CONT4_DPAD2_UP] 	= 1,
		[KBD_KEY_UP] 			= 1,

		[KBD_CONT1_DPAD_DOWN] 	= 2,	[KBD_CONT1_DPAD2_DOWN] 	= 2,
		[KBD_CONT2_DPAD_DOWN] 	= 2,	[KBD_CONT2_DPAD2_DOWN] 	= 2,
		[KBD_CONT3_DPAD_DOWN] 	= 2,	[KBD_CONT3_DPAD2_DOWN] 	= 2,
		[KBD_CONT4_DPAD_DOWN] 	= 2,	[KBD_CONT4_DPAD2_DOWN] 	= 2,
		[KBD_KEY_DOWN] 			= 2,

		[KBD_CONT1_B] 			= 3,	[KBD_CONT1_B] 			= 3,
		[KBD_CONT2_B] 			= 3,	[KBD_CONT2_B] 			= 3,
		[KBD_CONT3_B] 			= 3,	[KBD_CONT3_B] 			= 3,
		[KBD_CONT4_B] 			= 3,	[KBD_CONT4_B] 			= 3,
		[KBD_ESC] 				= 3,

		[KBD_CONT1_A] 			= 4,	[KBD_CONT1_A] 			= 4,
		[KBD_CONT2_A] 			= 4,	[KBD_CONT2_A] 			= 4,
		[KBD_CONT3_A] 			= 4,	[KBD_CONT3_A] 			= 4,
		[KBD_CONT4_A] 			= 4,	[KBD_CONT4_A] 			= 4,
		[KBD_ENTER] 			= 4}

	local cond_state = cond_connect(nil)
	repeat
		local action = actions[getchar()]
		if action == 4 then
			result = fl.confirm(fl)
			if result then
				done = 1
			end
		elseif action == 1 then
			textlist_movecursor(fl, -1)
		elseif action == 2 then
			textlist_movecursor(fl,  1)
		elseif action == 3 then
			done = 1
		end
	until done
	textlist_shutdown(fl)
	cond_connect(cond_state)
	return result
end

textlist_loaded = 1
print("Loaded textlist.lua")

if nil then
print("Run test (y/n) ?")
c = getchar()
if c == 121 then
	print ("Run test")
	print ("Load dir")
	dir = dirlist("/pc/t")
	print (dir)
	print ("Create text list")
	fl = textlist_create( {dir=dir} )
	print (fl)
	textlist_standalone_run(fl)
end
end
