--- @ingroup  dcplaya_lua_si_app
--- @file     song_info.lua
--- @author   benjamin gerard
--- @date     2002/11/29
--- @brief    Song info application.
---
--- $Id: song_info.lua,v 1.29 2003-04-05 16:33:31 ben Exp $

song_info_loaded = nil

if not dolib("basic") then return end
if not dolib("evt") then return end
if not dolib("box3d") then return end
if not dolib("sprite") then return end
if not dolib("style") then return end

--- @defgroup  dcplaya_lua_si_app Song Info
--- @ingroup   dcplaya_lua_app
--- @brief     song info application displays music information.
---
---   The song-info application is used to display current playing music
---   information. It could be displayed in two modes maximized and
---   minimized. 
---
---   Normal behaviour is to have only one instance of a song-info application.
---   It is stored in the global variable song_info.
---
--- @author   benjamin gerard
---
--- @{
---

--- Global song_info application
--- @ingroup dcplaya_lua_si_app
--: application song_info;

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

   local mat = mat_new(9,12)

   set_vertex(mat[1], { x1, y1, 0, 1,  1,1,1,1,  0, 0 } )
   set_vertex(mat[2], { x2, y1, 0, 1,  1,1,1,1,  0, 0 } )
   set_vertex(mat[3], { x1, y3, 0, 1,  1,1,1,1,  0, 0 } )
   set_vertex(mat[4], { x2, y3, 0, 1,  1,1,1,1,  0, 0 } )
   set_vertex(mat[5], { x3, y1, 0, 1,  1,1,1,1,  0, 0 } )
   set_vertex(mat[6], { x4, y1, 0, 1,  1,1,1,1,  0, 0 } )
   set_vertex(mat[7], { x3, y3, 0, 1,  1,1,1,1,  0, 0 } )
   set_vertex(mat[8], { x4, y3, 0, 1,  1,1,1,1,  0, 0 } )
   set_vertex(mat[9], { x4, y2, 0, 1,  1,1,1,1,  0, 0 } )

   c = si.text_color
   si.icons_dl = {}

   -- Make PLAY icons
   si.icons_dl[1] = dl_new_list(0,1,1)
   dl_draw_triangle(si.icons_dl[1], mat[1], mat[9], mat[3])

   -- Make PAUSE icons
   si.icons_dl[2] = dl_new_list(0,1,1)
   dl_draw_strip(si.icons_dl[2], mat[1], 4)
   dl_draw_strip(si.icons_dl[2], mat[5], 4)

   -- Make STOP icons
   si.icons_dl[3] = dl_new_list(0,1,1)
   mat[2],mat[4] = mat[6],mat[8]
   dl_draw_strip(si.icons_dl[3], mat[1], 4)

end




--- Create a song-info application.
---
--- @param  owner  Owner application (nil for desktop).
--- @param  name   Application name (nil for "song info").
--- @param  style  drwing style.
---
--- @return  song-info application
--- @retval  nil  error
---
function song_info_create(owner, name, style)
   local si

   if not owner then owner = evt_desktop_app end
   if not name then name = "song info" end

   function song_info_toggle_help(si, timeout)
      if timeout or not si.info or si.info.valid == 0 then
	 dl_set_active2(si.help_dl,si.info_dl,1)
      else
	 dl_set_active2(si.help_dl,si.info_dl,2)
      end
      si.help_timeout = timeout
   end

   function song_set_time_str(si, s)
      if not si.info_time or s ~= si.info_time then
	 si.info_time = s
	 dl_clear(si.time_dl)
	 dl_text_prop(si.time_dl, 1, 2, 0.75, 0)
	 dl_draw_text(si.time_dl, 0,0,0, 1,1,1,1, s)
      end
   end

   --- Default song-info update.
   --- @internal
   --
   function song_info_update_info(si)
      local new_id = playa_info_id()
      -- No info 
      if new_id == 0 then
	 si.info = nil;
	 song_set_time_str(si, "--:--");
      end

      local new_music = not si.info or si.info.valid ~= new_id

      -- Don't need to update, since new music update all fields.
      -- Force the update only at start up, even if it should not
      -- be required.
      si.info = playa_info(new_music)
      if si.info then
	 local i,v
	 for i,v in si.info do
	    if (si.info_fields[i]) then
	       if type(v) ~= type(si.info_fields[i].value) or
		  v ~= si.info_fields[i].value then
		  si.info_fields[i].value = v
		  song_info_draw_field(si,si.info_fields[i])
	       end
	    end
	 end

	 if new_music then
	    if si.info.comments and si.info.comments == "N/A" then
	       si.info.comments = nil
	    end
	    if si.info.comments then
	       si.sinfo_default_comments = nil
	    elseif not si.sinfo_default_comments then
	       si.sinfo_default_comments = "***  < Welcome to DCPLAYA >           ***       <The ultimate music player for Dreamcast>         ***        < (C)2002-2003 Benjamin Gerard >           ***             < Main programming : Benjamin Gerard and Vincent Penne >"
	       si.info.comments = si.sinfo_default_comments
	    end
	    if si.info.comments then
	       local x
	       local w,h = dl_measure_text(si.info_comments.dl,
					   si.info.comments)
	       dl_clear(si.info_comments.dl)
	       local c = si.label_color
	       dl_draw_scroll_text(si.info_comments.dl, 0,0,10,
				   c[1],c[2],c[3],c[4],
				   si.info.comments, si.info_comments.w, 1)
	    end
	 end

	 -- get play time
	 local s,fs = playtime();
	 -- Got a new track,
	 if si.info.track or new_music then
	    si.info_track = si.info.track
	 end
	 fs = (si.info_track and si.info_track ~= "N/A" and
	       (si.info_track .. " ") or "")
	    .. (fs or "??:??")
	 dl_clear(si.time_dl)
	 dl_text_prop(si.time_dl, 1, 2, 0.75, 0)
	 dl_draw_text(si.time_dl, 0,0,0, 1,1,1,1, fs)
      end
   end

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

      -- Help hide / show
      a = si.help_timeout
      if a then
	 a = a - frametime
	 a = a > 0 and a
      end
      song_info_toggle_help(si, a)

      -- Process refresh
      si.time_elapsed = si.time_elapsed + frametime
      if si.time_elapsed > si.time_refresh then
	 local isplaying, new_icon
	 si.time_elapsed = si.time_elapsed - si.time_refresh

	 isplaying = play()

	 -- refresh icon
	 new_icon = isplaying == 1 and (1 + pause()) or 3
	 if si.cur_icon ~= new_icon then
	    si.cur_icon = new_icon
	    dl_clear(si.icon_dl);
	    dl_sublist(si.icon_dl,si.icons_dl[new_icon])
	 end

	 -- Refresh info
	 song_info_update_info(si)

      end
   end

   --- Default song-info handle.
   --- @internal
   --
   function song_info_handle(si, evt)
      local key = evt.key

      if key == evt_shutdown_event then
	 si:shutdown()
	 return evt
      elseif key == gui_focus_event then
	 local save_minimized = si.minimized
	 si:maximize()
	 si.focus_has_maximized = save_minimized
	 return
      elseif key == gui_unfocus_event then
	 if si.focus_has_maximized then
	    si.focus_has_maximized = nil
	    si:minimize()
	 end
	 return
      elseif ke_keyactivate and ke_keyactivate[key] then
	 if __DEBUG then
	    print("toggle help:", si.help_timeout)
	 end
	 song_info_toggle_help(si, (not si.help_timeout and 5))
	 return
      end

      if __DEBUG_EVT then
	 print("si leave ", key)
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

   --- Default song-info minimize.
   --- @internal
   --
   function song_info_minimize(si)
      si.minimized = 1
      si:draw()
   end

   --- Default song-info minimize.
   --- @internal
   --
   function song_info_maximize(si)
      si.focus_has_maximized = nil
      si.minimized = nil
      si:draw()
   end

   --- Default song-info minimize/maximize toggle.
   --- @internal
   --
   function song_info_toggle(si)
      if si.minimized then
	 si:maximize()
      else
	 si:minimize()
      end
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
--      dl_set_trans(si.dl,mat_trans(0,0,si.z))
      dl_set_active(si.layer2_dl, not si.minimized)
      local s,x,y
      if si.minimized then
	 s,x,y = 14, 38, 12
      else
	 s,x,y = 24, 60, 35
      end
      dl_set_trans(si.layer0_dl, mat_scale(s,s,1) * mat_trans(x,y,10))
      dl_set_active(si.layer0_dl,1)
   end

   --- Default song-info shutdown.
   --- @internal
   --
   function song_info_shutdown(si)
      dl_set_active(si.dl,0)
      si.dl = nil
      si.icons_dl = nil
      si.icons_shadow_dl = nil
      si.layer0_dl = nil
      si.layer1_dl = nil
      si.shadow1_dl = nil
      si.layer2_dl = nil
      si.icon_dl = nil
      si.time_dl = nil
      si.info_dl = nil
      si.help_dl = nil
      si.info_fields = nil
      si.info_comments = nil
   end

   function song_info_menucreator(target)
      local si = target;
      local cb = {
	 toggle = function(menu, idx)
		     local si = menu.target
		     si:toggle()
		     menu_any_image(menu, idx, si.minimized,
				 'maximize', 'minimize',
				 'minimize', 'minimize')
		  end,
      }
      local root = ":" .. target.name .. ":"
	 .. menu_any_menu(si.minimized, 'maximize',
			  'minimize', 'minimize', 'minimize', 'toggle', -1)
      local def = menu_create_defs
      (
       {
	  root=root,
	  cb = cb,
       }, target)
      return def
   end
   
   si = {
      -- Application
      name = name,
      version = 1.0,
      handle = song_info_handle,
      update = song_info_update,
      mainmenu_def = song_info_menucreator,
      icon_name = "song-info",

      -- methods
      shutdown = song_info_shutdown,
      open = song_info_open,
      close = song_info_close,
      draw = song_info_draw,
      set_color = song_info_set_color,
      minimize = song_info_minimize,
      maximize = song_info_maximize,
      toggle = song_info_toggle,

      -- members
      time_elapsed = 0,
      time_refresh = 1,
      alpha = 0,
      fade = 0,
      z = gui_guess_z(owner,z),
      flags = { unfocusable = 1 },
   }

   local bstyle = style_get(style)
   local fcol, tcol, lcol, bcol, rcol

   fcol = bstyle:get_color(0)
   tcol = bstyle:get_color(1,0)
   lcol = bstyle:get_color(1,0) * { 1, 0.7, 0.7, 0.7 }
   bcol = bstyle:get_color(0,1)
   rcol = bstyle:get_color(0,1) * { 1, 0.7, 0.7, 0.7 }

   local mf, mt, ml, mr, mb
   mf = { 0.5, 1, 1, 1 }
   mt = { 1, 1, 1, 1 }
   ml = { 1, 0.7, 0.7, 0.7 }
   mb = { 1, 0.5, 0.5, 0.5 }
   mr = { 1, 0.4, 0.4, 0.4 }

   -- 1  2
   --  34
   --  56
   -- 7  8
   local borcol = {
      bstyle:get_color(1,0),     --1 
      bstyle:get_color(0.5,1),   --2
      bstyle:get_color(1,0.5),   --3
      bstyle:get_color(1,1),     --4
      bstyle:get_color(1,1),     --5
      bstyle:get_color(1,0),     --6
      bstyle:get_color(0.5,1),   --7
      bstyle:get_color(0,1),     --8
   }

   fcol = {
      mf * bstyle:get_color(0.5,0.5),
      mf * bstyle:get_color(0.25,0.25),
      nil,
      mf * bstyle:get_color(0)
   }

   tcol = {
      mt * borcol[1],
      mt * borcol[2],
      mt * borcol[3],
      mt * borcol[4],
   }

   lcol = {
      ml * borcol[1],
      ml * borcol[3],
      ml * borcol[7],
      ml * borcol[5],
   }

   bcol = {
      mb * borcol[5],
      mb * borcol[6],
      mb * borcol[7],
      mb * borcol[8],
   }

   rcol = {
      mr * borcol[4],
      mr * borcol[2],
      mr * borcol[6],
      mr * borcol[8],
   }
   
   local color = color_new(1, 1, 1, 1)

   si.label_color = bstyle:get_color(1,0)
   si.text_color  = bstyle:get_color(1,1)


   song_info_create_icons(si)
   si.dl = dl_new_list(256,0)        -- Main display list

   si.layer0_dl = dl_new_list(0,1,1)  -- support for layer1 shadow
   si.layer1_dl = dl_new_list(0,1,1)  -- Time and icon layer sub-list
   si.shadow1_dl = dl_new_list(0,1,1) -- Time and icon layer sub-list

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
   dl_set_color(si.layer1_dl,
		si.label_color[1],si.label_color[2],
		si.label_color[3],si.label_color[4])

   dl_sublist(si.shadow1_dl, si.layer1_dl)
   dl_set_color(si.shadow1_dl,0.8,0.5,0.5,0.5)
   dl_set_trans(si.shadow1_dl,mat_trans(0.3,0.3, -10))
   dl_set_trans(si.layer1_dl,mat_trans(0,0, 10))

   dl_sublist(si.layer0_dl, si.layer1_dl)
   dl_sublist(si.layer0_dl, si.shadow1_dl)

   dl_sublist(si.layer2_dl, si.info_dl)
   dl_sublist(si.layer2_dl, si.help_dl)
   dl_sublist(si.dl, si.layer0_dl)
   dl_sublist(si.dl, si.layer2_dl)
   
   si.layer2_box = box3d({0,0,550,120},
			 -4, fcol, tcol, lcol, bcol, rcol)

   si.layer2_ibox = box3d_inner_box(si.layer2_box)
   si.layer2_obox = box3d_outer_box(si.layer2_box)

   local screenw, screenh = 640, 480
   local outer, inner = si.layer2_obox,si.layer2_ibox

   dl_set_trans(si.layer2_dl,
		mat_trans((screenw - (outer[3]-outer[1])) * 0.5,
			  screenh - (outer[4]-outer[2]) - 16,
			  10))

   si.layer2_box:draw(si.layer2_dl, nil, 1)


   function song_info_draw_help(si)
      local w,h = dl_measure_text(si.help_dl,"Help")
      w = w * 24 / 16
      h = h * 24 / 16
      local x = (si.layer2_obox[3] + si.layer2_obox[1] - w) * 0.5
      local c
      dl_text_prop(si.help_dl, 0, 24)
      c = si.label_color
      dl_draw_text(si.help_dl, x, 0, 10, c[1],c[2],c[3],c[4], "Help")
      dl_text_prop(si.help_dl, 0, 16)
      c = si.text_color

      local y = h+8
      local i,text
      for i,text in {
	 "\016 ... Confirm (depends on the selected item)",
	 "\017 ... Cancel (stop playing in Filelist / remove in Playlist)",
	 "\018 ... Display contextual menu",
	 "\019 ... Toggle appication switcher (access to application menu)",
      } do
	 w,h = dl_measure_text(si.help_dl,text)
	 dl_draw_text(si.help_dl,
		      7, y ,10
		      , c[1],c[2],c[3],c[4],
		      text)
	 y = y + h + 4
      end
   end

   song_info_draw_help(si)

   -- Desc     Genre Year
   -- Artist
   -- Album
   -- Title
   -- Comment

   function song_info_draw_field(si,field)
      local x,y,space = 60,1,3
      local c, maxw
      
      dl_clear(field.dl)
      field.box:draw(field.dl,nil,1)
      if type(field.label) == "string" then
	 -- Draw label
	 c = si.label_color
	 maxw = x-space
	 local w,h = dl_measure_text(field.dl,field.label)
	 if w <= maxw then
	    dl_draw_text(field.dl, 0,y,10, c[1],c[2],c[3],c[4], field.label)
	 else
	    dl_set_clipping(field.dl,0,0,maxw,-1)
	    dl_draw_scroll_text(field.dl, 0,y,10, c[1],c[2],c[3],c[4],
				field.label, maxw, 0.5, 2)
	 end
      elseif tag(field.label) == sprite_tag then
	 field.label:draw(field.dl, 0,0,10)
      end

      if field.value then
	 local w,h = dl_measure_text(field.dl, field.value)
	 c = si.text_color
	 x = (field.label and x) or 0
	 maxw = field.w - x
	 if w > maxw then
	    dl_set_clipping(field.dl,x,0,x+maxw,-1)
	    dl_draw_scroll_text(field.dl, x,y,10, c[1],c[2],c[3],c[4],
				field.value, maxw, 0.5, 2)
	 else
	    x = (field.label and x) or ((maxw - w) * 0.5)
	    dl_draw_text(field.dl, x,y,10, c[1],c[2],c[3],c[4], field.value)
	 end
      end
   end

   function song_info_field(label, box, x, y)
      local field = {}
--      local obox = box3d_outer_box(box)
      local ibox = box3d_inner_box(box)
      field.label = label
      field.box = box
      field.w = ibox[3]-ibox[1]
      field.h = ibox[4]-ibox[2]
      field.dl = dl_new_list(0,1,1)
      dl_set_trans(field.dl, mat_trans(x,y,10))
      return field
   end


   color = color_new(0.5,1,1,1)
   local lbox, mbox, sbox
   local lw = inner[3]-inner[1]-12
   local sw = 100
   local mw = lw - sw - 10
   local h = 18

   local bcol1,bcol2 = {0.8, 0.4, 0.0, 0.0},{0.8, 0.3, 0.3, 0.3}

   local fcol = nil --{ bcol1,0.5*bcol1+0.5*bcol2,nil,bcol2 }

   lbox = box3d({0,0,lw,h}, -2,
		fcol, bcol, rcol, tcol, lcol)

   mbox = box3d({0,0,mw,h}, -2,
		fcol, bcol, rcol, tcol, lcol)

   sbox = box3d({0,0,sw,h}, -2,
		fcol, bcol, rcol, tcol, lcol)
   
   si.info_fields = {}
   local ob = box3d_outer_box(lbox)
   local h = (ob[4] - ob[2]) + 3
   si.info_fields.format = song_info_field("Format", mbox,0,h*0)
   si.info_fields.genre  = song_info_field(nil, sbox, mw+10, h*0)
   si.info_fields.artist = song_info_field("Artist", lbox,0,h*1)
   si.info_fields.album  = song_info_field("Album",  mbox,0,h*2)
   si.info_fields.year   = song_info_field(nil, sbox, mw+10, h*2)
   si.info_fields.title   = song_info_field("Title",  lbox,0,h*3)

   si.info_comments = {}
   si.info_comments.dl = dl_new_list(0,1,1)
   si.info_comments.w = ob[3] - ob[1]
   si.info_comments.h = inner[4] - h*4
   dl_set_trans(si.info_comments.dl, mat_trans(0, h*4-1, 10))

   dl_set_active(si.help_dl,0)

   dl_set_trans(si.info_dl,
		mat_trans( (inner[1] + inner[3] - ob[1] - ob[3]) * 0.5, 5, 10))

   local i,v
   for i,v in si.info_fields do
      song_info_draw_field(si,v)
      dl_sublist(si.info_dl, v.dl)
   end
   dl_set_clipping(si.info_dl, 0, 0, si.info_comments.w , 0)
   dl_sublist(si.info_dl, si.info_comments.dl)

   si.info = nil
   song_info_update_info(si)
   song_info_toggle_help(si,si.help_timeout)
   si:set_color(0, 1, 1, 1)
   si:draw()
   si:open()

   evt_app_insert_last(owner, si)

   return si
end

--
--- Kill a song-info application.
---
---   The song_info_kill() function kills the given application by
---   calling sending the evt_shutdown_app() function. If the given
---   application is nil or song_info the default song-info
---   (song_info) is killed and the global variable song_info is
---   set to nil.
---
--- @param  si  application to kill (default to song_info)
--
function song_info_kill(si)
   si = si or song_info
   if si then
      evt_shutdown_app(si)
      if si == song_info then song_info = nil end
   end
end

--
--- @}
---

-- Load texture for application icon
if not (tex_exist("song-info")
	or tex_new(home .. "lua/rsc/icons/song-info.tga"))
then
   print("song-info icon is missing")
end

if song_info then
   evt_shutdown_app(song_info)
end

song_info_kill()
song_info = song_info_create()

song_info_loaded = 1
return song_info_loaded
