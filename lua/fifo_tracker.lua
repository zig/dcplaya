--- @ingroup  dcplaya_lua_app
--- @file     fifo_tracker.lua
--- @author   vincent penne
--- @date     2003
--- @brief    fifo tracker application.
---
--- $Id: fifo_tracker.lua,v 1.3 2003-04-21 20:25:08 vincentp Exp $
---

fifo_tracker_loaded = nil

if not dolib("evt") then return end

fifo_tracker_full_color = { 0.5,0.3,0.7,0 }
fifo_tracker_empty_color = { 0.5,0.7,0.7,0 }
fifo_tracker_text_color = { 1,0,0,0 }


--- Create a fifo tracker application.
function fifo_tracker_create(owner, name)
   local z
   owner = owner or evt_desktop_app
   name = name or "fifo tracker"

   --- volume control application avent handler.
   function fifo_tracker_handle(vc, evt)
      local key = evt.key

      if key == evt_shutdown_event then
	 local k,dl
	 for k,dl in { vc.dl_full, vc.dl_empty, vc.dl } do
	    dl_set_active(dl,0)
	    dl_clear(dl)
	 end
	 vc.dl_full = nil
	 vc.dl_empty = nil
	 return evt
      end

      return evt
   end

   function fifo_tracker_update(vc, frametime)
      local total = fifo_size()
      local a = fifo_used()
      local percent = a*1000/total

      if percent > 990 then
	 percent = 1000
      end

      
      if percent > 900 or percent == 0 then
	 vc.time_full = vc.time_full + frametime
      else
	 vc.time_full = 0
      end

      if percent < 100 then
	 vc.time_empty = vc.time_empty + frametime
      else
	 vc.time_empty = 0
      end

      local color = { 1, 1, 1, 1 }
      if vc.time_full > 0.5 then
	 if vc.alpha < 0.1 then
	    vc.alpha = 0
	    dl_set_active(vc.dl, 0)
	    return
	 else
	    vc.alpha = vc.alpha + (0 - vc.alpha) * frametime * 4
	 end
      else
	 if vc.alpha > 0.9 then
	    vc.alpha = 1
	 else
	    vc.alpha = vc.alpha + (1 - vc.alpha) * frametime * 8
	 end
      end

      if vc.time_empty > 1 then
	 local a = (vc.time_empty - 1) * 60 * 4
	 a = cos(a)*0.25 + 0.75
	 color[1] = a
	 color[3] = a
      end

      --      dl_set_color(vc.dl, color)
--      dl_set_color(vc.dl_full, color * fifo_tracker_full_color)
--      dl_set_color(vc.dl_empty, color * fifo_tracker_empty_color)
--      dl_set_color(vc.dl_text, color)
      dl_set_color(vc.dl, 
		   vc.alpha,
		   color[2],
		   color[3],
		   color[4]
		)
      dl_set_active(vc.dl, 1)

      local w = 200
      local x = 200 * a / total
      dl_set_trans(vc.dl_full, 
		   mat_scale(x, 30, 1)
		   *
		   mat_trans(320-w/2, 5, 0)
	     )

      local x = 200
      dl_set_trans(vc.dl_empty, 
		   mat_scale(x, 30, 1)
		   *
		   mat_trans(320-w/2, 5, 0)
	     )

      dl_clear(vc.dl_text)
      
      dl_text_prop(vc.dl_text,1,16)
      dl_draw_text(vc.dl_text, 320-w/2 + 40, 14, 3, 1, 1, 1, 1, 
		   format("%4d", percent))

      --print(x)
   end

   --- Draw volume bar at a given volume
   local vc = {
      -- Application
      name = name,
      version = 1.0,
      handle = fifo_tracker_handle,
      update = fifo_tracker_update,
      icon_name = "volume2",
      flags = { unfocusable = 1 },

      -- Members
      z = gui_guess_z(owner,z),
      dl = dl_new_list(128, 0),
      dl_full = dl_new_list(128, 1, 1),
      dl_empty = dl_new_list(128, 1, 1),
      dl_text = dl_new_list(1024, 1, 1),

      time_full = 0,
      time_empty = 0,
      alpha = 0,
   }

   -- Main display list
   dl_clear(vc.dl)
   local tw = 16
   dl_text_prop(vc.dl,0,tw)
   local title = "sound buffer"
   local w,h = dl_measure_text(vc.dl,title, 0, tw)

   dl_draw_text(vc.dl, (640-w)*0.5,-h,0, 1,1,1,1, title);
   dl_sublist(vc.dl, vc.dl_full)
   dl_sublist(vc.dl, vc.dl_empty)
   dl_sublist(vc.dl, vc.dl_text)
   dl_set_trans(vc.dl, mat_trans(200,40,0))
   dl_set_color(vc.dl, 0.5,1,1,1)

   dl_set_color(vc.dl_text, 1, 0, 0, 0)


   -- build bar
   dl_set_color(vc.dl_full, 
		fifo_tracker_full_color[1],
		fifo_tracker_full_color[2],
		fifo_tracker_full_color[3],
		fifo_tracker_full_color[4]
	     )
   dl_draw_box(vc.dl_full, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1)

   dl_set_color(vc.dl_empty, 
		fifo_tracker_empty_color[1],
		fifo_tracker_empty_color[2],
		fifo_tracker_empty_color[3],
		fifo_tracker_empty_color[4]
	     )
   dl_draw_box(vc.dl_empty, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1)


   dl_set_active(vc.dl, 1)
      
   evt_app_insert_last(owner, vc)

   if fifo_tracker then
      evt_shutdown_app(fifo_tracker)
   end
   fifo_tracker = vc

   return vc
end

-- Load application icon
local tex = tex_exist("volume2") or
   tex_new(home .. "lua/rsc/icons/volume2.tga")

fifo_tracker_loaded = 1
return fifo_tracker_loaded

