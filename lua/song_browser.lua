
song_browser_loaded = nil
if not dolib("textlist") then return end
if not dolib("gui") then return end

function song_browser_create(owner, name, box)
	local sb

	if not owner then
		owner = evt_desktop_app
	end
	if not name then name = "song-browser" end

	-- Song-Browser default style
	-- --------------------------
	local style = {
		bkg_color		= { 0.5, 0.7, 0.7, 0.7,  0.5, 0.3, 0.3, 0.3 },
		border			= 5,
		span            = 1,
		file_color		= { 1, 0, 0, 0 },
		dir_color		= { 1, 0, 0, .4 },
		cur_color		= { 1, 1, 1, 1,  1, 0.1, 0.4, 0.5 },
	}

	-- Song-Browser update (handles fade in / fade out)
	-- -------------------
	function song_browser_update(sb, frametime)
		local loading = sb.fl.dir.loading
		if loading then
			sb.fl:change_dir(sb.fl.dir)
		end

		if sb.fade == 0 then return end
		local a = sb.alpha
		a = a + sb.fade * frametime
		if a > 1 then
			a = 1
			sb.fade  = 0
		elseif a < 0 then
			a = 0
			sb.fade  = 0
			dl_set_active(sb.fdl, 0)
			dl_set_active(sb.pdl, 0)
		end
		sb.alpha = a
		sb:set_color(a, 1, 1, 1)
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
			evt_shutdown_app(sb)
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
			end
			sb:open()
			return
		elseif gui_keyright[key] then
			if sb.cl ~= sb.pl then
				sb.cl = sb.pl
			end
			sb:open()
			return
		end

		return evt
	end

	-- Song-Browser open
	-- -----------------
	function song_browser_open(sb)
		sb.fade = 4
		sb.closed = nil
		dl_set_active(sb.fdl,1)
		dl_set_active(sb.pdl,1)
	end

	-- Song-Browser close
	-- ------------------
	function song_browser_close(sb)
		sb.closed = 1
		sb.fade = -4;
	end

	-- Song-Browser shutdown
	-- ---------------------
	function song_browser_shutdown(sb)
		if not sb then return end
		sb.fl:shutdown()
		sb.pl:shutdown()
		dl_destroy_list(sb.fdl)
		dl_destroy_list(sb.pdl)
		local i,v
		for i,v in sb do
			sb[i] = nil
		end
	end

	--- Song-Browser draw.
	--
	function song_browser_draw(menu)
	end

	--- Song-Browser set color
	--
	function song_browser_set_color(sb, a, r, g, b)
		local f1,f2
		a = a or sb.alpha or 1
		sb.alpha = a
		f1 = 1
		f2 = 0.5
		if sb.cl == sb.pl then
			f1,f2 = f2,f1
		end
		dl_set_color(sb.fdl,a*f1,r,g,b)
		dl_set_color(sb.pdl,a*f2,r,g,b)
		sb.fl:set_color(a*f1,r,g,b)
		sb.pl:set_color(a*f2,r,g,b)
	end

	--- Song-Browser confirm
	--
	function song_browser_confirm(sb)
		return sb.cl:confirm(sb)
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
		local entry_path = fl.dir.path

		if not entry_path then
			entry_path="/"
		end
		if entry_path ~= "/" then
			entry_path = entry_path.."/"
		end
		entry_path = entry_path .. fl.dir[idx].file

		if entry.size and entry.size==-1 then
			entrylist_load(fl.dir,entry_path)
			fl:change_dir(fl.dir)
		else
			play(entry_path)
			return 1
		end 
	end

	sb.fl = textlist_create(
				{	pos = {x, y, z},
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
				} )

	function sbpl_confirm(fl)
	end

	x = 341
	sb.pl = textlist_create(
				{	pos= {x, y, z+1},
					box = minmax,
					flags=nil,
					dir=entrylist_new(),
					filecolor = sb.style.file_color,
					dircolor  = sb.style.dir_color,
					bkgcolor  = sb.style.bkg_color,
					curcolor  = sb.style.cur_color,
					border    = sb.style.border,
					span      = sb.style.span,
				} )

	sb.cl = sb.fl

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


