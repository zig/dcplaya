--- @file   textlist.lua
--- @author benjamin gerard <ben@sashipa.com>
--- @date   2002/10/04
---

-- Unload the library
textlist_loaded = nil

---

-- Unload the library
textlist_loaded = nil

if not dolib("basic") then return end

--- @defgroup dcplaya_lua_textlist_gui Text-list GUI
--- @ingroup  dcplaya_lua_gui

--- Entry displayed in textlist.
-- @ingroup dcplaya_lua_textlist_gui
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

if not dolib("basic") then return end

--- @defgroup dcplaya_lua_textlist_gui Text-list GUI
--- @ingroup  dcplaya_lua_gui

--- Entry displayed in textlist.
-- @ingroup dcplaya_lua_textlist_gui
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

---

-- Unload the library
textlist_loaded = nil

if not dolib("basic") then return end

--- @defgroup dcplaya_lua_textlist_gui Text-list GUI
--- @ingroup  dcplaya_lua_gui

--- Entry displayed in textlist.
-- @ingroup dcplaya_lua_textlist_gui
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

---

-- Unload the library
textlist_loaded = nil

if not dolib("basic") then return end

--- @defgroup dcplaya_lua_textlist_gui Text-list GUI
--- @ingroup  dcplaya_lua_gui

--- Entry displayed in textlist.
-- @ingroup dcplaya_lua_textlist_gui
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

---

-- Unload the library
textlist_loaded = nil

if not dolib("basic") then return end

--- @defgroup dcplaya_lua_textlist_gui Text-list GUI
--- @ingroup  dcplaya_lua_gui

--- Entry displayed in textlist.
-- @ingroup dcplaya_lua_textlist_gui
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

--- @brief  Manage and display a list of text.
---
--- $Id: textlist.lua,v 1.13 2002-12-01 19:19:14 ben Exp $
---

-- Unload the library
textlist_loaded = nil

if not dolib("basic") then return end

--- @defgroup dcplaya_lua_textlist_gui Text-list GUI
--- @ingroup  dcplaya_lua_gui

--- Entry displayed in textlist.
-- @ingroup dcplaya_lua_textlist_gui
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
---   confirm();
---   /** Some flags.
---    *  flags are :
---    *  - @b align    Entry text horizontal alignment :
---    *  -- "left". This is the default.
---    *  -- "center".
---    *  -- "right".
---    */
---   flags;
--- };

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

   --- Change textlist position and size.
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

   --- Change textlist position.
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
	  fl:set_box(x,y,nil,nil,z)
	  fl.mtx = mat_trans(fl.box[1],fl.box[2],fl.bo2[3])
	  if fl.dl  then dl_set_trans(fl.dl, fl.mtx) end
	  -- 		if fl.cdl then
	  -- 			if not fl.cmtx then fl.cmtx = mat_trans(0,0,0.5) end
	  -- 			dl_set_trans(fl.cdl, mat_mult(fl.mtx,fl.cmtx))
	  -- 		end
   end

   --- Set textlist color.
   --- @ingroup dcplaya_lua_textlist_gui
   --- 
   --- @param  fl  textlist.
   --- @param  a   New alpha component or nil
   --- @param  r   New red component or nil
   --- @param  g   New green component or nil
   --- @param  b   New blue component or nil
   ---
   function textlist_set_color(fl,a,r,g,b)
	  dl_set_color(fl.dl,a,r,g,b)
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
	  fl:set_pos(x,y,z)
   end

   -- confirm default action : return current entry
   --
   function textlist_confirm(fl)
	  if not fl or fl.dir.n < 1 then return end
	  return 1
   end

   -- Mesure maximum width and height for an entry
   --
   function textlist_measure(fl)
	  if fl.dir.n < 1 then return 0,0 end

	  -- Measure text --
	  local i, w, h
	  w,h = fl:measure_text(fl.dir[1])
	  for i=2, fl.dir.n, 1 do
		 local w2,h2
		 w2,h2 = fl:measure_text(fl.dir[i])
		 if w2 > w then w = w2 end
		 if h2 > h then h = h2 end
	  end
	  fl.font_h = h;
	  return w, h
   end

   function textlist_measure_text(fl, entry)
	  return dl_measure_text(fl.dl, entry.name)
   end

   -- Reset textlist
   --
   function textlist_reset(fl)
	  local w,h

	  -- Control
	  fl.pos		= 0
	  fl.top		= 0

	  -- Display lists :

	  -- Main-list
	  fl.dl = fl.dl or dl_new_list(128);
	  dl_set_active(fl.dl,0)
	  dl_clear(fl.dl)

	  -- Cursor sub-list
	  fl.cdl = fl.cdl or dl_new_list(512,1,1)
	  dl_set_active(fl.cdl,0)
	  dl_clear(fl.cdl)

	  -- 2 sub-lists for double buffering rendering
	  fl.dl1 = fl.dl1 or dl_new_list(1024,0,1)
	  dl_set_active(fl.dl1,0)
	  dl_clear(fl.dl1)
	  fl.dl2 = fl.dl2 or dl_new_list(1024,0,1)
	  dl_set_active(fl.dl2,0)
	  dl_clear(fl.dl2)

	  dl_text_prop(fl.dl, 0, 16, 1)
	  dl_sublist(fl.dl, fl.cdl)
	  dl_sublist(fl.dl, fl.dl2)
	  dl_sublist(fl.dl, fl.dl1)
--$$$
-- 	  dl_set_color(fl.dl1, 1, 1,0,0)
-- 	  dl_set_color(fl.dl2, 1, 0,0,1)

	  
	  w,fl.font_h = fl:measure_text({ name="|" })

	  if not fl.minmax then
		 -- min_width, min_height, max_width, max heigth
		 local maxlines = fl.maxlines or 8
		 fl.minmax = {fl.border*2, fl.border*2,
			400, fl.border*2 + maxlines*(2*fl.span+fl.font_h) }
	  end

	  -- Compute max lines
	  fl:set_pos(nil,nil,nil)
	  fl:change_dir(fl.dir)
   end

   function textlist_change_dir(fl,dir)
	  if not dir then dir = {} end
	  if not dir.n then dir.n = getn(fl.dir) end
	  if not dir.n then dir.n = 0 end
	  fl.dir = dir

	  local w,h = textlist_measure(fl)
	  fl:set_box(nil,nil,
				 2*fl.border + w,
				 2*fl.border + (h+2*fl.span)*fl.dir.n, nil)

	  -- Set invalid top and pos will force update for both dl --
	  fl.top = 1
	  fl.pos = 1
	  return fl:move_cursor(-1)
   end


   --- Draw textlist.
   --
   function textlist_draw(fl)
	  textlist_draw_list(fl)
	  textlist_draw_cursor(fl)
   end

   --- Textlist shutdown
   ---
   function textlist_shutdown(fl)
	  local i,v
	  for i,v in fl do
		 fl[i] = nil
	  end
   end

   --- Draw given textlist entry in given diplay list.
   --
   function textlist_draw_entry(fl, dl, i, x ,y , z)
	  local color = fl.dircolor
	  if fl.dir[i].size and fl.dir[i].size >= 0 then
		 color = fl.filecolor
	  end
	  return dl_draw_text(dl,
						  x, y, z,
						  color[1],color[2],color[3],color[4],
						  fl.dir[i].name)
   end

   function textlist_draw_cursor(fl)
	  if fl.dir.n < 1 then 
		 dl_set_active(fl.cdl,0)
		 return
	  end
	  dl_clear(fl.cdl)
	  dl_set_trans(fl.cdl, fl.cmtx)

	  local i,w,h
	  i = fl.pos+1
	  w,h = fl:measure_text(fl.dir[i])
	  if fl.bo2[1] > w then w = fl.bo2[1] end
	  
	  dl_draw_box(fl.cdl,
				  0, 0, w, h+2*fl.span, 0,
				  fl.curcolor[1],fl.curcolor[2],fl.curcolor[3],fl.curcolor[4],
				  fl.curcolor[5],fl.curcolor[6],fl.curcolor[7],fl.curcolor[8])
	  fl:draw_entry(fl.cdl, i, fl.border, fl.span, 0.1)
	  dl_set_active(fl.cdl,1)
   end

   function textlist_swap_dl(fl)
 	  dl_set_active2(fl.dl1, fl.dl2, 1)
 	  dl_clear(fl.dl2)
	  fl.dl1, fl.dl2 = fl.dl2, fl.dl1
	  dl_set_active(fl.dl,1)
--  	  dl_set_active(fl.dl1,1)
--  	  dl_set_active(fl.dl2,0)
-- 	  print("swap dl",fl.dl1,fl.dl2)
   end


   function textlist_draw_list(fl)
	  local dl = fl.dl1

--	  dl_set_clipping(dl, 0, 0, fl.bo2[1], fl.bo2[2])
	  
	  if fl.bkgcolor then
		 dl_draw_box(dl,
					 0, 0, fl.bo2[1], fl.bo2[2], 0,
					 fl.bkgcolor[1],fl.bkgcolor[2],
					 fl.bkgcolor[3],fl.bkgcolor[4],
					 fl.bkgcolor[5],fl.bkgcolor[6],
					 fl.bkgcolor[7],fl.bkgcolor[8],
					 fl.bkgtype)
	  end

	  local i,y,h
	  local max=fl.top+fl.lines
	  if max > fl.dir.n then max = fl.dir.n end
	  y = fl.span + fl.border
	  h = fl.font_h+2*fl.span

	  for i=fl.top+1, max, 1 do
		 fl:draw_entry(dl, i, fl.border, y, 0.1)
		 fl.dir[i].y = y-fl.span
		 fl.dir[i].h = h
		 y = y + h
	  end

	  textlist_swap_dl(fl)
   end

   -- Textlist move cursor from 'mov'
   --
   function textlist_move_cursor(fl,mov)
	  local top,pos,action
	  action=0
	  top = fl.top
	  pos = fl.pos + mov
	  if pos >= fl.dir.n then pos = fl.dir.n-1 end
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
		 textlist_draw_list(fl)
		 action = action + 2
	  end
	  
	  if pos ~= fl.pos then
		 local y = fl.border + (pos-top) * (fl.font_h + 2*fl.span)
		 fl.cmtx = mat_trans(0,y,0.5)
		 fl.pos = pos
		 textlist_draw_cursor(fl)
		 action = action + 1
	  end
	  
	  return action
   end

   function textlist_get_entry()
	  if fl.dir.n < 1 then return end
	  local entry = fl.dir[fl.pos+1]
	  local i,v,e
	  e = {}
	  for i,v in entry do e[i] = v end
	  return e
   end

   function textlist_locate_entry_expr(fl,regexpr)
	  if fl.dir.n < 1 or not regexpr then return end
	  local i
	  for i=1, fl.dir.n, 1 do
		 if strfind(fl.dir[i].name,regexpr) then
			return fl:move_cursor(i-1-fl.pos)
		 end
	  end
   end

   function textlist_locate_entry(fl,name)
	  if fl.dir.n < 1 or not name then return end
	  local i
	  for i=1, fl.dir.n, 1 do
		 if fl.dir[i].name == name then
			return fl:move_cursor(i-1-fl.pos)
		 end
	  end
   end

   local fl

   fl = {
	  -- Control
	  dir = {},

	  -- Border size
	  border	= 3,
	  span	    = 1,
	  font_h	= 0,

	  -- Colors
	  filecolor = { 0.7, 1, 1, 1 },
	  dircolor  = { 1.0, 1, 1, 1 },
	  curcolor  = { 0.5, 1, 1, 0, 0.5, 0.5, 0.5, 0 },

	  -- Methods
	  confirm			= textlist_confirm,
	  move_cursor		= textlist_move_cursor,
	  get_entry		    = textlist_get_entry,
	  locate_entry_expr	= textlist_locate_entry_expr,
	  locate_entry		= textlist_locate_entry,
	  measure_text		= textlist_measure_text,
	  set_box			= textlist_set_box,
	  set_pos			= textlist_set_pos,
	  shutdown			= textlist_shutdown,
	  draw_entry		= textlist_draw_entry,
	  change_dir		= textlist_change_dir,
	  set_color			= textlist_set_color,
   }
   -- Set display box to min.
   fl:set_box(100,100,0,0,200)

   -- Overrides default by user's ones supply parms
   if flparm then
	  if flparm.dir then fl.dir = flparm.dir end

	  fl.flags = flparm.flags;
	  if flparm.pos then
		 fl:set_pos(flparm.pos[1], flparm.pos[2], flparm.pos[3])
	  end
	  
	  if flparm.lines     then fl.maxlines	= flparm.lines		end
	  if flparm.box		  then fl.minmax	= flparm.box		end
	  if flparm.confirm   then fl.confirm	= flparm.confirm 	end
	  if flparm.border    then fl.border	= flparm.border		end
	  if flparm.span 	  then fl.span		= flparm.span		end
	  if flparm.filecolor then fl.filecolor	= flparm.filecolor	end
	  if flparm.dircolor  then fl.dircolor	= flparm.dircolor	end
	  if flparm.bkgcolor  then fl.bkgcolor	= flparm.bkgcolor	end
	  if flparm.curcolor  then fl.curcolor	= flparm.curcolor	end
   end

   textlist_reset(fl)
   return fl
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

   function handle(app,evt)
	  local fl = app.fl
	  if key == evt_shutdown_event then
		 fl:shutdown()
		 app.done = 1
	  end
	  return evt
   end

   app = {	name	= "textlist",
	  version	= "0.1",
	  handle	= handle,
	  fl		= fl }
   evt_app_insert_first(owner, app)

   return app
end

function textlist_create_gui(fl, owner)
   if not fl or not owner then return nil end
   local app = textlist_create_app(fl,owner)
   if not app then return nil end

   -- textlist GUI application event handler.
   -- @internal
   --
   function handle(app,evt)
	  local key = evt.key
	  local fl = app.fl
	  local dir = nil
	  if fl then dir = fl.dir end

	  if key == evt_shutdown_event then
		 fl:shutdown()
		 app.done = 1
		 return evt
	  end

	  if dir.n < 1 or not gui_is_focus(app.owner,app) then
		 -- No dir loaded or no focus, ignore event --
		 return evt;
	  elseif gui_keyup[key] then
		 local code = fl:move_cursor(-1)
		 if code and code > 0 then
			evt_send(app.owner, { key = gui_item_change_event })
		 end
		 return
	  elseif gui_keydown[key] then
		 local code = fl:move_cursor(1)
		 if code and code > 0 then
			evt_send(app.owner, { key = gui_item_change_event })
		 end
		 return
	  elseif gui_keycancel[key] then
		 evt_send(app.owner, { key = gui_item_cancel_event })
		 return
	  elseif gui_keyconfirm[key] then
		 local action = fl:confirm()
		 if not action then return end
		 if action == 1 then
			evt_send(app.owner, { key = gui_item_confirm_event })
		 elseif action == 2 then
			evt_send(app.owner, { key = gui_item_change_event })
		 else
			evt_send(app.owner, { key = gui_item_change_event })
			evt_send(app.owner, { key = gui_item_confirm_event })
		 end
		 return -- ??? - $$$ just add this, was it forgot
	  end
	  return evt
   end

   app.z			= gui_guess_z(owner, nil)
   fl:set_pos(nil, nil, gui_guess_z(app))
   app.fl			= fl
   app.name	    	= "gui "..app.name
   app.box			= app.fl.box
   app.dl			= nil
   app.handle		= handle
   app.event_table = {}
   app.flags		= {}

   return app
end

--- Create textlist gui application.
--- @ingroup dcplaya_lua_textlist_gui
---
---
function gui_textlist(owner, flparm)
   if not owner then return nil end
   local fl = textlist_create(flparm)
   if not fl then return end
   return textlist_create_gui(fl, owner)
end

if nil then
   fl = gui_textlist(evt_desktop_app, { dir=dirlist("/") })
   getchar()
   evt_shutdown_app(fl)
end

textlist_loaded = 1
return textlist_loaded
