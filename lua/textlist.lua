--- @ingroup dcplaya_lua_textlist Text
--- @file    textlist.lua
--- @author  benjamin gerard <ben@sashipa.com>
--- @date    2002/10/04
--- @brief   Manage and display a list of text.
---
--- $Id: textlist.lua,v 1.40 2003-03-14 22:04:50 ben Exp $
---

-- DL hierarchy :
-- dl (main display list)
--  +-- textprop
--  +-- bdl (background sublist)
--  +-- ldl (sublist for item ans cursor)
--        +-- clipping Y only (allow cursor text outside texlist box!)
--        +-- cdl (cursor display list)
--              +-- cursor-box
--              +-- text or tag-text
--        +-- clipping XY (item will be clipped in box)
--        +-- idl (item sublist)
--              +-- lotsa text  ot tagged-text
--              +-- ... etc ...
--        +-- udl (user sublist)
--
-- space organization :
-- dl global textlist Z := [0..100]
-- Z : [00..25] reserved for background
-- Z : [25..75] reserved for items and cursor
-- Z : [75..100] reserved for user overlay
--
-- Each sub-list as a transformation setted properly. This mean than each
-- sub-list (bdl,cdl,idl,udl) as a useable Z space range [0..100]
--

--
-- bdl background dl. Z:[00..25]
-- mat(bdl) = mat_scale(1,1,0.25) * mat(dl)
--
-- ldl item and cursor Z[25..75]
-- mat(ldl) = mat_scale(1,1,0.5) * mat_trans(0,0,25) * mat(dl)
--
-- idl item sublist Z:[25..50]
-- mat(idl) = mat_scale(1,1,0.5) * mat(ldl)
--
-- cdl item sublist Z:[50..75]
-- mat(idl) = mat_scale(1,1,0.5) * mat_trans(0,0,25) * mat(ldl)
--


-- Unload the library
textlist_loaded = nil


-- z shift of textlist entry text
textlist_entry_z = 25
textlist_cursor_z = 25


-- Load required libraries
if not dolib("basic") then return end
if not dolib("taggedtext") then return end

--- @defgroup dcplaya_lua_textlist Text-list GUI
--- @ingroup  dcplaya_lua_gui
--- @brief    Text list graphic browser.

--- Entry displayed in textlist.
--- @ingroup dcplaya_lua_textlist
---
--- struct flentry {
---   name; ///< Displayed text
---   size; ///< Optionnal. -1 for highlight (directories).
--- };

--- textlist object definition.
--- @ingroup dcplaya_lua_textlist
---
--- struct textlist {
---
---   flentry dir[];     ///< Current fllist object
---   table dirinfo[];   ///< dir supplemental information
---   pos;       ///< Current select item
---   top;       ///< Top displayed line
---   lines;     ///< Computed max displayed lines
---   maxlines;  ///< Real maximum od displayed lines
---   n;         ///< Number of entry in "dir"
---   dl;        ///< Current display list
---   cdl;       ///< Cursor display list
---
---   /** Confirm callback.
---    *  This method is call when confirm button is pressed.
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

--- Create a textlist object.
--- @ingroup dcplaya_lua_textlist
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
   if not flparm then flparm = {} end

   --- Change textlist position and size.
   --- @ingroup dcplaya_lua_textlist
   ---
   --- @param  fl  textlist.
   --- @param  x   New horizontal position of textlist or nil.
   --- @param  y   New vertical position of textlist or nil.
   --- @param  w   New width of textlist box or nil.
   --- @param  h   New height of textlist box or nil.
   --- @param  z   New depth of textlist box or nil.
   --- @param  owner Owner application
   ---
   function textlist_set_box(fl,x,y,w,h,z)
      if not fl.box then fl.box = {} end
      if not fl.bo2 then fl.bo2 = {} end

      -- box {[1]:x1 [2]:y1 [3]:x2 [4]:y2} bo2 { [1]:w [2]:h [3]:z}
      x = x or fl.box[1]
      y = y or fl.box[2]
      w = w or fl.bo2[1]
      h = h or fl.bo2[2]
      z = z or fl.bo2[3]

      if fl.minmax then
	 if 	w < fl.minmax[1] then w = fl.minmax[1]
	 elseif	w > fl.minmax[3] then w = fl.minmax[3] end
	 if	h < fl.minmax[2] then h = fl.minmax[2]
	 elseif	h > fl.minmax[4] then h = fl.minmax[4] end
      end
      
      -- $$$ ben : screen size should be in variable one day !
      local sx1,sy1,sx2,sy2 = 40, 40, 640-40, 480-40
      if intop(fl.allow_out,'&',1) == 0 then
	 if x + w > sx2 then x = sx2 - w end
	 if x < sx1 then x = sx1 end
      end 
      if intop(fl.allow_out,'&',2) == 0 then
	 if y + h > sy2 then y = sy2 - h end
	 if y < sy1 then y = sy1 end 
      end
      -- $$$ ben : No test for box larger than screen. May be should it be ?
      -- Anyway I have added a minmax box that should avoid it by default
      -- but it could be override by user provided minmax box.
      
      fl.bo2[1] = w
      fl.bo2[2] = h
      fl.bo2[3] = z
      fl.box[1] = x
      fl.box[2] = y
      fl.box[3] = x + w
      fl.box[4] = y + h

      if fl.dl then
	 dl_set_trans(fl.dl, mat_trans(x,y,z))
      end

      --	  dump(fl.box,"set_box")

      --	  fl.lines  = floor( (fl.bo2[2]-2*fl.border) / (fl.font_h+2*fl.span))
      --	  fl.lines = clip_value(fl.lines,0,fl.maxlines)
   end

   --- Change textlist position.
   --- @ingroup dcplaya_lua_textlist
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
   end

   --- Set textlist color.
   --- @ingroup dcplaya_lua_textlist
   --- 
   --- @param  fl  textlist.
   --- @param  a   New alpha component or nil
   --- @param  r   New red component or nil
   --- @param  g   New green component or nil
   --- @param  b   New blue component or nil
   ---
   function textlist_set_color(fl,a,r,g,b)
      dl_set_color(fl.dl, a, r, g, b)
   end

   --- Open textlist (fade in).
   --
   function textlist_open(fl)
      fl.closed = nil
      fl:fade(fl.fade_max or 1)
      dl_set_active(fl.dl, 1)
   end

   --- Close textlist (fade out).
   --
   function textlist_close(fl)
      fl.closed = 1
      fl:fade(fl.fade_min or 0)
   end

   function textlist_fade(fl, to, speed)
      fl.fade_to  = to
      fl.fade_spd = abs(speed or fl.fade_spd or 2)
   end

   --- Center a textlist in a box.
   --- @ingroup dcplaya_lua_textlist
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

   --- Default confirm callback function.
   --
   function textlist_confirm(fl)
      if not fl or fl.dir.n < 1 then return end
      return 1
   end

   --- Textlist shutdown.
   ---
   function textlist_shutdown(fl)
      local i,v
      for i,v in { "dl","bdl","ldl","idl","cdl","udl" } do
	 if tag(fl[v]) == dl_tag then
	    dl_set_active(fl[v],0)
	    dl_clear(fl[v])
	    fl[v] = nil
	 end
      end
   end

   --- Measure dimension of the whole textlist and set dirinfo.
   --
   function textlist_measure(fl)
      if fl.dir.n < 1 then return 0,0 end

      -- Measure text --
      local i, w, h, y
      y,w,h = 0,0,0;
      for i=1, fl.dir.n, 1 do
	 local w2,h2
	 w2,h2 = fl:measure_text(fl.dir[i])
	 if w2 > w then w = w2 end
	 h = h + h2
	 fl.dirinfo[i] = {}
	 fl.dirinfo[i].y = y
	 fl.dirinfo[i].w = w2
	 fl.dirinfo[i].h = h2
	 y = y + h2
      end
      return w, h
   end

   --- Measure an entry.
   function textlist_measure_text(fl, entry)
      if not fl.not_use_tt then
	 local tt = tt_build(entry.name,
			     { border = {0,0}, x = "left", y = "up" })
	 entry.tt = tt
	 --tt.box = { x, y, x+info.w, y+info.h }
	 --	  tt.dl = dl
	 tt_draw(tt)
	 return tt.total_w, tt.total_h + 2*fl.span
      else
	 local w,h = dl_measure_text(fl.dl, entry.name)
	 return w, h+2*fl.span
      end
   end

   --- Draw given textlist entry in given diplay list.
   --
   function textlist_draw_entry(fl, dl, idx, x , y, z)
      dl = dl or fl.idl
      local entry = fl.dir[idx]
      local info = fl.dirinfo[idx]
      local color = fl.dircolor
      if entry.size and entry.size >= 0 then
	 color = fl.filecolor
      end
      local tt = entry.tt
      if tt then
--	 print("textlist TT:"..idx)
	 dl_set_trans(tt.dl, mat_trans(x, y, z))
	 dl_set_color(tt.dl, color)
	 dl_sublist(dl, tt.dl)
      else
--	 print("textlist:"..idx..":"..entry.name)
	 return dl_draw_text(dl,
			     x, y, z,
			     color[1],color[2],color[3],color[4],
			     entry.name)
      end
   end

   --- Insert an entry.
   function textlist_insert_entry(fl, entry, pos)
      local y = 0
      if not entry then return end
      if not pos then
	 local n = fl.dir.n
	 if n > 0 then
	    y = fl.dirinfo[n].y + fl.dirinfo[n].h
	 end
	 local w,h = fl:measure_text(entry)
	 tinsert(fl.dir, entry)
	 tinsert(fl.dirinfo, { y=y, w=w, h=h })
	 fl:draw()
      else
	 print("TEXTLIST INSERT NOT IMPLEMENTED")
      end
   end

   function textlist_remove_entry(fl, pos)
      if not fl.dir then return end
      pos = pos or (fl.pos+1)
      local n = fl.dir.n
      if pos <= 0 or pos > n then return end
      if type(fl.dir) == "table" then
	 tremove(fl.dir, pos)
      else
	 fl.dir[pos] = nil --- $$$ For entry list !
      end
      local newpos = fl.pos - ((pos-1 < fl.pos) or 0)
      fl:change_dir(fl.dir, newpos + 1)
      return pos
   end

   --- Reset textlist.
   --
   function textlist_reset(fl)
      local w,h

      -- Control
      fl.pos		= 0
      -- 	  fl.top		= 0

      -- $$$ Avoid going outside screen
      if not fl.minmax then
	 fl.minmax = {0,0,600,400}
      end

      -- Display lists :

      -- Main-list
      fl.dl = fl.dl or dl_new_list(128);
      dl_set_active(fl.dl,0)
      dl_clear(fl.dl)

      -- added by Vincent
      if fl.owner and fl.owner.dl then
	 dl_sublist(fl.owner.dl, fl.dl)
      end


      -- background sub-list
      fl.bdl = fl.bdl or dl_new_list(256,1,1)
      dl_clear(fl.bdl)
      dl_set_trans(fl.bdl, mat_scale(1,1,0.25))

      -- cursor and item sub-list
      fl.ldl = fl.ldl or dl_new_list(64,1,1)
      dl_clear(fl.ldl)
      dl_set_trans(fl.ldl, mat_scale(1,1,0.5) *
		   mat_trans(fl.border,fl.border,25))

      -- Item sub-list
      fl.idl = fl.idl or dl_new_list(1024,1,1)
      dl_clear(fl.idl)
      dl_set_trans(fl.idl, mat_scale(1,1,0.5))

      -- Cursor sub-list
      fl.cdl = fl.cdl or dl_new_list(512,1,1)
      dl_set_active(fl.cdl,0)
      dl_clear(fl.cdl)
      dl_set_trans(fl.cdl,
		   mat_scale(1,1,0.5) * mat_trans(0,0,50))

      -- User layer sub-list
      fl.udl = fl.udl or dl_new_list(64,1,0)
      dl_set_active(fl.udl,0)
      dl_clear(fl.udl)
      dl_set_trans(fl.udl, mat_scale(1,1,0.25) * mat_trans(0,0,75))

      fl:set_color(0,1,1,1)
      fl:change_dir(fl.dir)
      fl:open()
   end

   --- Change the textlist content.
   --
   function textlist_change_dir(fl, dir, pos)
      dir = dir or {}
      if not dir.n then dir.n = getn(dir) or 0 end
      fl.dir = dir
      fl.dirinfo = {}

      -- Measure new list.
      local w,h = textlist_measure(fl)
      fl:set_box(nil,nil,
		 2*fl.border + w,
		 2*fl.border + h, nil)
      pos = (pos or 1) - 1
      pos = (pos < dir.n and pos) or (dir.n - 1)
      fl.pos = (pos >= 0 and pos) or 0
      fl:draw()
      return 1
   end


   --- @name Textlist draw methods.
   --- @{
   --

   --- Draw textlist.
   --
   function textlist_draw(fl)
      textlist_draw_main(fl)
      fl:draw_list(fl.idl)
      fl:draw_cursor(fl.cdl)
   end

   --- Draw main list.
   --
   function textlist_draw_main(fl)

      -- Redraw all
      dl_clear(fl.dl)
      dl_text_prop(fl.dl, 0, 16, 1)
      fl:draw_background(fl.bdl)
      dl_sublist(fl.dl,fl.bdl)
      dl_clear(fl.ldl)

      local ww, wh
      ww = fl.bo2[1]-2*fl.border
      wh = fl.bo2[2]-2*fl.border
      if (ww <= 0) then ww = 1 end
      if (wh <= 0) then wh = 1 end

      dl_set_clipping(fl.ldl,0,0,0,wh)
      dl_sublist(fl.ldl,fl.cdl)
      dl_set_clipping(fl.ldl,0,0,ww,wh)
      dl_sublist(fl.ldl,fl.idl)
      dl_sublist(fl.dl,fl.ldl)
      dl_sublist(fl.dl,fl.udl)
   end

   --- Draw background box.
   --
   function textlist_draw_background(fl, dl)
      dl = dl or fl.bdl
      if fl.bkgcolor then
	 dl_draw_box(dl,
		     0, 0, fl.bo2[1], fl.bo2[2], 25,
		     fl.bkgcolor[1],fl.bkgcolor[2],
		     fl.bkgcolor[3],fl.bkgcolor[4],
		     fl.bkgcolor[5],fl.bkgcolor[6],
		     fl.bkgcolor[7],fl.bkgcolor[8],
		     fl.bkgtype)
      end
   end

   --- Draw textlist cursor.
   --
   function textlist_draw_cursor(fl, dl)
      dl = dl or fl.cdl
      if fl.dir.n < 1 then
	 dl_set_active(dl,0)
	 return
      end
      dl_clear(dl)
      local i = (fl.pos or 0) + 1
      if not fl.dirinfo or not fl.dirinfo[i] then
	 -- $$$ There is a bug here ...
	 print("-------------------------")
	 print("-------------------------")
	 print("textlist draw_cursor BUGS:")
	 print("fl.pos="..tostring(fl.pos))
	 print("fl.dirinfo="..tostring(fl.dirinfo))
	 if fl.dirinfo then
	    print("fl.dirinfo[fl.pos]="..tostring(fl.dirinfo[i]))
	 end
	 print("-------------------------")
	 print("-------------------------")
	 return
      end

      local y,w,h = fl.dirinfo[i].y, fl.dirinfo[i].w, fl.dirinfo[i].h
      local ww = fl.bo2[1] - 2 * fl.border
      if ww > w then w = ww end
      
      dl_draw_box(dl,
		  0, y, w, y+h, 33,
		  fl.curcolor[1],fl.curcolor[2],fl.curcolor[3],fl.curcolor[4],
		  fl.curcolor[5],fl.curcolor[6],fl.curcolor[7],fl.curcolor[8])
      fl:draw_entry(dl, i, 0, y+fl.span, 66)
      -- display to vmu
      vmu_set_text(fl:get_text(i))
      dl_set_active(dl,1)
   end

   --- Draw entries.
   --
   function textlist_draw_list(fl, dl)
      dl = dl or fl.idl
      local i
      dl_clear(dl)
      --- $$$ Added by ben for updating TT drawing
      textlist_measure(fl)
      local max = getn(fl.dirinfo)
      for i=1, max, 1 do
	 fl:draw_entry(dl, i, 0, fl.dirinfo[i].y+fl.span, 50)
      end
   end

   --- @}

   --- Move textlist cursor.
   --
   function textlist_move_cursor(fl,mov)
      local pos

      pos = fl.pos + mov
      if pos >= fl.dir.n then pos = fl.dir.n-1 end
      if pos < 0 then pos = 0 end

      if pos ~= fl.pos then
	 fl.pos = pos
	 fl:draw_cursor(fl.cdl)
	 return 2
      end
      
      return nil
   end

   --- Get a copy of the current entry.
   --
   function textlist_get_entry(fl)
      if not fl.dir or (fl.dir.n or 0) < 1 then return end
      local eentry = fl.dir[fl.pos+1]
      local xentry = fl.dirinfo and fl.dirinfo[fl.pos+1]
      local i,v,e
      e = {}
      if type(xentry) == "table" then
	 for i,v in xentry do e[i] = v end
      end
      if type(eentry) == "table" then
	 for i,v in eentry do e[i] = v end
	 return e
      end
   end

   function textlist_get_pos(fl,pos)
      if not fl.dir then return end
      pos = pos or ((fl.pos or -1) + 1)
      return pos > 0 and pos <= (fl.dir.n or 0) and pos
   end

   function textlist_get_text(fl,pos)
      pos = fl:get_pos(pos)
      if pos then
	 local entry = fl.dir[pos]
-- 	 local tt = entry.tt
-- 	 if tt and tt.mode and tt.mode.text_nude then
-- 	    return tt.mode.text_nude
-- 	 else
	    return entry.name or entry.file
-- 	 end
      end
   end

   function textlist_get_path(fl, entry)
      if not entry then 
	 entry = fl and fl.dir and fl.dir[(fl.pos or 0)+1]
	 if not entry then return end
      end
      local path = entry.path or (fl and fl.path) or "/"
      return type(path) == "string" and  canonical_path(path)
   end

   --- Get entry fullpath
   --
   function textlist_fullpath(fl, entry)
      if not entry then 
	 entry = fl and fl.dir and fl.dir[(fl.pos or 0)+1]
	 if not entry then return end
      end
      local leaf = entry.file or entry.name
      local path = fl:get_path(entry)
      if type(leaf) == "string" and type(path) == "string" then
	 return canonical_path(path.."/"..leaf)
      end
   end

   --- Locate an entry which name matches a regular expression.
   --
   function textlist_locate_entry_expr(fl,regexpr)
      if fl.dir.n < 1 or not regexpr then return end
      local i
      for i=1, fl.dir.n, 1 do
	 if strfind(fl.dir[i].name,regexpr) then
	    return fl:move_cursor(i-1-fl.pos)
	 end
      end
   end

   --- Locate an entry from its name.
   --
   function textlist_locate_entry(fl,name)
      if fl.dir.n < 1 or not name then return end
      local i
      for i=1, fl.dir.n, 1 do
	 if fl.dir[i].name == name then
	    return fl:move_cursor(i-1-fl.pos)
	 end
      end
   end


   --- Get screen TOP/LEFT coordinates of an entry.
   --
   function textlist_screen_coor(fl,pos)
      pos = fl:get_pos(pos)
      if not pos then return end
      local xentry = fl.dirinfo[pos]
      local ye = (xentry and xentry.y) or 0
      local m1,m2 = dl_get_trans(fl.dl), dl_get_trans(fl.cdl)
      return m1[4][1], m1[4][2] + m2[4][2] + ye
   end

   local fl

   fl = {
      -- Control
      dir = {},
      dirinfo = {},

      -- Border size
      border	= 3,
      span      = 1,
      -- 	  font_h	= 0,

      -- Colors
      filecolor = { 0.7, 1, 1, 1 },
      dircolor  = { 1.0, 1, 1, 1 },
      curcolor  = { 0.5, 1, 1, 0, 0.5, 0.5, 0.5, 0 },

      -- Methods
      confirm		= textlist_confirm,
      move_cursor	= textlist_move_cursor,
      get_entry		= textlist_get_entry,
      get_pos           = textlist_get_pos,
      get_text          = textlist_get_text,
      fullpath          = textlist_fullpath,
      get_path          = textlist_get_path,
      locate_entry_expr	= textlist_locate_entry_expr,
      locate_entry	= textlist_locate_entry,
      insert_entry      = textlist_insert_entry,
      remove_entry      = textlist_remove_entry,
      measure_text	= textlist_measure_text,
      set_box		= textlist_set_box,
      set_pos		= textlist_set_pos,
      shutdown		= textlist_shutdown,
      draw_list         = textlist_draw_list,
      draw_background   = textlist_draw_background,
      draw_cursor       = textlist_draw_cursor,
      draw_entry	= textlist_draw_entry,
      draw              = textlist_draw,
      change_dir	= textlist_change_dir,
      set_color		= textlist_set_color,
      open              = textlist_open,
      close             = textlist_close,
      fade              = textlist_fade,
      update            = textlist_update,
      not_use_tt        = flparm.not_use_tt
   }

   -- Set display box to min.
   fl:set_box(100,100,0,0,0)

   -- Overrides default by user's ones supply parms
   if flparm then
      if flparm.dir then fl.dir = flparm.dir end

      fl.flags = flparm.flags;
      if flparm.pos then
	 fl:set_pos(flparm.pos[1], flparm.pos[2], flparm.pos[3])
      end
      
      if flparm.lines     then fl.maxlines	= flparm.lines		end
      if flparm.box	  then fl.minmax	= flparm.box		end
      if flparm.confirm   then fl.confirm	= flparm.confirm 	end
      if flparm.border    then fl.border	= flparm.border		end
      if flparm.span 	  then fl.span		= flparm.span		end
      if flparm.filecolor then fl.filecolor	= flparm.filecolor	end
      if flparm.dircolor  then fl.dircolor	= flparm.dircolor	end
      if flparm.bkgcolor  then fl.bkgcolor	= flparm.bkgcolor	end
      if flparm.curcolor  then fl.curcolor	= flparm.curcolor	end
      if flparm.allow_out then fl.allow_out     = flparm.allow_out      end
      fl.owner = flparm.owner
   end

   textlist_reset(fl)
   return fl
end

--- Update function.
--
function textlist_update(fl, frametime)

   if fl.fade_to then
      local fade = fl.fade_spd * frametime
      local a,r,g,b = dl_get_color(fl.dl)
      if a > fl.fade_to then
	 a = a - fade
	 if a <= fl.fade_to then
	    a = fl.fade_to
	    fl.fade_to = nil
	 end
      else
	 a = a + fade
	 if a >= fl.fade_to then
	    a = fl.fade_to
	    fl.fade_to = nil
	 end
      end
      
      if not fl.fade_to then
	 dl_set_active(fl.dl, a > 0)
	 fl.closed = fl.closed and 2
      end
      fl:set_color(a,r,g,b)
   end

   local i
   i = fl.pos+1
   if type(fl.dirinfo) == "table" and fl.dirinfo[i] then
      local y,h = fl.dirinfo[i].y, fl.dirinfo[i].h
      local wh = fl.bo2[2] - 2 * fl.border
      local mat = dl_get_trans(fl.cdl) 

      local ytop = -mat[4][2]
      local ytop2 = ytop
      if wh < 0 then wh = 0 end
      if y < ytop then
	 ytop = y
      elseif y+h > ytop+wh then
	 ytop = y+h-wh
      end
      if abs(ytop-ytop2) > 0.01 then
	 ytop = ytop * 0.1 + ytop2 * 0.9
      end
      mat[4][2] = -ytop
      dl_set_trans(fl.cdl, mat)
      mat = dl_get_trans(fl.idl) 
      mat[4][2] = -ytop
      dl_set_trans(fl.idl, mat)
   end

end

--- Create a textlist applcation from a textlist object.
--- @ingroup dcplaya_lua_textlist
---
--- @param  fl     textlist
--- @param  owner  Parent of the created application
---
--- @return application.
--- @retval nil error
---
function textlist_create_app(fl,owner)

   --- Default textlist update handler.
   --- @internal
   --
   function textlist_app_update(app, frametime)
      app.fl:update(frametime)
   end

   --- Default textlist application event handler.
   --- @internal
   --
   function textlist_app_handle(app,evt)
      -- call the standard dialog handle (manage child autoplacement)
      evt = gui_dialog_basic_handle(app, evt)
      if not evt then
	 return
      end

      local fl = app.fl
      if key == evt_shutdown_event then
	 fl:shutdown()
	 app.done = 1
      end
      return evt
   end

   app = {
      name      = "textlist",
      version	= "0.1",
      handle	= textlist_app_handle,
      update    = textlist_app_update,
      fl	= fl,
      dl        = owner.dl
   }
   evt_app_insert_first(owner, app)

   -- added by Vincent
   if app.dl then
      dl_sublist(app.dl, fl.dl)
   end

   fl.owner = app

   return app
end

function textlist_create_gui(fl, owner)
   if not fl or not owner then return nil end
   local app = textlist_create_app(fl,owner)
   if not app then return nil end

   --- textlist GUI application event handler.
   --- @internal
   --
   function textlist_gui_handle(app,evt)
      -- call the standard dialog handle (manage child autoplacement)
      evt = gui_dialog_basic_handle(app, evt)
      if not evt then
	 return
      end

      local key = evt.key
      local fl = app.fl
      local dir = fl.dir

      if key == evt_shutdown_event then
	 fl:shutdown()
	 app.done = 1
	 return evt
      end

      if dir.n < 1 or not gui_is_focus(app) then
	 -- No dir loaded or no focus, ignore event --
	 --  		 print("NOFOCUUUUS:", dir.n)
	 return evt;
      elseif gui_keyup[key] then
	 local code = fl:move_cursor(-1)
	 if code and code > 0 then
	    evt_send(app.owner,
		     { key = gui_item_change_event, app = fl, pos = fl.pos })
	 end
	 return
      elseif gui_keydown[key] then
	 local code = fl:move_cursor(1)
	 if code and code > 0 then
	    evt_send(app.owner,
		     { key = gui_item_change_event, app = fl, pos = fl.pos })
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
	 return -- ??? - $$$ just add this, was it forgotten ?
      end
      return evt
   end

   app.z		= 0 --gui_guess_z(owner, nil)
   fl:set_pos(nil, nil, 0) --gui_guess_z(app))
   app.fl		= fl
   app.name	    	= "gui "..app.name
   app.box		= app.fl.box
   app.handle           = textlist_gui_handle
   app.event_table      = {}
   app.flags		= {}

   return app
end

--- Create textlist gui application.
--- @ingroup dcplaya_lua_textlist
--
function gui_textlist(owner, flparm)
   if not owner then return nil end
   local fl = textlist_create(flparm)
   if not fl then return end
   return textlist_create_gui(fl, owner)
end

if nil then
   fl = gui_textlist(evt_desktop_app, { dir=dirlist("/") })
   if fl then 
      getchar()
      evt_shutdown_app(fl)
   end
end

textlist_loaded = 1
return textlist_loaded
