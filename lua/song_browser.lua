--- @ingroup  dcplaya_lua_sb_app
--- @file     song_browser.lua
--- @author   benjamin gerard <ben@sashipa.com>
--- @date     2002
--- @brief    song browser application.
---
--- $Id: song_browser.lua,v 1.60 2003-03-22 10:19:16 ben Exp $
---

--- @defgroup dcplaya_lua_sb_app Song Browser
--- @ingroup  dcplaya_lua_app
--- @brief    song browser application
---
---  @par song-browser introduction
---
---   The song browser application is used to browse dcplaya filesystem and
---   a perform specific operation on file depending of the type. The song
---   browser application handles playlist. Normal behaviour is to have only
---   one instance of a song browser application. It is stored in the global
---   variable song_browser.
---
--- @author   benjamin gerard <ben@sashipa.com>
---
--- @{

song_browser_loaded = nil
if not dolib("textlist") then return end
if not dolib("gui") then return end
if not dolib("sprite") then return end
if not dolib("fileselector") then return end
if not dolib("playlist") then return end
if not dolib("lua_colorize") then return end





--- Update CDROM status.
--- @param sb song-browser application
--- @param cd cdrom_change_event
--- @internal
--
function song_browser_update_cdrom(sb, cd)
   local st,ty,id = cd.cdrom_status,cd.cdrom_disk,cd.cdrom_id
   if id == 0 or sb.cdrom_id == 0 then
      local path = (sb.fl.dir and sb.fl.dir.path) or "/"
      local incd = strsub(path,1,3) == "/cd"
      if id == 0 and incd and st == "nodisk" then
	 song_browser_loaddir(sb,"/")
      elseif id ~= sb.cdrom_id then
	 if id ~= 0 then
	    if incd then song_browser_loaddir(sb,"/cd") end
	 else
	    --	       print("No more CD in drive")
	 end
      end
   end
   sb.cdrom_stat, sb.cdrom_type, sb.cdrom_id = st, ty, id
end

--- Update the loaddir status.
--- @internal
--
function song_browser_update_loaddir(sb, frametime)
   if sb.loaded_dir then
      local loading = (tag(sb.loaded_dir) == entrylist_tag and
		       sb.loaded_dir.loading) or 2
      if loading == 2 then
	 local i
	 if type(sb.locatedir) == "string" then
	    i = sb.loaded_dir.n
	    while i >= 1 and sb.loaded_dir[i].file ~= sb.locatedir do
	       i = i - 1
	    end
	 end
	 sb.locatedir = nil
	 sb.fl:change_dir(sb.loaded_dir, i)
	 sb.loaded_dir = nil
      end
   end
end

--- Update the playlist recursive directory loading.
--- @internal
--
function song_browser_update_recloader(sb, frametime)
   if sb.recloader then
      local dir,i = sb.recloader.dir, sb.recloader.cur
      local filter = sb.recloader.el_filter or sb.el_filter or "DMI"

      if not dir.loading and i >= dir.n then
	 i = (sb.recursive_mode and sb.recloader.cur2) or dir.n
	 while i < dir.n do
	    i = i + 1
	    local entry = dir[i]
	    if entry and entry.size < 0 and
	       entry.file ~= "." and entry.file ~= ".."
	    then
	       local subel = entrylist_new()
	       if subel and entrylist_load(subel,
					   canonical(dir.path .. "/"
						     .. entry.file),
					   filter)
	       then
		  sb.recloader.cur2 = i
		  sb.recloader = {
		     parent = sb.recloader,
		     dir = subel,
		     cur = 0,
		     cur2 = 0,
		     el_filter = filter,
		  }
		  return 1
	       end
	    end
	 end
	 if i >= dir.n then
	    sb.recloader = sb.recloader.parent
	    sb.pl:draw()
	 end
      else
	 while i < dir.n do
	    i = i + 1
	    local typ,major,minor = filetype(dir[i].file,dir[i].size)
	    -- Only enqueue rnnable types
	    if type(sb.pl.actions.run[major]) ==
	       "function" then
	       sbpl_insert(sb, dir[i],nil,1)
	       sb.recloader.cur = i
	       return 1
	    end
	 end
	 sb.recloader.cur = i
      end
   end
end


--- Update the playlist running.
--- @internal
--
function song_browser_update_playlist(sb, frametime)

   if sb.stopping and playa_fade() == 0 then
      playa_stop()
      sb.stopping = nil
      sb.playlist_idx = nil
   elseif sb.playlist_start_pos then
      -- $$$$
      print("playlist starting:")
      print("@:",sb.playlist_start_pos)
      print("LOOP:",sb.playlist_loop_pos)
      playa_stop(1) -- Don't want any music at start
      sb.playlist_idx = sb.playlist_start_pos
      sb.playlist_start_pos = nil
      sb.playlist_wait = nil
   end

   if not sb.playlist_idx then
      return
   end
   
   -- Get next entry
   local nxt = sb.pl:get_pos(sb.playlist_idx + 1)
      or (sb.playlist_loop_pos and sb.pl:get_pos(sb.playlist_loop_pos))

   local wait = sb.playlist_wait and sb.playlist_wait(sb.pl, sb, frametime)

   --   while (Infinite loop and best display !)
   if not wait then
      sb.playlist_idx = nxt
      if nxt then
	 nxt = sb.pl:get_pos(nxt + 1)
	    or (sb.playlist_loop_pos and sb.pl:get_pos(sb.playlist_loop_pos))
	 
	 print ("PLAYLIST Run next entry : ",sb.playlist_idx,nxt)
	 
	 sb.playlist_wait = nil -- Clear wait method
	 
	 local path = sb.pl:fullpath(sb.pl.dir[sb.playlist_idx])
	 print(path)
	 if path then
	    printf("path=%s",path)
	    local ty, major, minor = filetype(path)
	    print(ty, major, minor)
	    local f = sb.pl.actions.run[major]
	    if type(f) == "function" then
	       f(sb.pl, sb, path, nxt)
	    else
	       printf("skipping [%s] : no run action [%s]",path,major)
	    end
	 end
	 sb.pl:draw()
      end
   end
end

--- song-browser update handler.
---
---   This is the main update handler.
---   It performs several update functions:
---    - updates filelist directory loading
---    - updates song-browser recursive directory loading
---    - handles fall asleep.
---    - handles key rate for keyboard browsing
---    - handles music fade / stop
---    - runs playlist
---    - call update for both filelist and playlist textlist.
---
--- @internal
-- 
function song_browser_update(sb, frametime)
   -- Run various update
   song_browser_update_loaddir(sb, frametime)
   song_browser_update_recloader(sb, frametime)
   song_browser_update_playlist(sb, frametime)

   if not sb.sleeping then
      sb.idle_time = sb.idle_time or 0
      if sb.idle_time > 20 then
	 sb:asleep()
      end
      sb.idle_time = sb.idle_time + frametime
   end

   -- Key browsing
   if sb.key_time then
      sb.key_time = sb.key_time + frametime
      if sb.key_time >= 1 then
	 sb.search_str = nil
	 sb.key_time = nil
      end
   end

   -- Update filelsit and playlist texlists.
   sb.fl:update(frametime)
   sb.pl:update(frametime)
end

--- Song-Browser handle.
--  -------------------
function song_browser_handle(sb, evt)
   -- call the standard dialog handle (manage child autoplacement)
   evt = gui_dialog_basic_handle(sb, evt)
   if not evt then
      return
   end

   local key = evt.key
   if key == evt_shutdown_event then
      sb:shutdown()
      return evt
   elseif key == ioctrl_cdrom_event then
      song_browser_update_cdrom(sb, evt)
      return
   elseif key == gui_menu_close_event then
      --	 printf("%q gui_menu_close_event",sb.name)
      song_browser_contextmenu(sb) -- shutdown menu
      return
   end

   if sb.closed then
      return evt
   end

   local action

   -- $$$ Test 96 '`' to prevent event eating for the console switching.
   if key >= 32 and key<128 and key ~= 96 then
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
--
function song_browser_asleep(sb)
   sb.sleeping = 1
   if not sb.closed then
      sb.cl.fade_min = 0.3
      sb.cl:close()
   end
end

--- Song-Browser awake.
--
function song_browser_awake(sb)
   sb.sleeping = nil
   sb.idle_time = 0
   if not sb.closed then
      sb.cl:open()
   end
end

--- Song-Browser open.
--
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
--
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
--- @internal
--
function song_browser_shutdown(sb)
   if not sb then return end
   sb.fl:shutdown()
   sb.pl:shutdown()
   dl_set_active(sb.dl, nil)
   sb.dl = nil;
   vmu_set_text("dcplaya")
end

--- Song-Browser draw.
--- @internal
--
function song_browser_draw(sb)
   dl_clear(sb.dl)
   sb.fl:draw()
   sb.pl:draw()
   dl_sublist(sb.dl,sb.fl.dl)
   dl_sublist(sb.dl,sb.pl.dl)
   dl_set_active(sb.dl, 1)
end

function song_browser_list_draw_background(fl, dl)
   local sb = fl.owner
   if not sb then return end
   local b3d = sb.fl_box;
   if not b3d then return end
   b3d:draw(dl, mat_trans(0, 0, 50))
end

--- Song-Browser set color.
--- @internal
--
function song_browser_set_color(sb, a, r, g, b)
   sb.fl:set_color(a,r,g,b)
   sb.pl:set_color(a,r,g,b)
end

--- Song-Browser confirm.
--- @internal
--
function song_browser_confirm(sb)
   return sb.cl:confirm(sb)
end

--- Song-Browser cancel.
--- @internal
--
function song_browser_cancel(sb)
   return sb.cl:cancel(sb)
end

--- Song-Browser select.
--- @internal
--
function song_browser_select(sb)
   return sb.cl:select(sb)
end

--- Songbrowser contextual menu create.
--- @internal
function song_browser_contextmenu(sb, name, fl, def, entry_path)
   if sb.menu then
      evt_shutdown_app(sb.menu)
   end
   local menudef = menu_create_defs(def, sb)
   if not menudef then return end

   fl = fl or sb.cl
   local x,y = textlist_screen_coor(fl)
   sb.menu = menu_create(sb, name, menudef, {x,y,0})
   if tag(sb.menu) == menu_tag then
      sb.menu.target = sb
      sb.menu.target_pos = fl.pos + 1
      sb.menu.__entry_path = entry_path
   end
end

--- Stop current music and playlist running.
--- @param  sb   song-browser application (default to song_browser)
function song_browser_stop(sb)
   sb = sb or song_browser
   if not sb then return end
   sb.stopping = 1
   playa_fade(-1)
   return song_browser_playlist_stop(sb)
end

--- Start a new music.
--- @param  sb   song-browser application (default to song_browser)
--- @param  filename  filename of music to play
--- @param  track-number 0:file default
--- @param  immediat  flush sound buffer if setted (some sound will be lost)
--          Do not set it to play chain tracks properly.
function song_browser_play(sb, filename, track, immediat)
   sb = sb or song_browser
   if not sb then return end
   sb.stopping = nil
   local r = playa_play(filename, track, immediat)
   playa_fade(2)
   return r
end

--- Run the playlist.
--- @param  sb    song-browser application (default to song_browser)
--- @param  pos   running position  (default to cursor)
--- @param  loop  loop at this position if setted 0:keep current loop
function song_browser_run(sb,pos,loop)
   sb = sb or song_browser
   if not sb then return end
   -- $$$$
   print("song_browser_run",pos,loop)
   local pl = sb.pl
   pos = pl:get_pos(type(pos) == "number" and pos)
   if not pos then return end
   pos = pos - 1
   if pl.dir and pl.dir.n and pos < pl.dir.n then
      sb.playlist_start_pos = pos
      if type(loop) == "number" and loop ~= 0 then
	 sb.playlist_loop_pos = loop
      end
      print("song_browser_run : ",sb.playlist_start_pos,sb.playlist_loop_pos)
      sb.stopping = nil
      pl:draw()
      return 1
   end
end

--- Load directory into filelist.
--- @param  sb      song-browser application (default to song_browser)
--- @param  path    path of directory to load.
--- @param  locate  name of entry to locate in this directory (optionnal)
--- @return error-code
--- @retval 1 success
--- @warning The fonction use a diffrent thread to load  and returns before
---          the directory is really loaded. Only one directory is available
---          and the function will fail if another loding is in progress.
---          To know if you can start a loadir or if your loading is done
---          check the loaded_dir (entrylist) member.
--
function song_browser_loaddir(sb, path, locate)
   sb = sb or song_browser
   if not test("-d",path) or sb.loaded_dir then
      return
   end
   sb.loaded_dir = entrylist_new()
   if sb.loaded_dir then
      if entrylist_load(sb.loaded_dir, path, sb.el_filter) then
	 sb.locatedir = locate
	 return 1
      end
   end
   sb.locatedir = nil
   sb.loaded_dir = nil
end


--- Clear playlist.
---
--- @param  sb      song-browser application (default to song_browser)
---
--- @return error-code
--- @retval 1 success
--
function song_browser_playlist_clear(sb)
   sb = sb or song_browser
   if not sb then return end
   sbpl_clear(sb)
end

--- Stop playlist running.
---
--- @param  sb      song-browser application (default to song_browser)
---
--- @return error-code
--- @retval 1 success
--
function song_browser_playlist_stop(sb)
   sb = sb or song_browser
   if not sb then return end
   sb.playlist_idx = nil
   sb.playlist_start_pos = nil
   sb.pl:draw()
   return 1
end

--- Load/Insert/Append playlist and run it
---
--- @param  sb      song-browser application (default to song_browser)
--- @param  path    path of playlist
--- @param  insert  insert postion : nil:erase before, 0:cursor pos
---                  <0:insert position from end of list
--- @param  run     run at given position 0:insertion point
---
--- @return error-code
--- @retval 1 success
--
function song_browser_playlist(sb, path, insert, run)
   sb = sb or song_browser
   if not sb then return end
   local dir = entry_path and playlist_load(entry_path)
   local odir = type(insert) == "number" and sb.pl.dir
   local insert_point

   if not odir then
      -- No old dir : simply load the playlist
      if dir then
	 if sb.pl:change_dir(dir) then
	    insert_point = 1
	 end
      else
	 -- No dir, if no entry_path, it is not an error
	 insert_point = entry_path == nil
      end
   else
      -- There is an insert point and a old dir !
      if insert == 0 then
	 -- After sursor insertion.
	 insert_point = sb.pl:get_pos()
	 if insert_point then insert_point = insert_point + 1 end
      elseif insert > 0 then
	 -- Given position insertion.
	 insert_point = sb.pl:get_pos(insert)
	 insert_point = insert_point == insert and insert_point
      else -- <0
	 -- Given position rom the end
	 insert_point = sb.pl.dir and sb.pl.dir.n
	 if insert_point then
	    insert_point = insert_point + insert + 2
	    if insert_point < 0 then insert_point = nil end
	 end
      end

      if insert_point then
	 for i,v in dir do
	    if type(v) == "table" then
	       tinsert(odir, pos, v)
	       pos = pos+1
	    end
	    if not sb.pl:change_dir(odir) then
	       insert_point = nil
	    end
	 end
      end
   end

   if type(run) == "number" then
      run = (run == 0 and insert_point) or run
      if run then
	 sb:run(run,0)
      end
   end
   return insert_point ~= nil
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

--- Load an image on the background
---
--- @param  sb      song-browser application (default to song_browser)
--- @param  path    path of image to load
--- @param  mode    background mode ("center", "scale", "tile"). Defaulted
---                 to sb.background_mode or load_background() default.
---
--- @return error-code
--- @retval 1 success
--
function songbrowser_load_image(sb, filename, mode)
   sb = sb or song_browser
   if not sb then return end
   if not background_tag or tag(background) ~= background_tag then
      gui_ask('The background library is not available, you will not be able to display background images.<br><img name="grimley" src="stock_grimley.tga" scale="1.5">',"close",256,"background error")
      return
   end
   if not (test("-f", filename) and
	   background:set_texture(filename,mode or sb.background_mode))
   then
      printf ("[song_browser] : loading background %q failed", filename)
      return
   end
   return 1
end

function songbrowser_info_image(sb, filename)
   sb = sb or song_browser
   if not sb then return end
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

--- View a file.
---
function song_browser_view_file(sb, entry_path)
   sb = sb or song_browser
   if not sb then return end
   if type(entry_path) ~= "string" then return end
   if not test("-f",entry_path) then return end
   local type,major,minor = filetype(entry_path)

   local path,leaf = get_path_and_leaf(entry_path)
   local type,major,minor = filetype(entry_path)
   local width,preformatted,view = 550,4,1

   if major == "lua" then
      if sb.no_lua_colorize then
	 gui_text_viewer(nil, { [leaf] = luacolor_file(entry_path) },
			 width, leaf)
	 return
      end
      preformatted = 8
   elseif major == "playlist" then
      preformatted = 1
   elseif major == "text" or major == "lua" then
      if minor ~= "zml" then
	 -- try to guess if zml
	 local fh = openfile(entry_path,"rt")
	 if not fh then return end
	 local line = read(fh)
	 closefile(fh)
	 if not line then return end
	 if strfind(line,"%s*<zml>.*") then
	    minor = "zml"
	 end
      end
      if minor == "zml" then
	 width = 450
	 preformatted = nil
      end
   else
      view = nil
   end
   
   if view then
      gui_file_viewer(nil, entry_path, width, leaf, nil, preformatted)
   end
end

--- Edit a file.
---
function song_browser_edit_file(sb, entry_path)
   sb = sb or song_browser
   if not sb then return end
   if type(entry_path) ~= "string" then return end
   if console_app then
      evt_app_insert_first(console_app.owner, console_app)
   end
   doshellcommand(format("zed(%q)",entry_path))
   if console_app then
      evt_app_insert_last(console_app.owner, console_app)
   end
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
   local pos = fl:get_pos()
   if not pos then return end
   local entry=fl.dir[pos]

   -- $$$$
   dump(entry,action)

   local entry_path = fl:fullpath(entry)
   if not entry_path then return end
   local ftype, major, minor = filetype(entry_path, entry.size)

   -- $$$
   printf("%q on %q [%q,%q]", action, entry_path, major, minor)

   local func = fl.actions[action][major] or fl.actions[action].default
   if type(func) == "function" then
      return func(fl,sb,action,entry_path,entry)
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
   return songbrowser_any_action(sb, "confirm", fl)
end

function sbfl_confirm_dir(fl, sb, action, entry_path, entry)
   if not test("-d", entry_path) then return end
   if not entry then
      local idx = fl:get_pos();
      entry = idx and fl.dir[idx];
   end
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

function sbfl_confirm_music(fl, sb, action, entry_path, entry)
   -- $$$ problem with cdda ... 
   --      if not test("-f",entry_path) then return end
   sb.playlist_idx = nil
   song_browser_play(sb, entry_path, 0, 1)
   return 1
end

function sbfl_confirm_playlist(fl, sb, action, entry_path, entry)
   local dir = playlist_load(entry_path)
   if not dir then return end
   sb.playlist_idx = nil
   sb.pl:change_dir(dir)
   return 1
end

function sbfl_confirm_image(fl, sb, action, entry_path, entry)
   song_browser_ask_background_load(sb)
   sb:load_image(entry_path)
end

function sbfl_confirm_plugin(fl, sb, action, entry_path, entry)
   --       if not test("-f",entry_path) then return end
   --       return driver_load(entry_path)
end

function sbfl_confirm_lua(fl, sb, action, entry_path, entry)
   --       if not test("-f",entry_path) then return end
   --       return dofile(entry_path)
end

function sbfl_confirm_text(fl, sb, action, entry_path, entry)
   if not test("-f",entry_path) then return end
   song_browser_view_file(sb,entry_path)
end

-- ----------------------------------------------------------------------
-- filelist "select" actions
-- ----------------------------------------------------------------------

function sbfl_select(fl, sb, action, entry_path, entry)
   songbrowser_any_action(sb,"select",fl)
end

function sbfl_select_dir(fl, sb, action, entry_path, entry)

   print("sbfl_select_dir:"..entry_path)

   if not test("-d", entry_path) then return end

   print("sbfl_select_dir is dir")

   local path,leaf = get_path_and_leaf(entry_path)

   print("sbfl_select_dir:",path,leaf)

   if not leaf then return end
   
   local def = {
      root = ":"
	 .. leaf
	 .. ":enter{enter},load{load},enqueue{enqueue},append{append},"
	 .. menu_yesno_menu(sb.recursive_mode,"recursive",'recurse'),
      cb = {
	 enter = function (menu, idx)
		    local root_menu = menu.root_menu
		    local sb = root_menu.target
		    sbfl_confirm_dir(sb.fl, sb, "confirm",
				     root_menu.__entry_path)
		 end,
	 load =  function (menu, idx)
		    local root_menu = menu.root_menu
		    local sb = menu.root_menu.target
		    local entry_path = root_menu.__entry_path or ""
		    sbpl_clear(sb)
		    sbpl_insertdir(sb, entry_path)
		 end,

	 append =  function (menu, idx)
		    local root_menu = menu.root_menu
		    local sb = menu.root_menu.target
		    local entry_path = root_menu.__entry_path or ""
		    sbpl_insertdir(sb, entry_path)
		 end,

	 recurse = function (menu, idx)
		      local sb = menu.root_menu.target
		      sb.recursive_mode = not sb.recursive_mode
		      menu_yesno_image(menu, idx, sb.recursive_mode,
				       'recursive')
		   end,
      },
   }
   song_browser_contextmenu(sb, "directory-menu,", fl, def, entry_path)
end

function sbfl_select_music(fl, sb, action, entry_path, entry)
   -- $$$ problem with cdda ... 
   -- if not test("-f",entry_path) then return end
   if not entry then
      local pos = fl:get_pos()
      entry = pos and fl.dir[pos]
   end
   if entry then
      return sbpl_insert(sb, entry)
   end
end

function sbfl_select_playlist(fl, sb, action, entry_path, entry)
   local path,leaf = get_path_and_leaf(entry_path)
   if not leaf then return end
   
   local def = {
      root = ":" .. leaf .. ":load{load},insert{load},append{load},{menu_textviewer}view{view}",
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
	 view =
	    function (menu, idx)
	       local root_menu = menu.root_menu
	       song_browser_view_file(root_menu.target,
				      root_menu.__entry_path)
	    end,

      },
   }
   song_browser_contextmenu(sb,"playlist-menu,",fl,def, entry_path)
end

function sbfl_select_image(fl, sb, action, entry_path, entry)
   local path,leaf = get_path_and_leaf(entry_path)
   if not leaf then return end

   if tag(background) == background_tag then
      local spr = sprite("menu_bkg", 0, 0, 32, 24) -- Create pipo sprite
      if spr then
	 spr.vtx = background.vtx * mat_scale(32,24,1); -- Hack vertex
	 spr.tex = background.tex;
      end
   end

   local def = {
      root = ":" .. leaf
	 .. ":{menu_info}info{info},{menu_bkg}background>"
      ,
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
		  sb:load_image(entry_path,mode)
	       end,
      },
      sub = {
	 background = ":background:scale{bkg},center{bkg},tile{bkg}"
      }
   }

   song_browser_contextmenu(sb, "image-menu", fl, def, entry_path)
end

function sbfl_select_plugin(fl, sb, action, entry_path, entry)
   if not test("-f",entry_path) then return end
   local path,leaf = get_path_and_leaf(entry_path)
   if not leaf then return end

   local def = {
      root = ":"..leaf..":load plugin{load}",
      cb = {
	 load = function (menu, idx)
		   local root_menu = menu.root_menu
		   local sb = root_menu.target
		   local entry_path = root_menu.__entry_path
		   local text,label,color
		   if not entry_path then return end
		   if not driver_load(entry_path) then
		      label = "error"
		      color = "#FF0000"
		      text = '<left>Unable to load the driver file'
		   else
		      label = "success"
		      color = "#00FF00"
		      text = '<left>Successfully load driver file'
		   end
		   text = text ..
		      '<p><vspace h="4"><center><font color=%q size="12">%s'
		   gui_ask(format(text,color,entry_path), "<center>close",
			   nil,
			   format("Plugin load %s", label))
		end,
      },
   }
   song_browser_contextmenu(sb,"plugin-menu,", fl, def, entry_path)
end

function sbfl_select_lua(fl, sb, action, entry_path, entry)
   if not test("-f",entry_path) then return end
   local path,leaf = get_path_and_leaf(entry_path)
   if not leaf then return end
   local def = {
      root = ":"..leaf..":execute{exe},load library{loadlib},{menu_textviewer}view{view},{menu_textedit}edit{edit}",
      cb = {
	 exe =
	    function (menu, idx)
	       local root_menu = menu.root_menu
	       local sb = root_menu.target
	       local entry_path = root_menu.__entry_path
	       if not entry_path then return end
	       dofile(entry_path)
	    end,
	 loadlib =
	    function (menu, idx)
	       local root_menu = menu.root_menu
	       local sb = root_menu.target
	       local entry_path = root_menu.__entry_path
	       local text,label,color
	       if not entry_path then return end
	       local path,leaf = get_path_and_leaf(entry_path);
	       local result = dolib(get_nude_name(leaf),1,path)
	       if not result then
		  label = "error"
		  color = "#FF0000"
		  text = '<left>Unable to load lua library'
	       else
		  printf("result:%d",result)
		  label = "success"
		  color = "#00FF00"
		  text = '<left>Successfully load lua library'

	       end
	       text = text ..
		  '<p><vspace h="4"><center><font color=%q size="12">%s'
	       gui_ask(format(text,color,entry_path), "<center>close",
		       nil,
		       format("LUA lib load %s", label))
	    end,
	 view =
	    function (menu, idx)
	       local root_menu = menu.root_menu
	       song_browser_view_file(root_menu.target,
				      root_menu.__entry_path)
	    end,
	 edit =
	    function (menu, idx)
	       local root_menu = menu.root_menu
	       song_browser_edit_file(root_menu.target,
				      root_menu.__entry_path)
	    end,
      },
   }
   song_browser_contextmenu(sb,"lua-menu,", fl, def, entry_path)
end


function sbfl_select_text(fl, sb, action, entry_path, entry)
   if not test("-f",entry_path) then return end
   local path,leaf = get_path_and_leaf(entry_path)
   if not leaf then return end
   local def = {
      root = ":"..leaf..":{menu_textviewer}view{view},{menu_textedit}edit{edit}",
      cb = {
	 view =
	    function (menu, idx)
	       local root_menu = menu.root_menu
	       song_browser_view_file(root_menu.target,
				      root_menu.__entry_path)
	    end,
	 edit =
	    function (menu, idx)
	       local root_menu = menu.root_menu
	       song_browser_edit_file(root_menu.target,
				      root_menu.__entry_path)
	    end,
      },
   }
   song_browser_contextmenu(sb,"text-menu,", fl, def, entry_path)
end

-- ----------------------------------------------------------------------
-- filelist "cancel" actions
-- ----------------------------------------------------------------------
function sbfl_cancel(fl, sb, action, entry_path, entry)
   songbrowser_any_action(sb,"cancel",fl)
end

function sbfl_cancel_default(fl, sb, action, entry_path, entry)
   if playa_play() == 1 then
      sb:stop();
   end
end

function sbfl_view_lua(fl, sb, action, entry_path, entry)
end

function sbfl_view_text(fl, sb, action, entry_path, entry)
end

function sbfl_edit_file(fl, sb, action, entry_path, entry)
end

function sbpl_clear(sb)
   sb.pl:change_dir(nil)
end

function sbpl_insert(sb, entry, pos, no_redraw)
   sb.pl:insert_entry(entry, pos, no_redraw)
end

function sbpl_insertdir(sb, path)
   if sb.recloader then return end
   local dir = entrylist_new()
   if dir then
      sb.recloader = {
	 dir = dir,
	 cur = 0,
	 cur2 = 0,
	--  el_filter = "DMI"  -- Default defined in update_recloader
      }
      if not entrylist_load(dir,path,sb.el_filter) then
	 sb.recloader = nil
      end
   end
end

-- Stop the playlist.
-- @param  sb  song-browser application
-- @internal
function sbpl_stop(sb)
   sb:stop()
end

-- Run the playlist.
-- @param  sb   song-browser application (default to song_browser)
-- @param  pos  running position (default to cursor)
-- @param  loop loop-point : nil=none 0:current
-- @internal
function sbpl_run(sb, pos, loop)
   sb:run(pos, 0)
end

function sbpl_remove(sb,pos)
   pos = sb.pl:get_pos(pos)
   if pos then
      if sb.playlist_idx and pos <= sb.playlist_idx then
	 sb.playlist_idx = sb.playlist_idx - 1
      end
      if sb.playlist_loop_pos and pos <= sb.playlist_loop_pos then
	 sb.playlist_loop_pos = sb.playlist_loop_pos - 1
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
      xsort(dir,cmp,sb)
   elseif type(cmp) == "table" then
      xsort(dir, sbpl_cmp_any, cmp)
   else
      print("[song_browser] : invalid sort parameter")
      return
   end
   
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
   sbpl_run(sb,nil,0)
end

function sbpl_cancel(pl, sb)
   sbpl_remove(sb)
end


function sbpl_open_menu(pl, sb)
   local idx = pl:get_pos()

   --- $$$$
   print("sbpl_open_menu idx, loop",idx, sb.playlist_loop_pos)

   local entry = pl:get_entry()
   if not entry then return end
   local root = ""
   local fname = entry.file or entry.name
   if type(fname) == "string" then root = ":"..fname..":" end
   root = root.."run{run},remove{remove},shuffle{shuffle},sort>sort,clear{clear},save{save}," .. menu_yesno_menu(sb.playlist_loop_pos and sb.playlist_loop_pos == idx, 'loop point','loop')

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
	       sbpl_run(sb, nil, 0)
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

      loop = function (menu, idx)
		local sb = menu.root_menu.target
		local r
		local pos = sb.pl:get_pos()
		if sb.playlist_loop_pos and sb.playlist_loop_pos == pos then
		   sb.playlist_loop_pos = nil
		   print("unset loop at "..pos)
		else
		   -- $$$$
		   print("set loop at "..pos)
		   sb.playlist_loop_pos = pos
		   r = 1
		end
		menu_yesno_image(menu, idx, r,'loop point')
		sb.pl:draw()
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

   song_browser_contextmenu(sb, "playlist-menu", pl, def, entry_path)
end

function sbpl_select(pl, sb)
   pl:open_menu(sb)
end

function songbrowser_menucreator(target)
   local sb = target;
   local cb = {
      toggle = function(menu, idx)
		  local sb = menu.target
		  if sb.closed then
		     sb:open()
		  else
		     sb:close()
		  end
		  menu.fl.dir[idx].name = (sb.closed and "open") or "close"
		  menu:draw()
	       end,
      saveplaylist = function(menu, idx)
			local sb = menu.root_menu.target
			sbpl_save(sb,menu)
		     end,
   }
   local root = ":" .. target.name .. ":" .. 
      ((sb.closed and "open") or "close") ..
      "{toggle},playlist >playlist"
   local def = {
      root=root,
      cb = cb,
      sub = {
	 playlist = ":playlist:save{saveplaylist}",
      }
   }
   return menu_create_defs(def , target)
end

--
--- @name song-browser initialization functions.
--- @ingroup dcplaya_lua_sb_app
--- @{
--

--
--- Create all icon sprites. 
--- @internal
--
function song_browser_create_sprites(sb)
   if sb then
      song_browser_create_dcpsprite(sb)
   end

   -- filetypes
   menu_create_sprite("ft_dir", "folder.tga",32)
   menu_create_sprite("ft_file", "type_file.tga",32)
   menu_create_sprite("ft_cdda", "type_cdda.tga",32)
   menu_create_sprite("floppy", "floppy2.tga", 32)
   menu_create_sprite("info", "info.tga", 32)
   menu_create_sprite("textviewer", "textviewer.tga", 32)
   menu_create_sprite("yes", "stock_button_apply.tga", 20)
   menu_create_sprite("no", "stock_button_cancel.tga", 20)
end

--- Creates some sprites.
--- @internal
function song_browser_create_dcpsprite(sb)
   sb.sprites = {}
   sb.sprites.texid = tex_exist("dcpsprites") or tex_new("/rd/dcpsprites.tga")

   local x1,y1,w,h

   x1,y1,w,h = 0,0,408,29
   sb.sprites.logo = sprite(nil,
			    w/2, h/2,
			    w, h,
			    x1/512, y1/128, (x1+w)/512, (y1+h)/128,
			    sb.sprites.texid)

   x1,y1,w,h = 1,31,165,14
   sb.sprites.file = sprite(nil,
			    w/2, h/2,
			    w, h,
			    x1/512, y1/128, (x1+w)/512, (y1+h)/128,
			    sb.sprites.texid)

   x1,y1,w,h = 170,31,249,14
   sb.sprites.list = sprite(nil,
			    w/2, h/2,
			    w, h,
			    x1/512, y1/128, (x1+w)/512, (y1+h)/128,
			    sb.sprites.texid)

   x1,y1,w,h = 1,46,184,19
   sb.sprites.copy = sprite(nil,
			    w/2, h/2,
			    w, h,
			    x1/512, y1/128, (x1+w)/512, (y1+h)/128,
			    sb.sprites.texid)

   x1,y1,w,h = 187,52,186,12
   sb.sprites.url = sprite(nil,
			   w/2, h/2,
			   w, h,
			   x1/512, y1/128, (x1+w)/512, (y1+h)/128,
			   sb.sprites.texid)

   x1,y1,w,h = 1,71,107,55
   sb.sprites.jess = sprite(nil,
			    w/2, h/2,
			    w, h,
			    x1/512, y1/128, (x1+w)/512, (y1+h)/128,
			    sb.sprites.texid)

   x1,y1,w,h = 454,0,58,81
   sb.sprites.proz = sprite(nil,
			    w/2, h/2,
			    w, h,
			    x1/512, y1/128, (x1+w)/512, (y1+h)/128,
			    sb.sprites.texid)

   --    x1,y1,w,h = 109,65,104,63
   --    sb.sprites.vmu = sprite(nil,
   -- 			   w/2, h/2,
   -- 			   w, h,
   -- 			   x1/512, y1/128, (x1+w)/512, (y1+h)/128,
   -- 			   sb.sprites.texid,1)

   sb.fl.title_sprite = sb.sprites.file
   sb.fl.icon_sprite = sb.sprites.jess
   sb.pl.title_sprite = sb.sprites.list
   sb.pl.icon_sprite = sb.sprites.proz

end

--- Creates box3d.
--- 
--- @internal
function song_browser_create_box(sb, box, style)
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
   tcol = { mt * borcol[1], mt * borcol[2], mt * borcol[3], mt * borcol[4] }
   lcol = { ml * borcol[1], ml * borcol[3], ml * borcol[7], ml * borcol[5] }
   bcol = { mb * borcol[5], mb * borcol[6], mb * borcol[7], mb * borcol[8] }
   rcol = { mr * borcol[4], mr * borcol[2], mr * borcol[6], mr * borcol[8] }
   sb.fl_box = box3d(box, -4, fcol, tcol, lcol, bcol, rcol)
end


--- Create a song-browser application.
---
--- @param  owner  Owner application (default to  desktop).
--- @param  name   Name of application (default to "song browser").
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
      bkg_color         = { 0.8, 0.4, 0.0, 0.0,  0.8, 0.3, 0.3, 0.3 },
      border            = 5,
      span              = 1,
      file_color        = { 1, 1, 0.8, 0 },
      dir_color         = { 1, 1, 1, 0 },
      cur_color         = { 1, 0.5, 0, 0,  0.5, 1, 0.7, 0 },
      text              = {font=0, size=16, aspect=1}
   }

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

      -- Music and file control methods
      play = song_browser_play,
      stop = song_browser_stop,
      run = song_browser_run,
      loaddir = song_browser_loaddir,

      -- Extra file action
      load_image = songbrowser_load_image,
      info_image = songbrowser_info_image,
      view_file =song_browser_view_file,
      edit_file =song_browser_edit_file,
      
      -- Members
      style = style,
      z = gui_guess_z(owner,z),
      fade = 0,
      dl = dl_new_list(128, 0, 0)
   }

   local x,y,z
   local box = { 0, 0, 256, 210 }
   local minmax = { box[3], box[4], box[3], box[4] }

   x = 42
   y = 110
   z = sb.z - 2



   sb.fl = textlist_create(
			   {
			      name = "filelist",
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

   sb.fl.curplay_color = color_new(1,0,1,0)
   sb.fl.curloop_color = color_new(1,1,0,0)


   x = 341
   sb.pl = textlist_create(
			   {
			      name = "playlist",
			      pos = {x, y, 0},
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
   sb.pl.curloop_color = sb.fl.curloop_color

   sb.pl.draw_entry = function (fl, dl, idx, x , y, z)
			 local sb = fl.cookie
			 local entry = fl.dir[idx]
			 local color = fl.dircolor
			 local colora,colorb
			 if sb.playlist_idx
			    and idx == sb.playlist_idx then
			    colora = fl.curplay_color
			 end
			 if sb.playlist_loop_pos
			    and idx == sb.playlist_loop_pos then
			    colorb = fl.curloop_color
			 end
			 if colora and colorb then
			    dl_draw_text(dl,
					 x-1, y-1, z,
					 colorb[1],colorb[2],
					 colorb[3],colorb[4],
					 entry.name)
			    dl_draw_text(dl,
					 x+1, y+1, z+10,
					 0.6*colora[1],colora[2],
					 colora[3],colora[4],
					 entry.name)
			    --			    dl_text_prop(dl,nil,14)
			    --			    dl_text_prop(dl,nil,16)
			 else
			    color = colora or colorb or color
			    dl_draw_text(dl,
					 x, y, z,
					 color[1],color[2],
					 color[3],color[4],
					 entry.name)
			 end
		      end


   sb.pl.fade_min = sb.fl.fade_min
   sb.pl.draw_background_old = sb.pl.draw_background
   sb.pl.draw_background = sb.fl.draw_background

   -- Defining actions

   song_browser_filelist_actions = {
      confirm = {
	 dir = sbfl_confirm_dir,
	 music = sbfl_confirm_music,
	 image = sbfl_confirm_image,
	 playlist = sbfl_confirm_playlist,
	 lua = sbfl_select_lua, -- select on purpose !
	 plugin = sbfl_select_plugin, -- select on purpose !
	 text = sbfl_confirm_text,
	 default = sbfl_confirm_default,
      },
      select = {
	 dir = sbfl_select_dir,
	 music = sbfl_select_music,
	 image = sbfl_select_image,
	 playlist = sbfl_select_playlist,
	 lua = sbfl_select_lua,
	 plugin = sbfl_select_plugin,
	 text = sbfl_select_text,
	 default = sbfl_select_default,
      },
      cancel = {
	 default = sbfl_cancel_default,
      },
      view = {
	 image = sbfl_confirm_image,
	 lua = sbfl_view_lua,
	 text = sbfl_view_text,
	 playlist = sbfl_view_text,
      },
      edit = {
	 lua = sbfl_edit_file,
	 text = sbfl_edit_file,
	 playlist = sbfl_edit_file,
      },
      info = {
	 image = sbfl_info_image,
	 plugin = sbfl_info_plugin,
	 plugin = sbfl_info_music,
	 default = sbfl_info_file,
      },
   }

   song_browser_playlist_actions = {
      run = {
	 music = function (fl, sb, path, nxt)
		    -- $$$
		    printf("song_browser_playlist_actions : music\n, [%s]",path)
		    if playa_play(path) then
		       print("-->Success")
		       sb.playlist_wait = fl.actions.wait.music
		       return 1
		    end
		    print("-->Failed")
		 end,
	 image = function (fl, sb, path, nxt)

		    -- Determine if there is an image after this one
		    -- in that case, wait for sb.slide_show_speed
		    -- seconds
		    sb.slide_show_timeout = nil
		    if nxt then
		       local nxt_path = fl:fullpath(fl.dir[nxt])
		       local ty,ma,mi = filetype(nxt_path)
		       if ma == "image" then
			  sb.slide_show_timeout = sb.slide_show_speed or 10
			  -- $$$
			  print("Slide show mode",sb.slide_show_timeout)
		       end
		    end
		    print("song_browser_playlist_actions : image")
		    if sb:load_image(path) then
		       print("-->Success")
		       sb.playlist_wait = fl.actions.wait.image
		       return 1
		    end
		    print("-->Failed")
		    sb.slide_show_timeout = nil
		 end,
      },
      wait = {
	 music = function (fl, sb, elapsed)
		    return playa_play() ~= 0
		 end,
	 image = function (fl, sb, elapsed)
		    if sb.slide_show_timeout then
		       sb.slide_show_timeout =
			  sb.slide_show_timeout - elapsed
		       if sb.slide_show_timeout <= 0 then
			  sb.slide_show_timeout = nil
		       end
		    end
		    return sb.slide_show_timeout
		 end,
      },
   }


   sb.fl.actions = song_browser_filelist_actions
   sb.pl.actions = song_browser_playlist_actions
   

   sb.cl = sb.fl

   song_browser_create_sprites(sb)
   song_browser_loaddir(sb,"/","cd")
   --    sb.fl:change_dir(sb.fl.dir)

   -- cdrom
   sb.cdrom_stat, sb.cdrom_type, sb.cdrom_id = cdrom_status(1)
   --   sb.cdrom_check_timeout = 0

   -- filters
   sb.el_filter = "DXIMPTL"

   -- Menu
   sb.mainmenu_def = songbrowser_menucreator

   song_browser_create_box(sb, box, nil)

   sb:set_color(0, 1, 1, 1)
   sb:draw()
   sb:open()

   evt_app_insert_first(owner, sb)

   return sb
end

--
--- Kill a song-browser application.
---
---   The song_browser_kill() function kills the given application by
---   calling sending the evt_shutdown_app() function. If the given
---   application is nil or song_browser the default song-browser
---   (song_browser) is killed and the global variable song_browser is
---   set to nil.
---
--- @param  sb  application to kill (default to song_browser)
--
function song_browser_kill(sb)
   sb = sb or song_browser
   if sb then
      evt_shutdown_app(sb)
      if sb == song_browser then
	 song_browser = nil
	 print("song-browser shutdowned")
      end
   end
end

if not entrylist_tag and plug_el and test("-f",plug_el) then
   driver_load(plug_el)
end

-- Load texture for application icon
local tex = tex_exist("song-browser")
   or tex_new(home .. "lua/rsc/icons/song-browser.tga")

song_browser_kill()
song_browser = song_browser_create()
if song_browser then
   print("song-browser is running")
   return 1 
end

--- song-browser playlist action table.
---
---    For the playlist actions are not standard. Standard action does
---    the same :
---      - @b confirm  always start the playlist
---      - @b select   opens menu
---      - @b cancel   removes entru
---
---    But playlist use special actions :
---      - @b run  when the entry start running
---      - @b wait to check if entry has finish its run session.
---
---    Major-types for these special actions are :
---      - @b music
---      - @b image
---
--- @warning special actions used non standard action parameters !!!
---
--: function song_browser_playlist_actions[run][major-type];

--- song-browser filelist actions table.
---
---  Standard actions are:
---   - @b confirm (A)
---   - @b select (X)
---   - @b confirm (B)
---
---  Extra actions are
---   - @b view
---   - @b edit
---   - @b info
---
---  Major-types are :
---   - @b dir
---   - @b music
---   - @b image
---   - @b playlist
---   - @b lua
---   - @b plugin
---   - @b text
---   - @b default
--: function song_browser_filelist_actions[action][major-type];;

--
--- @}
--

-- end of defgroup
--
--- @}
--
