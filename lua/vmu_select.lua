--- @ingroup dcplaya_lua_gui
--- @file    vmu_select.lua
--- @author  benjamin gerard <ben@sashipa.com>
--- @date    2002/12/11
--- @brief   VMU selector gui.
--

vmu_select_loaded = nil
if not dolib("textlist") then return end
if not dolib("gui") then return end
if not dolib("sprite") then return end

--- Creates vmu sprites.
--- @internal
function vmu_select_create_sprite()
   local spr = sprite_get("vs_vmu")
   if spr then return spr end

   local texid = tex_exist("dcpsprites") or tex_new("/rd/dcpsprites.tga")
   local x1,y1,w,h = 109,65,104,63
   return sprite("vs_vmu",
		 w/2, h/2,
		 w, h,
		 x1/512, y1/128, (x1+w)/512, (y1+h)/128,
		 texid,1)
end

--- Create a vmu select application.
function vmu_select_create(owner, name, dir, x, y, z)

   -- Default
   owner = owner or evt_desktop_app
   name = name or "vmu select"

   -- VMU-select default style
   -- ------------------------
   local style = {
      bkg_color	= { 0.8, 0.7, 0.7, 0.7,  0.8, 0.3, 0.3, 0.3 },
      border	= 8,
      span      = 1,
      file_color= { 1, 1, 1, 0 },
      dir_color	= { 1, 1, .7, 0 },
      cur_color	= { 1, 1, 1, 1,  1, 0.1, 0.4, 0.5 },
      text      = {font=0, size=16, aspect=1}
   }

   --- VMU-select event handler.
   --- @internal
   function vmu_select_handle(dial,evt)
      local key = evt.key

      if key == gui_item_confirm_event then
	 local fl=dial.vs.fl
	 local result = fl:fullpath(fl:get_entry())
	 evt_shutdown_app(dial)
	 dial._done = 1
	 dial._result = result
	 return
      elseif key == gui_item_cancel_event then
	 evt_shutdown_app(dial)
	 dial._done = 1
	 dial._result = nil
	 return
      end
      return evt
   end

   -- Create sprite
   local vmusprite = vmu_select_create_sprite()
   if not vmusprite then return end

   local border = 8
   local w = vmusprite.w * 2 + 2 * (border + style.border)
   local h = vmusprite.h + 16 + 2 * (border + style.border) + 16
   local box = { 0, 0, w, h }
   local screenw, screenh = 640,480

   x = x or ((screenw - box[3])/2) 
   y = y or ((screenh - box[4])/2)
   local x2,y2 = x+box[3], y+box[4]
   
   -- Create dialog
   local dial
   dial = gui_new_dialog(owner,
			 {x, y, x2, y2 }, z, nil, name,
			 { x = "left", y = "up" } )
   dial.event_table = {
      [gui_item_confirm_event]	= vmu_select_handle,
      [gui_item_cancel_event]	= vmu_select_handle,
      [gui_item_change_event]	= vmu_select_handle,
   }

   box = box + { x+border, y+16+border, x-border, y-border }
   local w,h = box[3]-box[1], box[4]-box[2]

   dial.vs = gui_textlist(dial,
			  {
			     pos = {box[1], box[2]},
			     box = {w,h,w,h},
			     flags=nil,
			     filecolor = style.file_color,
			     dircolor  = style.dir_color,
			     bkgcolor  = style.bkg_color,
			     curcolor  = style.cur_color,
			     border    = style.border,
			     span      = style.span,
			  })

   if not dial.vs then
      print("vmu_select: error creating textlist-gui")
      return
   end

   -- Customize textlist
   local fl = dial.vs.fl
   fl.vmusprite = vmusprite
   fl.measure_text = function(fl, entry)
			local w, h = dl_measure_text(fl.cdl,fl:fullpath(entry))
			return max(w,fl.vmusprite.w),
			h+fl.vmusprite.h+2*fl.span
		     end

   fl.fullpath =  function (fl, entry)
		     if not entry then 
			entry = fl and fl.dir and fl.dir[(fl.pos or 0)+1]
			if not entry then return end
		     end
		     return ((entry.path and (entry.path.."/")) or "")
			.. entry.name
		  end

   fl.draw_entry = function (fl, dl, idx, x , y, z)
		      local entry = fl.dir[idx]
		      local name = fl:fullpath(entry)
		      local color = fl.dircolor
		      local wt,ht = dl_measure_text(dl,name)
		      x = fl.bo2[1] * 0.5 - fl.border
		      local xt = x - wt * 0.5
		      dl_draw_text(dl,
				   xt, y, z+0.1,
				   color[1],color[2],color[3],color[4],
				   name)
		      fl.vmusprite:draw(dl, x, y + ht, z)
		   end

   fl.draw_cursor = function () end
   
   fl:change_dir(dir or dirlist("-nh","/vmu"))

   return dial
end

vmu_select_create_sprite()

vmu_select_loaded = 1
return vmu_select_loaded
