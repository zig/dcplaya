--- @file   filelist.lua
--- @author benjamin gerard
--- @date   2002/10/04
--- @brief  Manage and display a list of file.
---
--- $Id: filelist.lua,v 1.14 2003-03-26 23:02:49 ben Exp $
---

--- filelist object - Extends textlist
--
-- components :
-- "filter"  dirlist filter function
-- "path"     Current path

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
      e.full = fl.path..e.name
      return e
   end

   -- Filelist change current path: 
   --
   function filelist_set_path(fl,path,locate)
      if not path then
	 path = fl.path
      else
	 if strsub(path,1,1) ~= "/" then
	    path = fl.path.."/"..path
	 end
	 path = fullpath(path)
      end
      if strsub(path,-1) ~= "/" then
	 path = path.."/"
      end

      -- Load new path --
      -- $$$ missing filter
      local dir=dirlist("-n", path)
      if not dir then
	 print(format("filelist: failed to load '%s'",path))
	 return
      end

      if type(locate) == "string" then
	 -- Trying to locate old path in new one
	 local n,i = getn(dir)
	 for i=1,n do
	    if dir[i].name == locate then
	       locate = i
	       break
	    end
	 end
      end
      if type(locate) ~= "number" then
	 locate = nil
      end

      fl.path = path
      return fl:change_dir(dir, locate)
   end

   if not flparm then flparm = {} end
   if not flparm.confirm then flparm.confirm = filelist_confirm end
   flparm.not_use_tt = 1
   fl = textlist_create(flparm)
   if not fl then return end
   if not flparm.path then
      fl.path = PWD
   else
      fl.path = fullpath(flparm.path)
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
      fl = gui_filelist(evt_desktop_app, { path="/pc/t" } )
      getchar()
      evt_shutdown_app(fl)
   end
end

filelist_loaded = 1
return filelist_loaded
