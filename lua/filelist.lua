--- filelist.lua
-- 
-- author : benjamin gerard <ben@sashipa.com>
-- date   : 2002/10/04
--
-- $Id: filelist.lua,v 1.2 2002-10-07 23:54:58 benjihan Exp $
--

--- filelist object - Extends textlist
--
-- components :
-- "filter"  dirlist filter function
-- "pwd"     Current path

if not textlist_loaded then
	dofile("lua/textlist.lua")
end

function filelist_dump(fl)
	textlist_dump(fl)
	if not fl then return end
	if fl.pwd then
		print(format("pwd='%s'", fl.pwd))
	else
		print("pwd=nil")
	end
end

function filelist_default_confirm(fl)
	if not fl or not fl.dir or not fl.entries then return end
	local entry = fl.dir[fl.pos+1]
	if entry.size and entry.size==-1 then
		filelist_path(fl,entry.name)
		return
	else
		return 1
	end
end
	
function filelist_create(flparm)
	print("filelist_create...")
	if not flparm then flparm = {} end
	if not flparm.confirm then flparm.confirm = filelist_default_confirm end
	fl = textlist_create(flparm)
	if not fl then return end
	if not flparm.pwd then
		fl.pwd = PWD
	else
		fl.pwd = fullpath(flparm.pwd)
	end
	filelist_path(fl,fl.pwd)

	filelist_dump(fl)
	print("...filelist_create")
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


-- Filelist change current path: 
--
function filelist_path(fl,path)
	print("filelist_path...")

	if not fl then return end
	if not path then
		path = fl.pwd
	else
		if strsub(path,1,1) ~= "/" then
			path = fl.pwd.."/"..path
		end
		path = fullpath(path)
	end

	-- Load new path --
	-- $$$ missing filter
	local dir=dirlist("-n",path)
	if not dir then
		print(format("filelist: failed to load '%s'",path))
		return
	end
	fl.dir = dir
	fl.entries = getn(fl.dir)
	fl.pwd = path
	print(format("filelist: pwd '%s' ,%d",fl.pwd, fl.entries))

	local dim = textlist_measure(fl)
	textlist_set_box(fl,nil,nil,
						2*fl.border + dim[1],
						2*fl.border + (fl.font_h+2*fl.span)*fl.entries, nil)

	-- Set invalid top and pos will force update for both dl --
	fl.top = 1
	fl.pos = 1
	textlist_movecursor(fl,-1)

	print("...filelist_path")

end

filelist_loaded = 1
print("Loaded filelist.lua")

print("Run test (y/n) ?")
c = getchar()
if c == 121 then
	print ("Create file list")
	fl = filelist_create( { pwd="/pc/t" } )
	print (fl)
	textlist_standalone_run(fl)
end
