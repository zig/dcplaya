--- @ingroup dcplaya_lua_gui
--- @file    song_info.lua
--- @author  benjamin gerard <ben@sashipa.com>
--- @date    2002/11/29
--- @brief   Song info application.
--- $Id: song_info.lua,v 1.3 2002-12-05 08:17:47 ben Exp $

song_info_loaded = nil

if not dolib("basic") then return end
if not dolib("evt") then return end

function song_info_create_icons(si)

   -- Make vertrices
   --  x1 x3
   --  12 56  y1
   --      9  y2
   --  34 78  y3
   --   x2 x4
   local x1,x2,x3,x4,y1,y2,y3
   x1 = 0
   x2 = 0.3
   x3 = 1-x2
   x4 = 1
   y1 = 0
   y2 = 0.5
   y3 = 1

   local mat
   mat = mat_new(9,12)

   set_vertex(mat[1], { x1, y1, 0, 1,  1, 1, 1, 1,  0, 0 } )
   set_vertex(mat[2], { x2, y1, 0, 1,  1, 1, 1, 1,  0, 0 } )
   set_vertex(mat[3], { x1, y3, 0, 1,  1, 1, 1, 1,  0, 0 } )
   set_vertex(mat[4], { x2, y3, 0, 1,  1, 1, 1, 1,  0, 0 } )
   set_vertex(mat[5], { x3, y1, 0, 1,  1, 1, 1, 1,  0, 0 } )
   set_vertex(mat[6], { x4, y1, 0, 1,  1, 1, 1, 1,  0, 0 } )
   set_vertex(mat[7], { x3, y3, 0, 1,  1, 1, 1, 1,  0, 0 } )
   set_vertex(mat[8], { x4, y3, 0, 1,  1, 1, 1, 1,  0, 0 } )
   set_vertex(mat[9], { x4, y2, 0, 1,  1, 1, 1, 1,  0, 0 } )

   -- Make PLAY icons
   si.icons_dl = {}
   si.icons_dl[1] = dl_new_list(0,1,1)
   dl_draw_triangle(si.icons_dl[1], mat[1], mat[9], mat[3])

   -- Make PAUSE icons
   si.icons_dl[2] = dl_new_list(0,1,1)
   dl_draw_strip(si.icons_dl[2], mat[1], 4)
   dl_draw_strip(si.icons_dl[2], mat[5], 4)

   -- Make STOP icons
   si.icons_dl[3] = dl_new_list(0,1,1)
   mat[2] = mat[6]
   mat[4] = mat[8]
   dl_draw_strip(si.icons_dl[3], mat[1], 4)

end

--- Song-Info create.
--
function song_info_create(owner, nane)
   local si

   if not owner then owner = evt_desktop_app end
   if not name then name = "song-info" end

   --- Song-Info update.
   --
   function song_info_update(si, frametime)

	  vcolor(1,0,0,1)

	  if dl_get_active(si.icon_dl) == 1 then 
		 dl_set_active(si.icon_dl, 0)
	  else
		 dl_set_active(si.icon_dl, 1)
	  end

	  local a = si.alpha or 0

	  -- Process fading 
	  if si.fade then
		 a = a + si.fade * frametime
		 if a > 1 then
			a = 1
			si.fade = nil
		 elseif a < 0 then
			a = 0
			si.fade = nil
			dl_set_active(si.dl, 0)
		 end
		 si:set_color(a)
	  end

	  -- Process refresh
	  si.time_elapsed = si.time_elapsed + frametime
	  if si.time_elapsed > si.time_refresh then
		 local isplaying, new_icon
		 si.time_elapsed = si.time_elapsed - si.time_refresh

		 isplaying = play()

		 -- refresh icon
		 if isplaying == 1 then 
			if pause() == 1 then
			   new_icon = 2
			else
			   new_icon = 1
			end
		 else
			new_icon = 3
		 end

		 if si.cur_icon ~= new_icon then
			si.cur_icon = new_icon
			dl_clear(si.icon_dl);
			dl_sublist(si.icon_dl,si.icons_dl[new_icon])
		 end

		 -- Refresh info
		 if isplaying ~= 1 then
			si.info = nil
		 else
			if not si.info or playa_info_id() ~= si.info.valid then
			   si.info = playa_info()
			   if si.info and si.info.valid then
			   else
			   end
			end
		 end

		 -- Refresh time
		 dl_clear(si.time_dl)
		 dl_text_prop(si.time_dl, 1, 1, 1)
		 local s,fs = playtime();
		 if not fs then fs = "??:??" end
		 if si.info and si.info.valid ~= 0 and si.info.track then
			fs = si.info.track.." "..fs
		 end
		 dl_draw_text(si.time_dl, 0,0,0, 1,1,1,1, fs)
	  end

	  vcolor(0,0,0,0)

   end

   --- Song-Info handle.
   --
   function song_info_handle(si, evt)
	  local key = evt.key
	  if key == evt_shutdown_event then
		 si:shutdown()
	  end
	  return evt
   end

   --- Song-Info open.
   --
   function song_info_open(si)
	  si.fade = 1
	  si.closed = nil
	  dl_set_active(si.dl,1)
   end
   
   --- Song-Info close.
   --
   function song_info_close(si)
	  si.closed = 1
	  si.fade = -1;
   end

   --- Song-info set color.
   --
   function song_info_set_color(si, a, r, g, b)
	  a = a or si.alpha or 1
	  si.alpha = a
	  dl_set_color(si.dl, a, r, g, b)
   end

   --- Song-info draw.
   --
   function song_info_draw(si)
	  dl_set_trans(si.dl,mat_scale(16,16,1) * mat_trans(80,50,si.z))
-- 	  dl_set_trans(si.icon_dl, )
   end

   --- Song-Info shutdown.
   function song_info_shutdown(si)
	  dl_destroy_list(si.dl)
	  dl_destroy_list(si.icon_dl)
	  dl_destroy_list(si.time_dl);
	  local i,v
	  for i,v in si.icons_dl do
		 dl_destroy_list(v)
	  end
   end

   si = {
	  -- Application
	  name = name,
	  version = 1.0,
	  handle = song_info_handle,
	  update = song_info_update,

	  -- methods
	  shutdown = song_info_shutdown,
	  open = song_info_open,
	  close = song_info_close,
	  draw = song_info_draw,
	  set_color = song_info_set_color,

	  -- members
	  time_elapsed = 0,
	  time_refresh = 1,
	  alpha = 0,
	  fade = 0,
	  z = gui_guess_z(owner,z),
   }

   song_info_create_icons(si)
   si.dl = dl_new_list(256,0)
   si.icon_dl = dl_new_list(0,1,1)
   si.time_dl = dl_new_list(0,1,1)
   dl_set_trans(si.time_dl, mat_trans(2,0,0))

   dl_text_prop(si.time_dl, 0, 1.5, 1)
   dl_draw_text(si.time_dl, 0,0,0, 1,1,1,1, "info")


   dl_sublist(si.dl, si.icon_dl)
   dl_sublist(si.dl, si.time_dl)

   si:set_color(0, 1, 1, 1)
   si:draw()
   si:open()
   evt_app_insert_first(owner, si)

   return si
end


si = song_info_create()

function k()
   if si then evt_shutdown_app(si) end
   si = nil
end

song_info_loaded = 1
return song_info_loaded
