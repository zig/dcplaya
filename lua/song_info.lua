--- @ingroup dcplaya_lua_gui
--- @file    song_info.lua
--- @author  benjamin gerard <ben@sashipa.com>
--- @date    2002/11/29
--- @brief   Song info application.
---
--- $Id: song_info.lua,v 1.20 2003-03-11 15:07:58 zigziggy Exp $

song_info_loaded = nil

if not dolib("basic") then return end
if not dolib("evt") then return end
if not dolib("box3d") then return end
if not dolib("sprite") then return end
if not dolib("style") then return end

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
---
--- @return  song-info application
--- @retval  nil  error
---
function song_info_create(owner, name, style)
   local si

   if not owner then owner = evt_desktop_app end
   if not name then name = "song info" end

   --- Default song-info update.
   --- @internal
   --
   function song_info_update_info(si, update)
      si.info = playa_info(update)
      if si.info and si.info.valid then
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

	 if si.info.comments then
	    si.sinfo_default_comments = nil
	 elseif not si.sinfo_default_comments then
	       si.sinfo_default_comments = "***  < Welcome to DCPLAYA >           ***       <The ultimate music player for Dreamcast>         ***        < (C)2002 Benjamin Gerard >           ***             < Main programming : Benjamin Gerard and Vincent Penne >"
	       si.info.comments = si.sinfo_default_comments
	 end
	 
	 if si.info.comments then
	    local x
	    local w,h = dl_measure_text(si.info_comments.dl,
					si.info.comments)
	    si.info_comments.text_w = w
	    si.info_comments.scroll = -32
	    local mat = dl_get_trans(si.info_comments.dl)
	    mat[4][1] = si.info_comments.w
	    dl_set_trans(si.info_comments.dl,mat)
	    dl_clear(si.info_comments.dl)
	    local c = si.label_color
	    dl_draw_text(si.info_comments.dl, 0,0,0,
			 c[1],c[2],c[3],c[4],
			 si.info.comments)
	 end
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

      --- Scroll-text
      if si.info_comments.scroll and not si.minimized then
	 local mat = dl_get_trans(si.info_comments.dl)
	 local x = mat[4][1]
	 local scroll = si.info_comments.scroll * frametime
	 x = x + scroll
	 local x2 = x + si.info_comments.text_w
	 if scroll < 0 then
	    if x2 < 0 then
	       x = -x2 - si.info_comments.text_w
	       si.info_comments.scroll = -si.info_comments.scroll
	    end
	 elseif x > si.info_comments.w then
	    x = si.info_comments.w - x + si.info_comments.w
	    si.info_comments.scroll = -si.info_comments.scroll
	 end
	 mat[4][1] = x
	 dl_set_trans(si.info_comments.dl,mat)
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
	    dl_set_active2(si.info_dl,si.help_dl,2)
	    si.info = nil
	 else
	    dl_set_active2(si.info_dl,si.help_dl,1)
	    if not si.info or playa_info_id() ~= si.info.valid then
	       song_info_update_info(si,1)
	    else
	    end
	 end

	 -- Refresh time
	 dl_clear(si.time_dl)
	 dl_text_prop(si.time_dl, 1, 1, 1, 0)
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
      dl_set_active(si.layer2_dl, not si.minimized)
      local s,x,y
      if si.minimized then
	 s,x,y = 8, 38, 12
      else
	 s,x,y = 16, 60, 35
      end
      dl_set_trans(si.layer0_dl, mat_scale(s,s,1) * mat_trans(x,y,1))
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
		     if si.minimized then
			menu.fl.dir[idx].name = "minimize"
			si:maximize()
		     else
			menu.fl.dir[idx].name = "maximize"
			si:minimize()
		     end
		     menu:draw()
		  end,
      }
      local root = ":" .. target.name .. ":" ..
	 ((si.minimized and "maximize") or "minimize") .. "{toggle}"

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
			  0))

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
      dl_draw_text(si.help_dl, 0,24,10, c[1],c[2],c[3],c[4], "help text " ..
		   strchar(16) .. " " .. strchar(17) .. " " ..  
		      strchar(18) .. " " .. strchar(19))
   end

   song_info_draw_help(si)

   -- Desc     Genre Year
   -- Artist
   -- Album
   -- Title
   -- Comment

   function song_info_draw_field(si,field)
      local x,y = 60,1
      local c
      dl_clear(field.dl)
      field.box:draw(field.dl,nil,1)
      if type(field.label) == "string" then
	 c = si.label_color
	 dl_draw_text(field.dl, 0,y,10, c[1],c[2],c[3],c[4], field.label)
      elseif tag(field.label) == sprite_tag then
	 field.label:draw(field.dl, 0,0,10)
      end

      if field.value then
	 if not field.label then
	    local w,h = dl_measure_text(field.dl, field.value)
	    x = (field.w - w) * 0.5
	 end
	 local c = si.text_color
	 dl_draw_text(field.dl, x,y,10, c[1],c[2],c[3],c[4], field.value)
      end
   end

   function song_info_field(label, box, x, y)
      local field = {}
      local obox = box3d_outer_box(box)
      field.label = label
      field.box = box
      field.w = obox[3]-obox[1]
      field.h = obox[4]-obox[2]
      field.dl = dl_new_list(0,1,1)
      dl_set_trans(field.dl, mat_trans(x,y,1))
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
   dl_set_trans(si.info_comments.dl, mat_trans(0, h*4-1, 1))

   dl_set_active(si.help_dl,0)

   dl_set_trans(si.info_dl,
		mat_trans( (inner[1] + inner[3] - ob[1] - ob[3]) * 0.5, 5, 1))

   local i,v
   for i,v in si.info_fields do
      song_info_draw_field(si,v)
      dl_sublist(si.info_dl, v.dl)
   end
   dl_set_clipping(si.info_dl, 0, 0, si.info_comments.w , 0)
   dl_sublist(si.info_dl, si.info_comments.dl)

   song_info_update_info(si,1)
   si:set_color(0, 1, 1, 1)
   si:draw()
   si:open()

   evt_app_insert_last(owner, si)

   return si
end

--- @}

-- Load texture for application icon
local tex = tex_exist("song-info")
   or tex_new(home .. "lua/rsc/icons/song-info.tga")

if song_info then
   evt_shutdown_app(song_info)
end

song_info = song_info_create()

function song_info_kill(si)
   si = si or song_info
   if si then
      evt_shutdown_app(si)
      if si == song_info then song_info = nil end
   end
end

song_info_loaded = 1
return song_info_loaded
