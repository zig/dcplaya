--- @file   desktop.lua
--- @author Vincent Penne <ziggy@sashipa.com>
--- @brief  desktop application
---
--- $Id: desktop.lua,v 1.6 2002-12-17 23:34:32 zigziggy Exp $
---

if not dolib("evt") then return end
if not dolib("gui") then return end
if not dolib("textlist") then return end
if not dolib("sprite") then return end

dskt_keytoggle = { 
   [KBD_KEY_PRINT] = 1, 
   [KBD_CONT1_Y] = 1, 
   [KBD_CONT2_Y] = 1, 
   [KBD_CONT3_Y] = 1, 
   [KBD_CONT4_Y] = 1
}


function dskt_create_sprites(vs)
   vs.sprites = {}
end

function dskt_switcher_create(owner, name, dir, x, y, z)

   -- Default
   owner = owner or evt_desktop_app
   name = name or "app_switcher"

   -- application switcher default style
   -- ------------------------
   local style = {
      bkg_color	= { 0.8, 0.7, 0.7, 0.7,  0.8, 0.3, 0.3, 0.3 },
      border	= 8,
      span      = 1,
      file_color= { 1, 0, 0, 0 },
      dir_color	= { 1, 0, 0, .4 },
      cur_color	= { 1, 1, 1, 1,  1, 0.1, 0.4, 0.5 },
      text      = { font=0, size=16, aspect=1 }
   }

   --- application switcher event handler.
   --
   function dskt_switcher_handle(dial, evt)
      local key = evt.key

      if dskt_keytoggle[key] then
	 evt_shutdown_app(dial)
	 return
      end

      if key == gui_item_confirm_event then
	 local result =  dial.vs.fl:get_entry()
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

      if key == gui_item_change_event then
	 local dir = dial.dir
--	 print("dir = ", dir)
	 if dir then
	    local a = dir[evt.pos+1].app
	    gui_new_focus(evt_desktop_app, a)
	    gui_new_focus(evt_desktop_app, dial)
	 end
      end

      return gui_dialog_handle(dial, evt)
   end

   -- Create sprite
   local texid = tex_get("dcpsprites") or tex_new("/rd/dcpsprites.tga")
   local vmusprite = sprite("vmu",	
			    0, 62/2,
			    104, 62,
			    108/512, 65/128, 212/512, 127/128,
			    texid,1)

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
			 { x, y, x2, y2 }, z, nil, name,
			 { x = "left", y = "up" }, "app_switcher" )
   dial.handle = dskt_switcher_handle

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
      print("dskt_switcher: error creating textlist-gui")
      return
   end

   -- Customize textlist
   local fl = dial.vs.fl
   fl.vmusprite = vmusprite
   fl.measure_text = function(fl, entry)
			local w, h = dl_measure_text(fl.cdl,entry.name)
			return max(w,fl.vmusprite.w),
			h+fl.vmusprite.h+2*fl.span
		     end

   fl.draw_entry = function (fl, dl, entry, x , y, z)
		      local color = fl.dircolor
		      local wt,ht = dl_measure_text(dl,entry.name)
		      x = fl.bo2[1] * 0.5 - fl.border
		      local xt = x - wt * 0.5
		      dl_draw_text(dl,
				   xt, y, z+0.1,
				   color[1],color[2],color[3],color[4],
				   entry.name)
		      fl.vmusprite:draw(dl, x, y + ht, z)
		   end

   fl.draw_cursor = function () end

   dial.dir = dir or { }
   fl:change_dir(dial.dir)

   return dial
end


function dskt_handle(app, evt)
   local key = evt.key

   if dskt_keytoggle[key] then
      if app.switcher then
	 evt_shutdown_app(app.switcher)
	 app.switcher = nil
      else

	 local dir = { n = 0 }

	 local i = app.sub
	 while i do
	    tinsert(dir, { name = i.name, size = 0, app = i })

	    i = i.next
	 end

	 app.switcher = dskt_switcher_create(app, "Application Switcher", dir)
	 
      end

      return
   end

   if key == evt_app_remove_event and evt.app == app.switcher then
      app.switcher = nil
   end

   if (key == evt_app_insert_event or key == evt_app_remove_event) and evt.app.owner == app then

      local focused = app.sub


      if key == evt_app_remove_event and focused and focused == evt.app then
	 focused = focused.next
      end


      if console_app and evt.app ~= console_app and focused ~= console_app and console_app.next then
	 -- force console to be last application (user friendly)
	 evt_app_insert_last(app, console_app)
      end


      if focused ~= app.focused then
	 if app.focused then
--	    print("unfocus", app.focused.name)
	    evt_send(app.focused, { key = gui_unfocus_event, app = app.focused }, 1)
	 end
	 if focused then
	    evt_send(focused, { key = gui_focus_event, app = focused }, 1)
--	    print("new focused", focused.name)
	 end
	 app.focused = focused
	 app.focus_time = 0
      end

      gui_child_autoplacement(app)
      return
   end

   if key < KBD_USER and key ~= shell_toggleconsolekey then
      -- prevent event falling back from one top application to the other
      return
   end
   return evt
end

function dskt_update(app)
end

function dskt_create()
   print("Installing desktop application")

   local app = evt_desktop_app

   app.handle = dskt_handle
   app.update = dskt_update
   if not app.dl then
      app.dl = dl_new_list(256, 1)
   end
   app.z = 0

   return app
end

dskt_create()

desktop_loaded = 1
