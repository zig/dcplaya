--- @file   textlist.lua
--- @author benjamin gerard <ben@sashipa.com>
--- @date   2002/10/04
--- @brief  Manage and display a list of text.
---
--- $Id: textlist.lua,v 1.11 2002-10-28 18:53:40 benjihan Exp $
---

-- Unload the library
textlist_loaded = nil

if not dolib("basic") then return end

--- @defgroup dcplaya_lua_textlist_gui Text-list GUI
--- @ingroup  dcplaya_lua_gui


--- Entry displayed in textlist.
--- @ingroup dcplaya_lua_textlist_gui
---
--- struct flentry {
---   name; ///< Displayed text
---   size; ///< Optionnal. -1 for highlight (directories).
--- };


--- textlist object definition.
--- @ingroup dcplaya_lua_textlist_gui
---
--- struct textlist {
---
---   flentry dir[]; ///< Current fllist object
---   pos;       ///< Current select item
---   top;       ///< Top displayed line
---   lines;     ///< Computed max displayed lines
---   maxlines;  ///< Real maximum od displayed lines
---   n;         ///< Number of entry in "dir"
---   dl;        ///< Current display list
---   cdl;       ///< Cursor display list
---   mtx;       ///< Current transform matrix
---   cmtx;   ///< Cursor transform matrix (local, need to be multiply by mtx)
---   box;       ///< Display bounding box {x1,y1,x2,y2}
---   bo2;       ///< Extended box info {w,h,z}
---   border;    ///< Size of border. Default 3.
---   span;      ///< Size of separator line (up and down). Default 1.
---   filecolor; ///< Color to display flentry with size >= 0
---   dircolor;  ///< Color to display flentry with size < -1
---   bkgcolor;  ///< Color of background box
---   curcolor;  ///< Color of cursor box
---   /** bool function(textlist).
---    *   This function is call when confirm button is pressed.
---    *   @return
---    *     - @b nil  if action failed
---    *     - @b 0    ok but no change
---    *     - @b 1    if entry is "confirmed"
---    *     - @b 2    if entry is "not confirmed" but change occurs
---    *     - @b 3    if entry is "confirmed" and change occurs
---    */
---   confirm;
---   /** Some flags.
---    *  flags are :
---    *  - @b align    Entry text horizontal alignment :
---    *  -- "left". This is the default.
---    *  -- "center".
---    *  -- "right".
---    */
---   flags;
--- };

--- Dump the content of a textlist.
--- @ingroup dcplaya_lua_textlist_gui
---
--- @param  fl  text-list to dump.
---
function textlist_dump(fl)
	print("textlist object = "..type_dump(fl))
end

--- Change the position and the size of a textlist.
--- @ingroup dcplaya_lua_textlist_gui
---
--- @param  fl  textlist.
--- @param  x   New horizontal position of textlist or nil.
--- @param  y   New vertical position of textlist or nil.
--- @param  w   New width of textlist box or nil.
--- @param  h   New height of textlist box or nil.
--- @param  z   New depth of textlist box or nil.
---
function textlist_set_box(fl,x,y,w,h,z)
	if not fl then return end
	if not fl.box then fl.box = {} end
	if not fl.bo2 then fl.bo2 = {} end

-- [1]:x1 [2]:y1 [3]:x2 [4]:y2 [5]:w [6]:h [7]:z
	if x then fl.box[1] = x else x = fl.box[1] end
	if y then fl.box[2] = y else y = fl.box[2] end
	if w then fl.bo2[1] = w else w = fl.bo2[1] end
	if h then fl.bo2[2] = h else h = fl.bo2[2] end
	if z then fl.bo2[3] = z else z = fl.bo2[3] end

	if fl.minmax then
		if 		w < fl.minmax[1] then w = fl.minmax[1]
		elseif	w > fl.minmax[3] then w = fl.minmax[3] end
		if		h < fl.minmax[2] then h = fl.minmax[2]
		elseif	h > fl.minmax[4] then h = fl.minmax[4] end
	end
	fl.bo2[1] = w
	fl.bo2[2] = h
	fl.box[3] = x + w
	fl.box[4] = y + h
	fl.bo2[3] = z
	fl.lines  = floor( (fl.bo2[2]-2*fl.border) / (fl.font_h+2*fl.span))
	fl.lines = clip_value(fl.lines,0,fl.maxlines)
end

--- Change the position a textlist.
--- @ingroup dcplaya_lua_textlist_gui
--- 
--- @param  fl  textlist.
--- @param  x   New horizontal position of textlist or nil.
--- @param  y   New vertical position of textlist or nil.
--- @param  z   New depth of textlist box or nil.
---
--- @see textlist_set_box
---
function textlist_set_pos(fl,x,y,z)
	if not fl then return end
	textlist_set_box(fl,x,y,nil,nil,z)
	fl.mtx = mat_trans(fl.box[1],fl.box[2],fl.bo2[3])
	if fl.dl  then dl_set_trans(fl.dl, fl.mtx) end
	if fl.cdl then
		if not fl.cmtx then fl.cmtx = mat_trans(0,0,0.5) end
		dl_set_trans(fl.cdl, mat_mult(fl.mtx,fl.cmtx))
	end
end

--- Center a textlist in a box.
--- @ingroup dcplaya_lua_textlist_gui
--- 
--- @param  fl  textlist.
--- @param  x   Horizontal position of the outer box or nil.
--- @param  y   Vertical position of the outer box or nil.
--- @param  w   Width of the outer box or nil.
--- @param  h   Height of the outer box or nil.
--- @param  z   New depth of textlist box or nil.
---
--- @see textlist_set_pos
---
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
	if not fl or not fl.dir or fl.n < 1 then return end
	return 1
end

--
-- Set textlist default values
--
function textlist_default(fl)
	if not fl then return end

	-- Control
	fl.dir = nil
	fl.n = 0;
	fl.maxlines=nil

	-- Border size
	fl.border	= 3
	fl.span		= 1
	fl.font_h	= 0

	-- Colors
	fl.filecolor = { 0.7, 1, 1, 1 }
	fl.dircolor  = { 1.0, 1, 1, 1 }
--	fl.bkgcolor  = { 0.6, 0, 0, 0, 0.6, 0, 0, 0 }
	fl.curcolor  = { 0.5, 1, 1, 0, 0.5, 0.5, 0.5, 0 }

	-- confirm function
	fl.confirm = file_list_default_confirm

	-- Set display box to min.
	fl.minmax = nil;
	textlist_set_box(fl,100,100,0,0,200)

end

-- Mesure maximum width and height for an entry
--
function textlist_measure(fl)
	if not fl or not fl.dir or fl.n < 1 then return {0,0} end

	-- Measure text --
	local i, w, h
	w,h = dl_measure_text(fl.dl, fl.dir[1].name)
	for i=2, fl.n, 1 do
		local w2,h2
		w2,h2 = dl_measure_text(fl.dl, fl.dir[i].name)
		if w2 > w then w = w2 end
		if h2 > h then h = h2 end
	end
	fl.font_h = h;
	return { w, h }
end	

-- Reset textlist
--
function textlist_reset(fl)
	if not fl then end
	local w

	-- Control
	fl.pos		= 0
	fl.top		= 0
	fl.n	= 0
	if fl.dir then fl.n = getn(fl.dir) end

	-- Display lists
	if fl.dl  then dl_destroy_list(fl.dl)  end
	if fl.cdl then dl_destroy_list(fl.cdl) end
	fl.dl  		= dl_new_list(2048,0)
	fl.cdl 		= dl_new_list(512,0)
	w,fl.font_h	= dl_measure_text(fl.dl, "|")

	if not fl.minmax then
		-- min_width, min_height, max_width, max heigth
		local maxlines = fl.maxlines or 8
		fl.minmax = {fl.border*2, fl.border*2,
					400, fl.border*2 + maxlines*(2*fl.span+fl.font_h) }
	end

	-- Compute max lines
	textlist_set_pos(fl,nil,nil,nil)
	
	local dim = textlist_measure(fl)
	textlist_set_box(fl,nil,nil,dim[1] + 2*fl.border,
					2*fl.border + (dim[2]+2*fl.span)*fl.n, nil)

	fl.pos = -1
	fl.top = 1
	textlist_movecursor(fl,1)
end

--- Create a textlist objects.
--- @ingroup dcplaya_lua_textlist_gui
--- 
--- @param  flparm   Optionnal creation structure, with optionnal fields.
---                  Most fields are the same than fllist ones.
---
---  Exceptions are:
---
---    - pos : { x, y, z }. Each component is optionnal. Default {100,100,100}
---    - box : { min_width, max_width, min_heigth, max_height }
---          display box min/max Default={0,0,400, size for 8 entries)
---
--- @return textlist object
--- @retval nil error
---
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
		
		if flparm.lines		then fl.maxlines	= flparm.lines		end
		if flparm.box		then fl.minmax		= flparm.box		end
		if flparm.confirm 	then fl.confirm		= flparm.confirm 	end
		if flparm.border 	then fl.border		= flparm.border		end
		if flparm.span 		then fl.span		= flparm.span		end
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
	fl.n	= 0;
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
	local dlsize = 256  -- For the dl_draw_box + dl_set_clipping

	local max=fl.top+fl.lines
	if max > fl.n then max = fl.n end
	for i=fl.top+1, max, 1 do
		dlsize = dlsize + 64 + strlen(fl.dir[i].name);
	end

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
	if fl.n > 0 then
		dl_clear(fl.cdl)
		dl_set_trans(fl.cdl, mat_mult(fl.mtx,fl.cmtx)) -- $$$ order ?

		local i = fl.pos+1
		local color = fl.dircolor
		if fl.dir[i].size and fl.dir[i].size >= 0 then
			color = fl.filecolor
		end

		local w,h
		w,h = dl_measure_text(fl.cdl, fl.dir[i].name)
		if fl.bo2[1] > w then w = fl.bo2[1] end

		dl_draw_box(fl.cdl,
--				0, 0, w, fl.font_h+2*fl.span, 0,
				0, 0, w, fl.dir[i].h, 0,
				fl.curcolor[1], fl.curcolor[2],	fl.curcolor[3], fl.curcolor[4],
				fl.curcolor[5], fl.curcolor[6],	fl.curcolor[7], fl.curcolor[8])

		dl_draw_text(fl.cdl,
					 fl.border, fl.span, 0.1,
					 color[1]+0.3,color[2],color[3],color[4],
					 fl.dir[i].name)


		dl_set_active(fl.cdl,1)
	else
		dl_set_active(fl.cdl,0)
	end
end

function textlist_update(fl)
	if not fl then return end

	textlist_create_dl(fl)
	if not fl.dl then return end

	local i,y,h
	local max=fl.top+fl.lines
	if max > fl.n then max = fl.n end
	y = fl.span + fl.border
	h = fl.font_h+2*fl.span

	for i=fl.top+1, max, 1 do
		local color = fl.dircolor

		if fl.dir[i].size and fl.dir[i].size >= 0 then
			color = fl.filecolor
		end

		dl_draw_text(fl.dl,
					fl.border, y, 0.1,
					color[1],color[2],color[3],color[4],
					fl.dir[i].name)
		fl.dir[i].y = y-fl.span
		fl.dir[i].h = h
		y = y + h
	end
	-- Here because commands are fetched from last to first
	dl_set_clipping(fl.dl, 0, 0, fl.bo2[1], fl.bo2[2])

	if fl.bkgcolor then
		dl_draw_box(fl.dl,
				0, 0, fl.bo2[1], fl.bo2[2],	0,
				fl.bkgcolor[1],fl.bkgcolor[2],fl.bkgcolor[3],fl.bkgcolor[4],
				fl.bkgcolor[5],fl.bkgcolor[6],fl.bkgcolor[7],fl.bkgcolor[8],
				fl.bkgtype)
	end
	dl_set_active(fl.dl,1)
end

-- Textlist move cursor from 'mov'
--
function textlist_movecursor(fl,mov)
	if not fl then return end
	local top,pos,action
	action=0
	top = fl.top
	pos = fl.pos + mov
	if pos >= fl.n then pos = fl.n-1 end
	if pos < 0 then pos = 0 end
	if pos < top then
		top = pos
	elseif pos >= top+fl.lines then
		top = pos - fl.lines + 1
		if top < 0 then top = 0 end
	end

	-- $$$ Just move this to try. It seems to be OK.
	if top ~= fl.top then
		fl.top = top
		textlist_update(fl)
		action = action + 2
	end

	if pos ~= fl.pos then
		local y
		y = fl.border + (pos-top) * (fl.font_h + 2*fl.span)
		fl.cmtx = mat_trans(0,y,0.5)
		fl.pos = pos
		textlist_updatecursor(fl)
		action = 1
	end

	return action
end

function textlist_get_entry(fl)
	if not fl or not fl.dir or fl.n < 1 then return end
	local entry = fl.dir[fl.pos+1]
	return { name=entry.name, size=entry.size }
end

function textlist_find_entry_expr(fl,regexpr)
	if not fl or not fl.dir or fl.n < 1 or not regexpr then return end
	local i
	for i=1, fl.n, 1 do
		if strfind(fl.dir[i].name,regexpr) then
			return textlist_movecursor(fl,i-1-fl.pos)
		end
	end
	return
end

function textlist_locate_entry(fl,name)
	if not fl or not fl.dir or fl.n < 1 or not name then return end
	local i
	for i=1, fl.n, 1 do
		if fl.dir[i].name == name then
			return textlist_movecursor(fl,i-1-fl.pos)
		end
	end
	return
end

-- textlist GUI application event handler.
-- @internal
--
function textlist_gui_handle(app,evt)
	local key = evt.key
	local fl = app.fl
	local dir = nil

	if fl then dir = fl.dir end

	if key == evt_shutdown_event then
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

--- Create a textlist applcation from a textlist object.
--- @ingroup dcplaya_lua_textlist_gui
---
--- @param  fl     textlist
--- @param  owner  Parent of the created application
---
--- @return application.
--- @retval nil error
---
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
--- @ingroup dcplaya_lua_textlist_gui
---
---
function gui_textlist(owner, flparm)
	if not owner then return nil end
	local fl = textlist_create(flparm)
	if not fl then return nil end
	return textlist_create_gui(fl, owner)
end

textlist_loaded = 1
return textlist_loaded
