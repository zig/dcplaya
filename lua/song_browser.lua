--- @ingroup  dcplaya_lua_application
--- @file     song_browser.lua
--- @author   benjamin gerard <ben@sashipa.com>
--- @date     2002
--- @brief    song browser application.
---
--- $Id: song_browser.lua,v 1.36 2003-03-07 10:11:16 ben Exp $
---

song_browser_loaded = nil
if not dolib("textlist") then return end
if not dolib("gui") then return end
if not dolib("sprite") then return end
if not dolib("fileselector") then return end
if not dolib("playlist") then return end

function song_browser_create_sprite(sb)
   sb.sprites = {}
   sb.sprites.texid = tex_get("dcpsprites") or tex_new("/rd/dcpsprites.tga")

   sb.sprites.logo = sprite("dcplogo",
			    408/2, 29/2,
			    408, 29,
			    0, 0, 408/512, 29/128,
			    sb.sprites.texid)

   sb.sprites.file = sprite("file",	
			    129/2, 12/2,
			    129, 12, 0, 32/128, 129/512, 44/128,
			    sb.sprites.texid)

   sb.sprites.list = sprite("list",	
			    247/2, 12/2,
			    247, 12,
			    164/512, 32/128, 411/512, 44/128,
			    sb.sprites.texid)

   sb.sprites.copy = sprite("copy",	
			    185/2, 19/2,
			    185, 19,
			    0, 48/128, 185/512, 67/128,
			    sb.sprites.texid)

   sb.sprites.url = sprite("url",	
			   185/2, 19/2,
			   185, 19,
			   186/512, 48/128, 371/512, 67/128,
			   sb.sprites.texid)

   sb.sprites.jess = sprite("jess",	
			    107/2, 53/2,
			    107, 53,
			    0/512, 72/128, 107/512, 125/128,
			    sb.sprites.texid)

   sb.sprites.proz = sprite("prozak",	
			    56/2, 80/2,
			    56, 80,
			    453/512, 0/128, 510/512, 80/128,
			    sb.sprites.texid)

   sb.sprites.vmu = sprite("vmu",	
			   104/2, 62/2,
			   104, 62,
			   108/512, 65/128, 212/512, 127/128,
			   sb.sprites.texid,1)

   sb.fl.title_sprite = sb.sprites.file
   sb.fl.icon_sprite = sb.sprites.jess
   sb.pl.title_sprite = sb.sprites.list
   sb.pl.icon_sprite = sb.sprites.proz

end

--- Create a song-browser application.
---
--- @param  owner  Owner application (nil for desktop).
--- @param  name   Name of application (nil for "song browser").
---
--- @return song-browswer application
--- @retval nil Error
---
function song_browser_create(owner, name)
   local sb
   local z

   if not owner then
      owner = evt_desktop_app
   end
   if not name then name = "song browser" end

   -- Song-Browser default style
   -- --------------------------
   local style = {
      bkg_color		= { 0.8, 0.4, 0.0, 0.0,  0.8, 0.3, 0.3, 0.3 },
      border			= 5,
      span            = 1,
      file_color		= { 1, 1, 0.8, 0 },
      dir_color		= { 1, 1, 1, 0 },
      cur_color		= { 1, 0.5, 0, 0,  0.5, 1, 0.7, 0 },
      text            = {font=0, size=16, aspect=1}
   }

   function song_browser_update_cdrom(sb, frametime)
      local timeout = sb.cdrom_check_timeout - frametime
      if timeout <= 0 then
	 timeout = 0.5
	 local st,ty,id = cdrom_status(1)

	 if id == 0 or sb.cdrom_id == 0 then
	    local path = (sb.fl.dir and sb.fl.dir.path) or "/"
	    local incd = strsub(path,1,3) == "/cd"

	    if id == 0 and incd and st == "nodisk" then
	       print("Drive empty")
	       song_browser_loaddir(sb,"/")
	    elseif id ~= sb.cdrom_id then
	       if id ~= 0 then
		  print(format("New CD detected #%X", id))
		  if incd then song_browser_loaddir(sb,"/cd") end
	       else
		  print("No more CD in drive")
	       end
	    end
	 end
	 sb.cdrom_stat, sb.cdrom_type, sb.cdrom_id = st, ty, id
--	 print("CD:"..sb.cdrom_stat..","..sb.cdrom_type..","..sb.cdrom_id)
      end
      sb.cdrom_check_timeout = timeout
   end

   function song_browser_update_loaddir(sb, frametime)
      if sb.loaddir then
	 local loading = (tag(sb.loaddir) == entrylist_tag and
			  sb.loaddir.loading) or 2
	 if loading == 2 then
	    local i
	    if type(sb.locatedir) == "string" then
	       i = sb.loaddir.n
	       while i >= 1 and sb.loaddir[i].file ~= sb.locatedir do
		  i = i - 1
	       end
	    end
	    sb.locatedir = nil
	    sb.fl:change_dir(sb.loaddir, i)
	    sb.loaddir = nil
	 end
      end
   end

   function song_browser_update_recloader(sb, frametime)
      if sb.recloader then
	 local dir,i = sb.recloader.dir, sb.recloader.cur

	 if not dir.loading and i >= dir.n then
	    i = sb.recloader.cur2
	    while i < dir.n do
	       i = i + 1
	       local entry = dir[i]
	       if entry and entry.size < 0 and
		  entry.file ~= "." and entry.file ~= ".."
	       then
		  local subel = entrylist_new()
		  if subel then
		     entrylist_load(subel,
				    canonical(dir.path.."/"..entry.file),
				    sb.el_filter)
		     sb.recloader.cur2 = i
		     sb.recloader = {
			parent = sb.recloader,
			dir = subel,
			cur = 0,
			cur2 = 0
		     }
		     return 1
		  end
	       end
	    end
	    if i >= dir.n then
	       sb.recloader = sb.recloader.parent
	    end
	 else
	    while i < dir.n do
	       i = i + 1
	       if dir[i].size > 0 then
		  sbpl_insert(sb, dir[i])
		  sb.recloader.cur = i
		  return 1
	       end
	    end
	    sb.recloader.cur = i
	 end
      end
   end

   --- Song-Browser update (handles fade in / fade out).
   --  ------------------------------------------------
   function song_browser_update(sb, frametime)
      song_browser_update_cdrom(sb, frametime)
      song_browser_update_loaddir(sb, frametime)
      song_browser_update_recloader(sb, frametime)

      if not sb.sleeping then
	 sb.idle_time = sb.idle_time or 0
	 if sb.idle_time > 20 then
	    sb:asleep()
	 end
	 sb.idle_time = sb.idle_time + frametime
      end

      if sb.key_time then
	 sb.key_time = sb.key_time + frametime
	 if sb.key_time >= 1 then
	    sb.search_str = nil
	    sb.key_time = nil
	 end
      end

      if sb.stopping and playa_fade() == 0 then
	 playa_stop()
	 sb.stopping = nil
	 sb.playlist_idx = nil
      elseif sb.playlist_idx then
	 if not sb.pl.dir or not sb.pl.dir.n then
	    sb.playlist_idx = nil
	 else 
	    local n = sb.pl.dir.n
	    while playa_play() == 0 and sb.playlist_idx < n do
	       sb.playlist_idx = sb.playlist_idx + 1
	       local path = sb.pl:fullpath(sb.pl.dir[sb.playlist_idx])
	       if path then
		  song_browser_play(sb, path, 0, 0)
	       end
	    end
	    if playa_play() == 0 then
	       song_browser_stop(sb)
	    end
	 end
      end

      sb.fl:update(frametime)
      sb.pl:update(frametime)
   end

   --- Song-Browser handle.
   --  -------------------
   function song_browser_handle(sb, evt)
      local key = evt.key
      if key == evt_shutdown_event then
	 sb:shutdown()
	 return evt
      end
      if sb.closed then
	 return evt
      end

      local action

      -- $$$ Test 96 '`' to prevent event eating for the console switching.
      if key >= 0 and key<128 and key ~= 96 then
	 local key_char 
	 key_char = strchar(key)
	 if key_char then
	    if strfind(key_char,"%a") then
	       key_char = "[".. strlower(key_char) .. strupper(key_char) .."]"
	    end
	    if not strfind("`'\"^$()%.[]*+-?`",key_char,1,1) then
	       sb.search_str = (sb.search_str or "^") .. key_char
	       action = sb.cl:locate_entry_expr(sb.search_str .. ".*")
	       sb.key_time = 0
	    end
	 end
      elseif key == gui_focus_event then
	 -- nothing to do, this will awake song-browser
      elseif key == gui_unfocus_event then
	 sb:asleep()
	 return
      elseif gui_keyconfirm[key] then
	 action = sb:confirm()
      elseif gui_keycancel[key] then
	 action = sb:cancel()
      elseif gui_keyselect[key] then
	 action = sb:select()
      elseif gui_keyup[key] then
	 action = sb.cl:move_cursor(-1)
      elseif gui_keydown[key] then
	 action = sb.cl:move_cursor(1)
      elseif gui_keyleft[key] then
	 if sb.cl ~= sb.fl then
	    sb.cl = sb.fl
	    action = 2
	 end
      elseif gui_keyright[key] then
	 if sb.cl ~= sb.pl then
	    sb.cl = sb.pl
	    action = 2
	 end
      else
	 return evt
      end

--       if action and action >= 2 then
-- 	 local entry = sb.cl:get_entry()
-- 	 vmu_set_text(entry and entry.name)
--       end
      sb:open()
   end

   --- Song-Browser asleep.
   --  -------------------
   function song_browser_asleep(sb)
      sb.sleeping = 1
      if not sb.closed then
	 sb.cl.fade_min = 0.3
	 sb.cl:close()
      end
   end

   --- Song-Browser awake.
   --  ------------------
   function song_browser_awake(sb)
      sb.sleeping = nil
      sb.idle_time = 0
      if not sb.closed then
	 sb.cl:open()
      end
   end

   --- Song-Browser open.
   --  -----------------
   function song_browser_open(sb, which)
      sb.closed = nil
      if not which then
	 sb:awake()
	 local ol = (sb.cl == sb.fl and sb.pl) or sb.fl
	 ol.fade_min = 0.3
	 ol:close()
      elseif which == 1 then
	 sb.fl:open()
      else
	 sb.pl:open()
      end	  
   end

   --- Song-Browser close.
   --  ------------------
   function song_browser_close(sb, which)
      if not which then
	 sb.fl.fade_min = 0
	 sb.pl.fade_min = 0
	 sb.fl:close()
	 sb.pl:close()
	 sb.closed = 1
	 sb.idle_time = 0
      elseif which == 1 then
	 sb.fl.fade_min = 0.3
	 sb.fl:close()
      else
	 sb.pl.fade_min = 0.3
	 sb.pl:close()
      end
   end

   --- Song-Browser shutdown.
   --  ---------------------
   function song_browser_shutdown(sb)
      if not sb then return end
      sb.fl:shutdown()
      sb.pl:shutdown()
      dl_set_active(sb.dl, nil)
      sb.dl = nil;
      vmu_set_text("dcplaya")
   end

   --- Song-Browser draw.
   --  -----------------
   function song_browser_draw(sb)
      sb.fl:draw()
      sb.pl:draw()
   end

   function song_browser_list_draw_background(fl,dl)
      local v = mat_new(4,8)
      local x1,y1,x2,y2,x3,y3,x4,y4
      local border = fl.border * 0.75
      x1 = 0
      x2 = x1 + border
      x4 = fl.bo2[1]
      x3 = x4 - border
      y1 = 0
      y2 = y1 + border
      y4 = fl.bo2[2]
      y3 = y4 - border

      local a1,r1,g1,b1 = 1.0, 1.0, 0.0, 0.0
      local a2,r2,g2,b2 = 1.0, 1.0, 1.0, 0.0

      local w = {
	 {x1, y1, 0.1, 1.0,  a1, r1, g1, b1 }, -- 1
	 {x4, y1, 0.1, 1.0,  a1, r1, g1, b1 }, -- 2
	 {x1, y4, 0.1, 1.0,  a1, r1, g1, b1 }, -- 3
	 {x4, y4, 0.1, 1.0,  a1, r1, g1, b1 }, -- 4

	 {x2, y2, 0.1, 1.0,  a2, r2, g2, b2 }, -- 5
	 {x3, y2, 0.1, 1.0,  a2, r2, g2, b2 }, -- 6
	 {x2, y3, 0.1, 1.0,  a2, r2, g2, b2 }, -- 7
	 {x3, y3, 0.1, 1.0,  a2, r2, g2, b2 }, -- 8
      }

      local def = {
	 { 1, 5, 3, 7, 0.6 },
	 { 1, 2, 5, 6, 1.0 },
	 { 6, 2, 8, 4, 0.4 },
	 { 7, 8, 3, 4, 0.3 },
      }

      local i,d
      for i,d in def do
	 local m =  { 1,1,1,1, 1, d[5], d[5], d[5] }
	 set_vertex(v[1], w[d[1]] * m)
	 set_vertex(v[2], w[d[2]] * m)
	 set_vertex(v[3], w[d[3]] * m)
	 set_vertex(v[4], w[d[4]] * m)
	 dl_draw_strip(dl,v);
      end

      if fl.title_sprite then
	 fl.title_sprite:set_color(1,r2,g2,b2)
	 fl.title_sprite:draw(dl, (x1+x4) * 0.5, y1-fl.title_sprite.h, 0)
      end

      if fl.icon_sprite then
	 local w,h = fl.icon_sprite.w, fl.icon_sprite.h
	 fl.icon_sprite:set_color(0.4,r2,g2,b2)
	 fl.icon_sprite:draw(dl, x3 - w * 0.5, y3 - h * 0.5, 0.05)
      end

      if fl.draw_background_old then
	 fl:draw_background_old(dl)
      end
   end

   --- Song-Browser set color.
   --
   function song_browser_set_color(sb, a, r, g, b)
      sb.fl:set_color(a,r,g,b)
      sb.pl:set_color(a,r,g,b)
   end

   function song_browser_loaddir(sb,path,locate)
      if not test("-d",path) or sb.loaddir then
	 return
      end
      sb.loaddir = entrylist_new()
      if sb.loaddir then
	 if entrylist_load(sb.loaddir,path,sb.el_filter) then
	    sb.locatedir = locate
	    return 1
	 end
      end
      sb.locatedir = nil
      sb.loaddir = nil
   end

   --- Song-Browser confirm.
   --
   function song_browser_confirm(sb)
      return sb.cl:confirm(sb)
   end

   --- Song-Browser cancel.
   --
   function song_browser_cancel(sb)
      return sb.cl:cancel(sb)
   end

   --- Song-Browser select.
   --
   function song_browser_select(sb)
      return sb.cl:select(sb)
   end

   sb = {
      -- Application
      name = name,
      version = 1.0,
      handle = song_browser_handle,
      update = song_browser_update,
      icon_name = "song-browser",
      
      -- Methods
      open = song_browser_open,
      close = song_browser_close,
      set_color = song_browser_set_color,
      draw = song_browser_draw,
      confirm = song_browser_confirm,
      cancel = song_browser_cancel,
      select = song_browser_select,
      shutdown = song_browser_shutdown,
      asleep = song_browser_asleep,
      awake = song_browser_awake,
      
      -- Members
      style = style,
      z = gui_guess_z(owner,z),
      fade = 0,
      dl = dl_new_list(1024, 1)
   }

   print("sb.z "..type(sb.z).." "..tostring(sb.z))

   local x,y,z
   local box = { 0, 0, 256, 210 }
   local minmax = { box[3], box[4], box[3], box[4] }

   x = 42
   y = 120
   z = sb.z - 2

   --- Stop current music and playlist running.
   function song_browser_stop(sb)
      sb.stopping = 1
      sb.playlist_idx = nil
      sb:draw()
      playa_fade(-1)
   end

   --- Start a new music
   function song_browser_play(sb, filename, track, immediat)
      sb.stopping = nil
      playa_play(filename, track, immediat)
      sb:draw()
      playa_fade(2)
   end

   function song_browser_ask_background_load(sb)
      if tag(background) == background_tag then
	 return
      end
      local r = gui_yesno("The background library has not been properly loaded. Do you want to try to load it ?" , 256, "Missing background", "load background library", "cancel")
      if r == 1 then
	 dolib("background",1)
      end
   end

   function songbrowser_load_image(sb, filename, mode)
      if not background_tag or tag(background) ~= background_tag then
	 gui_ask('The background library is not available, you will not be able to display background images.<br><img name="grimley" src="stock_grimley.tga" scale="1.5">',"close",256,"background error")
	 return
      end
      if not (test("-f", filename) and
	      background:set_texture(filename,mode or sb.background_mode))
      then
	 printf ("[song_browser] : loading background %q failed", filename)
      end
   end

   function songbrowser_info_image(sb, filename)
      local info = image_info(filename)
      local path, leaf = get_path_and_leaf(filename)
      local text
      
      if type(info) ~= "table" then
	 text = '<left><img name="grimley" src="stock_grimley.tga" scale="1.5"><br><center>Can not get information from image file.<br>.'
      else
	 text = '<left><img name="smiley" src="stock_smiley.tga" scale="1.5"><br><center>' ..
	    format("%s image<br>%dx%dx%d, %s<br>",
		   (info.format or "unknown"),
		   (info.w or -1),
		   (info.h or -1),
		   (info.bpp or -1),
		   (info.type or "unknown pixel format"))
      end
      gui_ask(text, "<center>close", 300, format("info on %s",leaf or ""))
   end

   -- ----------------------------------------------------------------------
   -- filelist "confirm" actions
   -- ----------------------------------------------------------------------

   function songbrowser_any_action(sb, action, fl)
      if type(action) ~= "string" then return end
      fl = fl or sb.cl
      if not fl then return end
      if not fl.actions or type(fl.actions[action]) ~= "table" then
	 printf("[songbrowser] : unknown action %q", action)
	 return
      end
      local entry_path = fl:fullpath()
      if not entry_path then return end
      local ftype, major, minor = filetype(entry_path)

      -- $$$
      printf("%q on %q [%q,%q]", action, entry_path, major, minor)

      local func = fl.actions[action][major] or fl.actions[action].default
      if type(func) == "function" then
	 return func(fl,sb,action,entry_path)
      end
      -- $$$
      printf("No %q action for %q", action, entry_path)
   end

   --    *     - @b nil  if action failed
   --    *     - @b 0    ok but no change
   --    *     - @b 1    if entry is "confirmed"
   --    *     - @b 2    if entry is "not confirmed" but change occurs
   --    *     - @b 3    if entry is "confirmed" and change occurs
   function sbfl_confirm(fl, sb)
      print("sbfl_confirm")
      return songbrowser_any_action(sb, "confirm", fl)
   end

   function sbfl_confirm_dir(fl, sb, action, entry_path)
      if not test("-d", entry_path) then return end
      local idx = fl:get_pos();
      local entry = fl.dir[idx];
      local name = entry and (entry.file or entry.name)
      if not name then return end

      local locate_me
      if name == "." then
	 locate_me = "."
	 if strfind(entry_path,"^/cd.*") then
	    clear_cd_cache()
	 end
      elseif name == ".." then
	 local p
	 p,locate_me = get_path_and_leaf(entry.path or fl.path)
      end
      song_browser_loaddir(sb, entry_path, locate_me)
   end

   function sbfl_confirm_music(fl, sb, action, entry_path)
-- $$$ problem with cdda ... 
--      if not test("-f",entry_path) then return end
      sb.playlist_idx = nil
      song_browser_play(sb, entry_path, 0, 1)
      return 1
   end

   function sbfl_confirm_playlist(fl, sb, action, entry_path)
      local dir = playlist_load(entry_path)
      if not dir then return end
      sb.pl:change_dir(dir)
      return 1
   end

   function sbfl_confirm_image(fl, sb, action, entry_path)
      song_browser_ask_background_load(sb)
      songbrowser_load_image(sb,entry_path)
   end

   function sbfl_confirm_plugin(fl, sb, action, entry_path)
      if not test("-f",entry_path) then return end
      return driver_load(entry_path)
   end

   function sbfl_confirm_lua(fl, sb, action, entry_path)
      if not test("-f",entry_path) then return end
      return dofile(entry_path)
   end

   -- ----------------------------------------------------------------------
   -- filelist "select" actions
   -- ----------------------------------------------------------------------

   function sbfl_select(fl, sb)
      songbrowser_any_action(sb,"select",fl)
   end
   
   function sbfl_select_dir(fl, sb, action, entry_path)
      if not test("-d", entry_path) then return end
      return sbpl_insertdir(sb, entry_path)
   end
   
   function sbfl_select_music(fl, sb, action, entry_path)
-- $$$ problem with cdda ... 
--       if not test("-f",entry_path) then return end
      local pos = fl:get_pos()
      if pos then
	 return sbpl_insert(sb, fl.dir[pos])
      end
   end
   
   function sbfl_select_playlist(fl, sb, action, entry_path)
      local path,leaf = get_path_and_leaf(entry_path)
      if not leaf then return end
      
      local def = {
	 root = ":"..leaf..":load{load},insert{load},append{load}",
	 cb = {
	    load = function (menu, idx)
		      local root_menu = menu.root_menu
		      local sb = root_menu.target
		      local entry_path = root_menu.__entry_path or ""
		      local dir = playlist_load(entry_path)
		      local odir = sb.pl.dir
		      if not dir then return end
		      if idx == 1 then
			 sb.pl:change_dir(dir)
			 return 1
		      elseif idx == 2 and odir then
			 pos = (sb.pl:get_pos() or 0) + 1
			 local i,v
			 for i,v in dir do
			    if type(v) == "table" then
			       tinsert(odir, pos, v)
			       pos = pos+1
			    end
			 end
			 sb.pl:change_dir(odir)
			 return 1
		      elseif idx == 3 and odir then
			 local i,v
			 for i,v in dir do
			    if type(v) == "table" then
			       tinsert(odir,v)
			    end
			 end
			 sb.pl:change_dir(odir)
			 return 1
		      end
		   end,
	 },
      }
      local menudef = menu_create_defs(def, sb)
      fl.menu = menu_create(sb, "filelist-menu", menudef)
      if tag(fl.menu) == menu_tag then
	 fl.menu.target = sb
	 fl.menu.target_pos = fl.pos + 1
	 fl.menu.__entry_path = entry_path
      end
   end

   function sbfl_select_image(fl, sb, action, entry_path)
      local path,leaf = get_path_and_leaf(entry_path)
      if not leaf then return end
      
      local def = {
	 root = ":"..leaf..":info{info},background>",
	 cb = {
	    info = function (menu, idx)
		      local root_menu = menu.root_menu
		      local sb = root_menu.target
		      songbrowser_info_image(sb,root_menu.__entry_path)
		   end,
	    bkg = function (menu, idx)
		     local root_menu = menu.root_menu
		     local sb = root_menu.target
		     local entry_path = root_menu.__entry_path or ""
		     local mode = idx and menu.fl.dir[idx].name
 		     song_browser_ask_background_load(sb)
 		     songbrowser_load_image(sb,entry_path,mode)
		  end,
	 },
	 sub = {
	    background = ":background:scale{bkg},center{bkg},tile{bkg}"
	 }
      }

      local menudef = menu_create_defs(def, sb)
      fl.menu = menu_create(sb, "filelist-menu", menudef)
      if tag(fl.menu) == menu_tag then
	 fl.menu.target = sb
	 fl.menu.target_pos = fl.pos + 1
	 fl.menu.__entry_path = entry_path
      end
   end

   function sbfl_select_plugin(fl, sb, action, entry_path)
      if not test("-f",entry_path) then return end
      return driver_load(entry_path)
   end

   function sbfl_select_lua(fl, sb, action, entry_path)
      if not test("-f",entry_path) then return end
      return dofile(entry_path)
   end

   -- ----------------------------------------------------------------------
   -- filelist "cancel" actions
   -- ----------------------------------------------------------------------
   function sbfl_cancel(fl, sb)
      songbrowser_any_action(sb,"cancel",fl)
   end

   function sbfl_cancel_default(fl, sb, action, entry_path)
      if playa_play() == 1 then
	 song_browser_stop(sb);
      end
   end

   function sbfl_open_menu(fl,sb)
      print("TO DO : File list open menu")
   end


   sb.fl = textlist_create(
			   {
			      pos = {x, y, z},
			      box = minmax,
			      flags=nil,
			      dir=entrylist_new(),
			      filecolor = sb.style.file_color,
			      dircolor  = sb.style.dir_color,
			      bkgcolor  = sb.style.bkg_color,
			      curcolor  = sb.style.cur_color,
			      border    = sb.style.border,
			      span      = sb.style.span,
			      confirm   = sbfl_confirm,
			      owner     = sb,
			      not_use_tt = 1
			   })
   sb.fl.cancel = sbfl_cancel
   sb.fl.select = sbfl_select
   sb.fl.open_menu = sbfl_open_menu

   sb.fl.fade_min = 0.3
   sb.fl.draw_background_old = sb.fl.draw_background
   sb.fl.draw_background = song_browser_list_draw_background

   sb.fl.curplay_color = {1,1,0,0}


   function sbpl_clear(sb)
      sb.pl:change_dir(nil)
   end

   function sbpl_insert(sb, entry)
      sb.pl:insert_entry(entry)
   end

   function sbpl_insertdir(sb, path)
      if sb.recloader then return end
      local dir = entrylist_new()
      if dir then
	 sb.recloader = {
	    dir = dir,
	    cur = 0,
	    cur2 = 0,
	 }
	 entrylist_load(dir,path,sb.el_filter,"DM")
      end
   end

   function sbpl_stop(sb)
      song_browser_stop(sb)
   end

   function sbpl_run(sb,pos)
      local pl = sb.pl
      pos = (pos and pos-1) or pl.pos
      if pl.dir.n and pos < pl.dir.n then
	 playa_stop()
	 sb.playlist_idx = pos
      end
   end

   function sbpl_remove(sb,pos)
      pos = sb.pl:get_pos(pos)
      if pos then
	 if sb.playlist_idx and pos <= sb.playlist_idx then
	    sb.playlist_idx = sb.playlist_idx - 1
	 end
	 sb.pl:remove_entry()
      end
   end

   function sbpl_save(sb,owner)
      owner = owner or sb
      local fs = fileselector("Save playlist",
			      "/ram/dcplaya/playlists",
			      "playlist.m3u",owner)
      local fname = evt_run_standalone(fs)
      if type(fname) ~= "string" then
	 return
      end
      local result
      if test("-d",fname) then
      elseif test("-f",fname) then
      else
	 result = playlist_save(fname, sb.pl.dir)
      end
      return result
   end

   function sbpl_shuffle(sb)
      local dir = sb.pl.dir
      if not dir or (dir.n or 0) < 1 then return end
      local n = dir.n
      local pos = sb.pl:get_pos() or 0
      local i,j,idx
      local locate = nil

      -- save index
      if pos then
	 dir[pos].__sortpos = pos
      end
      
      local order = {}
      for i=1,n do
	 local idx = random(1,getn(dir))
	 local entry = tremove(dir,idx)
	 if entry.__sortpos then
	    if locate then print("[shuffle] : locate twice !") end
	    locate = i
	    entry.__sortpos = nil
	 end
	 tinsert(order,entry)
      end
      sb.pl:change_dir(order, locate)
   end

   function sbpl_sort_any(sb, cmp)
      local dir = sb.pl.dir
      if not dir or (dir.n or 0) < 1 then return end
      local n = dir.n
      local pos = sb.pl:get_pos()
      -- Save the current position
      if pos then dir[pos].__sortpos = 1 end
      
      if type(cmp) == "function" then
-- 	 print("sorting with function")
	 xsort(dir,cmp,sb)
      elseif type(cmp) == "table" then
-- 	 printf("sorting with table of functions (%d)", getn(cmp))
-- 	 local i,v
-- 	 for i,v in cmp do
-- 	    printf("%q=%q",i,tostring(v))
-- 	    if type(v) == "function" then v() end
-- 	 end
	 xsort(dir, sbpl_cmp_any, cmp)
      else
	 print("[song_browser] : invalid sort parameter")
	 return
      end
   
--       local i
--       -- $$$
--       for i=1,n do
-- 	 dump(dir[i],tostring(i))
--       end

      for i=1,n do
	 if dir[i].__sortpos then
	    dir[i].__sortpos = nil
	    sb.pl:change_dir(dir, i)
	    return
	 end
      end
      sb.pl:change_dir(dir)
   end
   
   function sbpl_sort_get_names(a,b)
      return strlower(a.name or a.file or ""), strlower(b.name or b.file or "")
   end

   function sbpl_sort_get_files(a,b)
      return strlower(a.file or a.name or ""), strlower(b.file or b.name or "")
   end

   function sbpl_sort_get_pathes(a,b)
      return strlower(a.path or "/"), strlower(b.path or "/")
   end

   function sbpl_cmp_type(a,b,sb)
      if not a then print("types") return end
      return (a.type or 0) - (b.type or 0)
   end

   function sbpl_cmp_string(a,b,sb)
      if a<b then return -1
      elseif a>b then return 1
      end
      return 0
   end

   function sbpl_cmp_name(a,b,sb)
      if not a then print("names") return end
      local s1,s2 =  sbpl_sort_get_names(a,b)
      return sbpl_cmp_string(s1,s2,sb)
   end

   function sbpl_cmp_path(a,b,sb)
      if not a then print("pathes") return end
      local s1,s2 =  sbpl_sort_get_pathes(a,b)
      return sbpl_cmp_string(s1,s2,sb)
   end


   function sbpl_cmp_any(a,b,cmptable)
      local i,v
      for i,v in cmptable do
	 local r = v(a,b)
	 if r ~= 0 then return r end
      end
      return 0
   end

   function sbpl_sort_by_type(a,b,sb)
      local r = sbpl_cmp_type(a,b,sb)
      if r == 0 then
	 local s1,s2 = sbpl_sort_get_names(a,b)
	 r = sbpl_cmp_string(s1,s2,sb)
      end
      return r or 0
   end
	 
   function sbpl_sort_by_name(a,b,sb)
      local s1,s2 =  sbpl_sort_get_names(a,b)
      local r = sbpl_cmp_string(s1,s2,sb)
      if r == 0 then
	 r = sbpl_cmp_type(a,b,sb)
      end
      return r or 0
   end

   function sbpl_sort_by_path(a,b,sb)
      local s1,s2 =  sbpl_sort_get_pathes(a,b)
      local r = sbpl_cmp_string(s1,s2,sb)
      if r == 0 then
	 r = sbpl_cmp_type(a,b,sb)
      end
      return r or 0
   end


   function sbpl_confirm(pl, sb)
      sbpl_run(sb)
   end

   function sbpl_cancel(pl, sb)
      sbpl_remove(sb)
   end


   function sbpl_open_menu(pl, sb)
      if tag(pl.menu) == menu_tag then
	 pl.menu:close()
      end

      local entry = pl:get_entry()
      if not entry then return end
      local root = ""
      local fname = entry.file or entry.name
      if type(fname) == "string" then root = ":"..fname..":" end
      root = root.."run{run},remove{remove},shuffle{shuffle},sort>sort,clear{clear},save{save}"

      function sbpl_make_menu_sort_func(sb,str,nocache)
	 sb.sort_func_cache = (type(sb.sort_func_cache) == "table" and
			       sb.sort_func_cache) or {}
	 if not nocache and type(sb.sort_func_cache[str]) == "function" then
	    return sb.sort_func_cache[str]
	 end

	 local table, s = {}, str
	 while 1 do
	    local t = strsub(s,1,1)
	    if strlen(t) == 0 then
	       if getn(table) > 0 then
		  table.n = nil
		  sb.sort_func_cache[str] =
		     function (menu)
			local sb = menu.root_menu.target
			sbpl_sort_any(sb, %table)
		     end
		  return sb.sort_func_cache[str]
	       end
	    end
	    s = strsub(s,2)
	    if t == "n" then
	       e = sbpl_cmp_name
	    elseif t == "t" then
	       e = sbpl_cmp_type
	    elseif t == "p" then
	       e = sbpl_cmp_path
	    else
	       e = nil
	    end
	    if e then tinsert(table,e) end
	 end
      end

      local cb = {
	 run = function (menu)
		  local sb = menu.root_menu.target
		  sbpl_run(sb)
	       end,
	 remove = function (menu)
		     local sb = menu.root_menu.target
		     sbpl_remove(sb)
		  end,
	 clear = function (menu)
		    local sb = menu.root_menu.target
		    sbpl_clear(sb)
		 end,
	 save = function (menu)
		   local sb = menu.root_menu.target
		   sbpl_save(sb,menu)
		end,
	 shuffle = function (menu)
		      local sb = menu.root_menu.target
		      sbpl_shuffle(sb)
		   end,

	 sort_by_nt = sbpl_make_menu_sort_func(sb,"ntp"),
	 sort_by_np = sbpl_make_menu_sort_func(sb,"npt"),
	 sort_by_tn = sbpl_make_menu_sort_func(sb,"tnp"),
	 sort_by_tp = sbpl_make_menu_sort_func(sb,"tpn"),
	 sort_by_pt = sbpl_make_menu_sort_func(sb,"ptn"),
	 sort_by_pn = sbpl_make_menu_sort_func(sb,"pnt"),

      }

      local def = {
	 root=root,
	 cb = cb,
	 sub = {
	    sort = {
	       root = ":sort by ...:name>,type>,path>",
	       sub = {
		  name = ":... name and ...:type{sort_by_nt},path{sort_by_np}",
		  type = ":... type and ...:name{sort_by_tn},path{sort_by_tp}",
		  path = ":... path and ...:name{sort_by_pn},type{sort_by_pt}",
	       },
	    },
	 }
      }

      local menudef = menu_create_defs(def, sb)
      pl.menu = menu_create(sb, "playlist-menu", menudef)
      if tag(pl.menu) == menu_tag then
	 pl.menu.target = sb
	 pl.menu.target_pos = pl.pos + 1
      end
   end

   function sbpl_select(pl, sb)
      pl:open_menu(sb)
   end

   song_browser_filelist_actions = {
      confirm = {
	 dir = sbfl_confirm_dir,
	 music = sbfl_confirm_music,
	 image = sbfl_confirm_image,
	 playlist = sbfl_confirm_playlist,
	 default = sbfl_confirm_default,
      },
      select = {
	 dir = sbfl_select_dir,
	 music = sbfl_select_music,
	 image = sbfl_select_image,
	 playlist = sbfl_select_playlist,
	 default = sbfl_select_default,
      },
      cancel = {
	 default = sbfl_cancel_default,
      },
   }

   sb.fl.actions = song_browser_filelist_actions

   x = 341
   sb.pl = textlist_create(
			   {
			      pos = {x, y, z+1},
			      box = minmax,
			      flags=nil,
			      dir={},
			      filecolor = sb.style.file_color,
			      dircolor  = sb.style.dir_color,
			      bkgcolor  = sb.style.bkg_color,
			      curcolor  = sb.style.cur_color,
			      border    = sb.style.border,
			      span      = sb.style.span,
			      owner     = sb,
			      not_use_tt = 1
			   } )
   sb.pl.cookie = sb -- $$$ This should be owner !!!
   sb.pl.confirm = sbpl_confirm
   sb.pl.cancel = sbpl_cancel
   sb.pl.select = sbpl_select
   sb.pl.open_menu = sbpl_open_menu
   sb.pl.curplay_color = sb.fl.curplay_color
   sb.pl.draw_entry = function (fl, dl, idx, x , y, z)
			 local sb = fl.cookie
			 local entry = fl.dir[idx]
			 local color = fl.dircolor
			 if sb.playlist_idx and idx == sb.playlist_idx then
			    color = fl.curplay_color or color
			 end
			 dl_draw_text(dl,
				      x, y, z+0.1,
				      color[1],color[2],color[3],color[4],
				      entry.name)
		      end


   sb.pl.fade_min = sb.fl.fade_min
   sb.pl.draw_background_old = sb.pl.draw_background
   sb.pl.draw_background = sb.fl.draw_background

   sb.cl = sb.fl

   song_browser_create_sprite(sb)
   song_browser_loaddir(sb,"/","cd")
--    sb.fl:change_dir(sb.fl.dir)


   function songbrowser_menucreator(target)
      local sb = target;
      local cb = {
	 toggle = function(menu)
		     local sb = menu.target
		     if sb.closed then
			sb:open()
		     else
			sb:close()
		     end
		  end,
	 saveplaylist = function(menu)
			   local sb = menu.root_menu.target
			   sbpl_save(sb,menu)
			end,
      }
      local root = ":" .. target.name .. ":" .. 
	 "toggle{toggle},playlist >playlist"
      local def = {
	 root=root,
	 cb = cb,
	 sub = {
	    playlist = ":playlist:save{saveplaylist}",
	 }
      }
      return menu_create_defs(def , target)
   end

   -- cdrom
   sb.cdrom_stat, sb.cdrom_type, sb.cdrom_id = cdrom_status(1)
   sb.cdrom_check_timeout = 0

   -- filer
   sb.el_filter = "DXIMP"

   -- Menu
   sb.mainmenu_def = songbrowser_menucreator

   sb:set_color(0, 1, 1, 1)
   sb:draw()
   sb:open()

   evt_app_insert_first(owner, sb)

   return sb
end

if not entrylist_tag then
   driver_load(plug_el)
end

-- Load texture for application icon
local tex = tex_get("song-browser")
   or tex_new(home .. "lua/rsc/icons/song-browser.tga")

if song_browser then
   evt_shutdown_app(song_browser)
end

song_browser = song_browser_create()

function song_browser_kill(sb)
   sb = sb or song_browser
   if sb then
      evt_shutdown_app(sb)
      if sb == song_browser then song_browser = nil end
   end
end
