--- @ingroup  dcplaya_lua_application
--- @file     volume_control.lua
--- @author   benjamin gerard <ben@sashipa.com>
--- @date     2002
--- @brief    volume  application.
---
--- $Id: volume_control.lua,v 1.4 2003-03-05 08:42:44 ben Exp $
---

volume_control_loaded = nil

if not dolib("evt") then return end

--- Create a volume control application.
function volume_control_create(owner, name)
   local z
   owner = owner or evt_desktop_app
   name = name or "volume control"

   --- Update volume bar.
   function volume_control_update(vc, inc)
      local vol = playa_volume()
      local old = vol
      vol = vol + inc
      if vol < 0 then vol = 0
      elseif vol > 1 then vol = 1 end
      if vol ~= old then
	 playa_volume(vol)
	 vc:draw()
      end
   end

   --- volume control application avent handler.
   function volume_control_handle(vc, evt)
      local key = evt.key

      if key == evt_shutdown_event then
	 local k,dl
	 for k,dl in { vc.dl_bar, vc.dl_vol, vc.dl_sha, vc.dl } do
	    dl_set_active(dl,0)
	    dl_clear(dl)
	 end
	 vc.dl_bar = nil
	 vc.dl_sha = nil
	 vc.dl_vol = nil
	 return evt
      end

      if gui_keycancel[key] or gui_keyselect[key]  or gui_keyconfirm[key] then
	 evt_shutdown_app(vc)
	 return
      end

      if gui_keyleft[key] then
	 volume_control_update(vc, -0.01)
	 return
      elseif gui_keyright[key] then
	 volume_control_update(vc, 0.01)
	 return
      end
      return evt
   end

   --- Draw the volume bars sublist
   function volume_control_draw_bar(vc,w,space,h,n)
      local dl = vc.dl_bar
      local x = 0
      local scale_x,scale_y = 1 / ((w+space)*n), 1/h
      w = w * scale_x
      space = space * scale_x
      local i
      dl_clear(dl)
      for i=0, n do
	 dl_draw_box1(dl, x, 0, x+w, 1, 0, 1,1,1,1)
	 x = x + w + space
	 end
      return scale_x, scale_y
   end

   --- Draw "clipped" volume bar into given display list
   function volume_control_draw_it(vc,dl,vol)
      dl_clear(dl)
      dl_set_clipping(dl,-0.01,0,vol,1)
      dl_sublist(dl,vc.dl_bar)
   end

   --- Draw volume bar at a given volume
   function volume_control_draw(vc, vol)
      vol = vol or playa_volume()
      volume_control_draw_it(vc, vc.dl_vol, vol)
      volume_control_draw_it(vc, vc.dl_sha, vol)
      dl_set_active(vc.dl, 1)
      vmu_set_text(format("vol:%02d",vol*100))
   end

   local vc = {
      -- Application
      name = name,
      version = 1.0,
      handle = volume_control_handle,
      icon_name = "volume2",

      -- methods
      draw = volume_control_draw,

      -- Members
      z = gui_guess_z(owner,z),
      dl = dl_new_list(128, 0),
      dl_vol = dl_new_list(128, 1, 1),
      dl_sha = dl_new_list(128, 1, 1),
      dl_bar = dl_new_list(1024, 1, 1),
   }

   -- build bar
   local bar_w, bar_h = 400,40
   local scale_x, scale_y = volume_control_draw_bar(vc,16,10,bar_h,24)

   dl_set_color(vc.dl_vol, 1,1,1,1)
   dl_set_trans(vc.dl_vol,
		mat_scale(bar_w,bar_h,1) * mat_trans((640-bar_w)*0.5,0,10))

   dl_set_color(vc.dl_sha, 0.5,1,1,1)
   dl_set_trans(vc.dl_sha,
		mat_scale(bar_w,bar_h,1) * mat_trans((640-bar_w)*0.5+4,4,0))

   -- Main display list
   dl_clear(vc.dl)
   local tw = 48
   dl_text_prop(vc.dl,0,tw)
   local w,h = dl_measure_text(vc.dl,"volume", 0, tw)

   dl_draw_text(vc.dl, (640-w)*0.5,-h,0, 1,1,1,1, "volume");
   dl_sublist(vc.dl, vc.dl_vol)
   dl_sublist(vc.dl, vc.dl_sha)
   dl_set_trans(vc.dl, mat_trans(0,400,0))
   dl_set_color(vc.dl, 1,0,1,0)

   vc:draw()
   evt_app_insert_first(owner, vc)

   return vc
end

-- Load application icon
local tex = tex_get("volume2") or
		    tex_new(home .. "lua/rsc/icons/volume2.tga")

volume_control_loaded = 1
return volume_control_loaded

