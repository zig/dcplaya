
song_browser_loaded = nil
if not dolib("textlist") then return end
if not dolib("gui") then return end
if not dolib("sprite") then return end

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

   sb.fl.title_sprite = sb.sprites.file
   sb.fl.icon_sprite = sb.sprites.jess
   sb.pl.title_sprite = sb.sprites.list
   sb.pl.icon_sprite = sb.sprites.proz

end

function song_browser_create(owner, name, box)
	local sb

	if not owner then
		owner = evt_desktop_app
	end
	if not name then name = "song-browser" end

	-- Song-Browser default style
	-- --------------------------
	local style = {
		bkg_color		= { 0.8, 0.7, 0.7, 0.7,  0.8, 0.3, 0.3, 0.3 },
		border			= 12,
		span            = 1,
		file_color		= { 1, 0, 0, 0 },
		dir_color		= { 1, 0, 0, .4 },
		cur_color		= { 1, 1, 1, 1,  1, 0.1, 0.4, 0.5 },
		text            = {font=0, size=16, aspect=1}
	}

	-- Song-Browser update (handles fade in / fade out)
	-- -------------------
	function song_browser_update(sb, frametime)
		local loading = sb.fl.dir.loading
		if loading then
			sb.fl:change_dir(sb.fl.dir)
		 end

		 if sb.stopping and playa_fade() == 0 then
			print("REAL STOP")
			playa_stop()
			sb.stopping = nil
			sb.playlist_idx = nil
		 elseif sb.playlist_idx then
			local n = sb.pl.dir.n
			while playa_play() == 0 and sb.playlist_idx < n do
			   sb.playlist_idx = sb.playlist_idx + 1
			   local path = sb.pl.dir[sb.playlist_idx].path
			   local leaf = sb.pl.dir[sb.playlist_idx].file
			   if leaf then 
				  if path then
					 leaf = path .. "/" .. leaf
				  end
				  path = fullpath(leaf)
				  print("PLAY-LIST ADVANCE: #"..sb.playlist_idx.." "..path)
				  song_browser_play(sb, path,0,0)
			   end
			   
			end
			if sb.playlist_idx > n then
			   print("END OF PLAY-LIST")
			   sb.playlist_idx = nil
			end
			   
		 end

		 sb.fl:update(frametime)
		 sb.pl:update(frametime)
	end

	-- Song-Browser handle
	-- -------------------
	function song_browser_handle(sb, evt)
		local key = evt.key
		if key == evt_shutdown_event then
			sb:shutdown()
			return evt
		end
		if sb.closed then
			return evt
		end

		if gui_keyconfirm[key] then
		   sb:confirm()
		   return
		elseif gui_keycancel[key] then
		   sb:cancel()
		   return
		elseif gui_keyselect[key] then
		   sb:select()
		   return
		elseif key == gui_item_change_event then
			return
		elseif gui_keyup[key] then
			sb:open()
			sb.cl:move_cursor(-1)
			return
		elseif gui_keydown[key] then
			sb:open()
			sb.cl:move_cursor(1)
			return
		elseif gui_keyleft[key] then
			if sb.cl ~= sb.fl then
			   sb.cl = sb.fl
			   sb:open()
-- 			   sb:close(2)
			end
			return
		elseif gui_keyright[key] then
			if sb.cl ~= sb.pl then
			   sb.cl = sb.pl
			   sb:open()
-- 			   sb:close(1)
			end
			return
		end

		return evt
	end

	-- Song-Browser open
	-- -----------------
	function song_browser_open(sb, which)
	   sb.closed = nil
	   if not which then
		  sb.cl:open()
		  if sb.cl == sb.fl then
			 sb.pl:close()
		  else
			 sb.fl:close()
		  end
	   elseif which == 1 then
 		  sb.fl:open()
	   else
		  sb.pl:open()
	   end	  
	end

	-- Song-Browser close
	-- ------------------
	function song_browser_close(sb, which)
-- 	   print ("close",which)
	   if not which then
		  sb.fl:close()
		  sb.pl:close()
	   elseif which == 1 then
		  sb.fl:close()
	   else
		  sb.pl:close()
	   end
--		sb.fade = -4;
	end

	-- Song-Browser shutdown
	-- ---------------------
	function song_browser_shutdown(sb)
		if not sb then return end
		sb.fl:shutdown()
		sb.pl:shutdown()
		local i,v
		for i,v in sb do
			sb[i] = nil
		end
	end

	--- Song-Browser draw.
	--
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

	   local a1,r1,g1,b1 = 1.0, 0.2, 0.8, 1.0
	   local a2,r2,g2,b2 = 1.0, 0.8, 0.5, 0.8

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

	--- Song-Browser set color
	--
	function song_browser_set_color(sb, a, r, g, b)
	   sb.fl:set_color(a,r,g,b)
	   sb.pl:set_color(a,r,g,b)

-- 		local f1,f2
-- 		a = a or sb.alpha or 1
-- 		sb.alpha = a
-- 		f1 = 1
-- 		f2 = 0.5
-- 		if sb.cl == sb.pl then
-- 			f1,f2 = f2,f1
-- 		end
-- 		dl_set_color(sb.fdl,a*f1,r,g,b)
-- 		dl_set_color(sb.pdl,a*f2,r,g,b)
-- 		sb.fl:set_color(a*f1,r,g,b)
-- 		sb.pl:set_color(a*f2,r,g,b)
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

		-- Methods
		open = song_browser_open,
		close = song_browser_close,
		set_color = song_browser_set_color,
		draw = song_browser_draw,
		confirm = song_browser_confirm,
		cancel = song_browser_cancel,
		select = song_browser_select,
		shutdown = song_browser_shutdown,

		-- Members
		style = style,
		fdl = dl_new_list(64),
		pdl = dl_new_list(64),
		z = gui_guess_z(owner,z),
		fade = 0,

	}

	local x,y,z
	local box = { 0, 0, 256, 210 }
	local minmax = { box[3], box[4], box[3], box[4] }

	x = 42
	y = 120
	z = sb.z - 2

--    *     - @b nil  if action failed
--    *     - @b 0    ok but no change
--    *     - @b 1    if entry is "confirmed"
--    *     - @b 2    if entry is "not confirmed" but change occurs
--    *     - @b 3    if entry is "confirmed" and change occurs
	function sbfl_confirm(fl, sb)
	   print("sbfl_confirm")
	   local idx = fl.pos+1
	   local entry = fl.dir[idx]
	   local entry_path = fl.dir.path or "/"
	   entry_path = canonical(entry_path .. "/" .. fl.dir[idx].file)
	   
	   if entry.size and entry.size==-1 then
		  entrylist_load(fl.dir,entry_path)
		  fl:change_dir(fl.dir)
	   else
		  song_browser_play(sb, entry_path, 0, 1)
		  return 1
	   end 
	end

	function song_browser_stop(sb)
	   sb.stopping = 1
	   playa_fade(-1)
	end

	function song_browser_play(sb, filename, track, immediat)
	   sb.stopping = nil
	   playa_play(filename, track, immediat)
	   playa_fade(2)
	end

	function sbfl_cancel(fl, sb)
	   print("sbfl_cancel")
	   if playa_play() == 1 then
		  song_browser_stop(sb);
	   else
		  evt_shutdown_app(sb)
	   end
	end

	function sbfl_select(fl, sb)
	   print("sbfl_select")
	   sbpl_insert(sb, fl:get_entry())
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
			})
	sb.fl.cancel = sbfl_cancel
	sb.fl.select = sbfl_select

	sb.fl.fade_min = 0.3
	sb.fl.draw_background_old = sb.fl.draw_background
	sb.fl.draw_background = song_browser_list_draw_background


	function sbpl_insert(sb, entry)
	   sb.pl:insert_entry(entry)
	end

	function sbpl_confirm(pl, sb)
	   sb.playlist_idx = pl.pos
	   print("START PLAYLIST AT " .. sb.playlist_idx)
	   playa_stop()
	end

	function sbpl_cancel(pl, sb)
	   evt_shutdown_app(sb)
	end

	function sbpl_select(pl, sb)
	end

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
			} )
	sb.pl.confirm = sbpl_confirm
	sb.pl.cancel = sbpl_cancel
	sb.pl.select = sbpl_select

	sb.pl.fade_min = sb.fl.fade_min
	sb.pl.draw_background_old = sb.pl.draw_background
	sb.pl.draw_background = sb.fl.draw_background

	sb.cl = sb.fl

	song_browser_create_sprite(sb)
	entrylist_load(sb.fl.dir,"/")
	sb.fl:change_dir(sb.fl.dir)
	sb:set_color(0, 1, 1, 1)
	sb:draw()
	sb:open()
	evt_app_insert_first(owner, sb)

	return sb
end

if not entrylist_tag then
	dl(plug_el)
end

sb = song_browser_create()

function k()
	if sb then evt_shutdown_app(sb) end
	sb = nil
end


