--- @file   filelist.lua
--- @author benjamin gerard <ben@sashipa.com>
--- @date   2002/10/04
--- @brief  Manage and display a list of file.
---
--- $Id: filelist.lua,v 1.8 2002-10-30 19:59:30 benjihan Exp $
---

--- filelist object - Extends textlist
--
-- components :
-- "filter"  dirlist filter function
-- "pwd"     Current path

-- Unload the library
filelist_loaded = nil

-- Load required libraries
--
if not dolib("textlist") then return end
	
function filelist_create(flparm)

	function filelist_confirm(fl)
		if fl.dir.n < 1 then return end
		local entry = fl.dir[fl.pos+1]
		if entry.size and entry.size==-1 then
			local action = fl:set_path(entry.name)
			if not action then return end
			return 2
		else
			return 3
		end
	end

	function filelist_get_entry(fl)
		if fl.dir.n < 1 then return end
		local e = textlist_get_entry(fl)
		e.full = fl.pwd..e.name
		return e
	end

	-- Filelist change current path: 
	--
	function filelist_set_path(fl,path)
		if not path then
			path = fl.pwd
		else
			if strsub(path,1,1) ~= "/" then
				path = fl.pwd..path
			end
			path = fullpath(path)
		end
		local len = strlen(path)
		if strsub(path,len,len) ~= "/" then
			path = path.."/"
		end

		local start,stop

		-- Load new path --
		-- $$$ missing filter
		local dir=dirlist("-n", path)
		if not dir then
			print(format("filelist: failed to load '%s'",path))
			return
		end
		fl.pwd = path
		return fl:change_dir(dir)
	end

	if not flparm then flparm = {} end
	if not flparm.confirm then flparm.confirm = filelist_confirm end
	fl = textlist_create(flparm)
	if not fl then return end
	if not flparm.pwd then
		fl.pwd = PWD
	else
		fl.pwd = fullpath(flparm.pwd)
	end

	fl.get_entry = filelist_get_entry
	fl.set_path = filelist_set_path
	fl:set_path()
	return fl
end

--- Create textlist gui application.
--
function gui_filelist(owner, flparm)
	if not owner then return nil end
	local fl = filelist_create(flparm)
	if not fl then return nil end
	return textlist_create_gui(fl, owner)
end

if nil then
	print("Run test (y/n) ?")
	c = getchar()
	if c == 121 then
		print ("Create file list")
		fl = gui_filelist(evt_desktop_app, { pwd="/pc/t" } )
		getchar()
		evt_shutdown_app(fl)
	end
end

filelist_loaded = 1
return filelist_loaded
