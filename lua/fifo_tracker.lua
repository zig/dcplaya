--- @ingroup  dcplaya_lua_app
--- @file     fifo_tracker.lua
--- @author   vincent penne
--- @date     2003
--- @brief    fifo tracker application.
---
--- $Id: fifo_tracker.lua,v 1.5 2003-07-29 08:25:00 benjihan Exp $
---

fifo_tracker_loaded = nil

if not dolib("evt") then return end
if not dolib("menu") then return end

fifo_tracker_full_color = { 0.5,0.3,0.7,0 }
fifo_tracker_empty_color = { 0.5,0.7,0.7,0 }
fifo_tracker_text_color = { 1,0,0,0 }


--- Create a fifo tracker application.
function fifo_tracker_create(owner, name)
   local z
   owner = owner or evt_desktop_app
   name = name or "fifo tracker"

   --- Create fifo tracker menu
   function fifo_tracker_menucreator(target)
      local ft = target;
      if not ft then return end
      local cb = {
	 show = function(menu, idx)
		   local ft = menu.root_menu.target
		   if ft then
		      local old = ft.hidden_mode or 1
		      ft.hidden_mode = idx or 1
		      if old ~= ft.hidden_mode then
			 local label = { "normal", "never", "always" }
			 menu_yesno_image(menu, old, nil,  label[old], 1)
			 menu_yesno_image(menu, idx, 1, label[idx], nil)
		      end
		   end
		end,
      }
      ft.hidden_mode = ft.hidden_mode or 1

      local root = ":" .. target.name .. ":show >show"
      local showdef = {
	 root = ":show:"
	    .. menu_yesno_menu(ft.hidden_mode == 1,'normal','show') .. ","
	    .. menu_yesno_menu(ft.hidden_mode == 2,'never','show') .. ","
	    .. menu_yesno_menu(ft.hidden_mode == 3,'always','show')
      }
      local def = {
	 root = root,
	 cb = cb,
	 sub = { show = showdef },
      }
      return menu_create_defs(def , target)
   end

   --- volume control application avent handler.
   function fifo_tracker_handle(ft, evt)
      local key = evt.key

      if key == evt_shutdown_event then
	 local k,dl
	 for k,dl in { ft.dl_full, ft.dl_empty, ft.dl } do
	    dl_set_active(dl,0)
	    dl_clear(dl)
	 end
	 ft.dl_full = nil
	 ft.dl_empty = nil
	 return evt
      end

      return evt
   end

   function fifo_tracker_update(ft, frametime)
      local total = fifo_size()
      local a = fifo_used()
      local percent = a*1000/total

      if percent > 990 then
	 percent = 1000
      end

      
      if percent > 900 or percent == 0 then
	 ft.time_full = ft.time_full + frametime
      else
	 ft.time_full = 0
      end

      if percent < 100 then
	 ft.time_empty = ft.time_empty + frametime
      else
	 ft.time_empty = 0
      end

      local color = { 1, 1, 1, 1 }
      
      ft.hidden_mode = ft.hidden_mode or 1
      if ft.hidden_mode == 1 then
	 ft.hidden = ft.time_full > 0.5
      else
	 ft.hidden = ft.hidden_mode == 2
      end

      if ft.hidden then
	 if ft.alpha < 0.1 then
	    ft.alpha = 0
	    dl_set_active(ft.dl, 0)
	    return
	 else
	    ft.alpha = ft.alpha + (0 - ft.alpha) * frametime * 4
	 end
      else
	 if ft.alpha > 0.9 then
	    ft.alpha = 1
	 else
	    ft.alpha = ft.alpha + (1 - ft.alpha) * frametime * 8
	 end
      end

      if ft.time_empty > 1 then
	 local a = (ft.time_empty - 1) * 60 * 4
	 a = cos(a)*0.25 + 0.75
	 color[1] = a
	 color[3] = a
      end

      --      dl_set_color(ft.dl, color)
--      dl_set_color(ft.dl_full, color * fifo_tracker_full_color)
--      dl_set_color(ft.dl_empty, color * fifo_tracker_empty_color)
--      dl_set_color(ft.dl_text, color)
      dl_set_color(ft.dl, 
		   ft.alpha,
		   color[2],
		   color[3],
		   color[4]
		)
      dl_set_active(ft.dl, 1)

      local w = 200
      local x = 200 * a / total
      dl_set_trans(ft.dl_full, 
		   mat_scale(x, 30, 1)
		   *
		   mat_trans(320-w/2, 5, 0)
	     )

      local x = 200
      dl_set_trans(ft.dl_empty, 
		   mat_scale(x, 30, 1)
		   *
		   mat_trans(320-w/2, 5, 0)
	     )

      dl_clear(ft.dl_text)
      
      dl_text_prop(ft.dl_text,1,16)
      dl_draw_text(ft.dl_text, 320-w/2 + 40, 14, 3, 1, 1, 1, 1, 
		   format("%4d", percent))

      --print(x)
   end

   --- Draw volume bar at a given volume
   local ft = {
      -- Application
      name = name,
      version = 1.0,
      handle = fifo_tracker_handle,
      update = fifo_tracker_update,
      icon_name = "volume2",
      flags = { unfocusable = 1 },
      mainmenu_def = fifo_tracker_menucreator,

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
   dl_clear(ft.dl)
   local tw = 16
   dl_text_prop(ft.dl,0,tw)
   local title = "sound buffer"
   local w,h = dl_measure_text(ft.dl,title, 0, tw)

   dl_draw_text(ft.dl, (640-w)*0.5,-h,0, 1,1,1,1, title);
   dl_sublist(ft.dl, ft.dl_full)
   dl_sublist(ft.dl, ft.dl_empty)
   dl_sublist(ft.dl, ft.dl_text)
   dl_set_trans(ft.dl, mat_trans(200,40,0))
   dl_set_color(ft.dl, 0.5,1,1,1)

   dl_set_color(ft.dl_text, 1, 0, 0, 0)


   -- build bar
   dl_set_color(ft.dl_full, 
		fifo_tracker_full_color[1],
		fifo_tracker_full_color[2],
		fifo_tracker_full_color[3],
		fifo_tracker_full_color[4]
	     )
   dl_draw_box(ft.dl_full, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1)

   dl_set_color(ft.dl_empty, 
		fifo_tracker_empty_color[1],
		fifo_tracker_empty_color[2],
		fifo_tracker_empty_color[3],
		fifo_tracker_empty_color[4]
	     )
   dl_draw_box(ft.dl_empty, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1)


   dl_set_active(ft.dl, 1)
      
   evt_app_insert_last(owner, ft)

   fifo_tracker_kill()
   fifo_tracker = ft

   return ft
end

--
--- Kill a fifo tracker application.
---
---   The fifo_tracker_kill() function kill the given application by
---   calling sending the evt_shutdown_app() function. If the given
---   application is nil or fifo_tracker the default fifo tracker
---   (fifo_tracker) is killed and the global fifo_tracker is set
---   to nil.
---
--- @param  cc  application to kill (default to fifo_tracker)
--
function fifo_tracker_kill(cc)
   cc = cc or fifo_tracker
   if type(cc) == "table" then
      evt_shutdown_app(cc)
      if cc == fifo_tracker then
	 fifo_tracker = nil
	 print("fifo-tracker shutdowed")
      end
   end
end


-- Load application icon
local tex = tex_exist("volume2") or
   tex_new(home .. "lua/rsc/icons/volume2.tga")

fifo_tracker_loaded = 1
return fifo_tracker_loaded

