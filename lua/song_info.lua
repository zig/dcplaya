--- @ingroup dcplaya_lua_gui
--- @file    song_info.lua
--- @author  benjamin gerard <ben@sashipa.com>
--- @date    2002/11/29
--- @brief   Song info application.
---
--- $Id: song_info.lua,v 1.6 2002-12-11 14:18:50 ben Exp $

song_info_loaded = nil

if not dolib("basic") then return end
if not dolib("evt") then return end
if not dolib("box3d") then return end
if not dolib("sprite") then return end

--- @name song-info functions
--- @ingroup dcplaya_lua_gui
--- @{
--

--- Creates song-info sprite icons.
--- @internal
--- @param  si  song-info application
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

--- Create a song-info application.
---
--- @param  owner  Owner application (nil for desktop).
--- @param  name   Application name (nil for "song-info").
---
--- @return  song-info application
--- @retval  nil  error
---
function song_info_create(owner, name)
   local si

   if not owner then owner = evt_desktop_app end
   if not name then name = "song-info" end

   --- Default song-info update.
   --- @internal
   --
   function song_info_update(si, frametime)

-- 	  if dl_get_active(si.icon_dl) == 1 then 
-- 		 dl_set_active(si.icon_dl, 0)
-- 	  else
-- 		 dl_set_active(si.icon_dl, 1)
-- 	  end

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
				  local i,v
				  for i,v in si.info_fields do
					 print("update "..i..":"..si.info[i])
					 v.value = si.info[i]
					 song_info_draw_field(v)
				  end
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
   end

   --- Default song-info handle.
   --- @internal
   --
   function song_info_handle(si, evt)
	  local key = evt.key
	  if key == evt_shutdown_event then
		 si:shutdown()
	  end
	  return evt
   end

   --- Default song-info open.
   --- @internal
   --
   function song_info_open(si)
	  si.fade = 1
	  si.closed = nil
	  dl_set_active(si.dl,1)
   end
   
   --- Default song-info close.
   --- @internal
   --
   function song_info_close(si)
	  si.closed = 1
	  si.fade = -1;
   end

   --- Default song-info set color.
   --- @internal
   --
   function song_info_set_color(si, a, r, g, b)
	  a = a or si.alpha or 1
	  si.alpha = a
	  dl_set_color(si.dl, a, r, g, b)
   end

   --- Default song-info draw.
   --- @internal
   --
   function song_info_draw(si)
	  dl_set_trans(si.dl,mat_trans(0,0,si.z))
	  dl_set_trans(si.layer1_dl, mat_scale(16,16,1) * mat_trans(80,50,1))
--	  dl_set_trans(si.layer2_dl, mat_trans(45,300,1))

-- 	  dl_set_trans(si.icon_dl, )
   end

   --- Default song-info shutdown.
   --- @internal
   --
   function song_info_shutdown(si)
	  si.dl = nil
	  si.icons_dl = nil
	  si.layer1_dl = nil
	  si.layer2_dl = nil
	  si.icon_dl = nil
	  si.time_dl = nil
	  si.info_dl = nil
	  si.help_dl = nil
	  si.info_fields = nil
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
   si.dl = dl_new_list(256,0)        -- Main display list
   si.layer1_dl = dl_new_list(0,1,1) -- Time and icon layer sub-list
   si.layer2_dl = dl_new_list(0,1,1) -- Help and info layer sub-list

   si.icon_dl = dl_new_list(0,1,1)
   si.time_dl = dl_new_list(0,1,1)
   si.info_dl = dl_new_list(0,1,1)
   si.help_dl = dl_new_list(0,1,1)

   dl_set_trans(si.time_dl, mat_trans(2,0,0))

--    dl_text_prop(si.time_dl, 0, 1.5, 1)
--    dl_draw_text(si.time_dl, 0,0,0, 1,1,1,1, "info")

   dl_sublist(si.layer1_dl, si.icon_dl)
   dl_sublist(si.layer1_dl, si.time_dl)
   dl_sublist(si.layer2_dl, si.info_dl)
   dl_sublist(si.layer2_dl, si.help_dl)
   dl_sublist(si.dl, si.layer1_dl)
   dl_sublist(si.dl, si.layer2_dl)

   local color = color_new(1, 0, 0.7, 1)

   local ct, cl, cb, cr
   ct = { 1, 0.9, 0.9, 0.9 }
   cl = { 1, 0.7, 0.7, 0.7 }
   cb = { 1, 0.5, 0.5, 0.5 }
   cr = { 1, 0.4, 0.4, 0.4 }

   si.layer2_box = box3d({0,0,550,120},
						 -4,
						 color * 0.5,
						 color * ct,
						 color * cl,
						 color * cr,
						 color * cb)
   si.layer2_ibox = box3d_inner_box(si.layer2_box)
   si.layer2_obox = box3d_outer_box(si.layer2_box)

   local screenw, screenh = 640, 480
   local outer, inner = si.layer2_obox,si.layer2_ibox

   dl_set_trans(si.layer2_dl,
				mat_trans((screenw - (outer[3]-outer[1])) * 0.5,
						  screenh - (outer[4]-outer[2]) - 16,
						  0))

   si.layer2_box:draw(si.layer2_dl, nil, 1)


   function song_info_draw_help(si)
	  local w,h = dl_measure_text(si.help_dl,"Help")
	  w = w * 24 / 16
	  h = h * 24 / 16
	  local x = (si.layer2_obox[3] + si.layer2_obox[1] - w) * 0.5
	  dl_text_prop(si.help_dl, 0, 24)
	  dl_draw_text(si.help_dl, x, 0, 10, 1,1,1,0, "Help")
	  dl_text_prop(si.help_dl, 0, 16)
	  dl_draw_text(si.help_dl, 0,24,10, 1,1,1,1, "help text " ..
				   strchar(16) .. " " .. strchar(17) .. " " ..  
				   strchar(18) .. " " .. strchar(19))
   end

   song_info_draw_help(si)

-- Desc     Genre Year
-- Artist
-- Album
-- Title
-- Comment

   function song_info_draw_field(field)
	  local x,y = 70,1
	  dl_clear(field.dl)
	  field.box:draw(field.dl,nil,1)
	  if type(field.label) == "string" then
		 dl_draw_text(field.dl, 0,y,10, 1,1,1,0.6, field.label)
	  elseif tag(field.label) == sprite_tag then
		 field.label:draw(field.dl, 0,0,10)
	  else
		 x = 0
	  end

	  if field.value then
		 dl_draw_text(field.dl, x,y,10, 1,1,1,1, field.value)
	  end
   end

   function song_info_field(label, box, x, y)
	  local field = {}
	  field.label = label
	  field.box = box
	  field.dl = dl_new_list(0,1,1)
	  dl_set_trans(field.dl, mat_trans(x,y,0))
	  return field
   end

   color = color_new(0.5,1,1,1)
   local lbox
   lbox = box3d({0,0,inner[3]-inner[1]-12,18},
				-2,
				{0.5,0,0,0},
				color * cb,
				color * cr,
				color * cl,
				color * ct)

   si.info_fields = {}
   local ob = box3d_outer_box(lbox)
   local h = (ob[4] - ob[2]) + 3
   si.info_fields.format  = song_info_field("Format", lbox,0,h*0)
   si.info_fields.artist  = song_info_field("Artist", lbox,0,h*1)
   si.info_fields.album   = song_info_field("Album", lbox,0,h*2)
   si.info_fields.title   = song_info_field("Title", lbox,0,h*3)
   dl_set_active(si.help_dl,0)

   dl_set_trans(si.info_dl,
				mat_trans( (inner[1] + inner[3] - ob[1] - ob[3]) * 0.5, 5, 1))

   local i,v
   for i,v in si.info_fields do
	  song_info_draw_field(v)
	  dl_sublist(si.info_dl, v.dl)
   end

   si:set_color(0, 1, 1, 1)
   si:draw()
   si:open()

   evt_app_insert_first(owner, si)

   return si
end

--- @}


si = song_info_create()

function ksi()
   if si then evt_shutdown_app(si) end
   si = nil
end

song_info_loaded = 1
return song_info_loaded
