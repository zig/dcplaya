--- @date 2002/12/06
--- @author benjamin gerard <ben@sashipa.com>
--- @brief  LUA script to initialize dcplaya VMU backup.
--- $Id: vmu_init.lua,v 1.1 2002-12-06 16:36:04 ben Exp $
---

-- Unload library
vmu_init_loaded = nil

-- Load required libraries

--- Global VMU path variable
vmu_path = nil

--- Initialise VMU path.
--
function vmu_init()
   
   local dir

   -- Check available VMU
   dir = dirlist("-n", "/vmu")
   if type(dir) ~= "table" or dir.n < 1 then
	  print("vmu_init : no VMU found.")
	  return
   end

   local found = {}
   -- Check for available dcplaya save files.
   local i
   for i=1, dir.n do
	  local v = dir[i]
	  local vmudir
	  local path = "/vmu/" .. v.name .. "/"
	  print("Scanning VMU .." .. path)
	  vmudir = dirlist(path)
	  if type(vmudir) == "table" then
		 local j
		 for j=1, vmudir.n do
			local w = vmudir[j]
			print(" -> " .. w.name)
			if strfind(w.name,"^dcplaya.*") then
			   w.name = v.name .. "/" .. w.name
			   tinsert(found, w)
			   print(" + Added " .. w.name)
			end
		 end
	  end
   end

   local choice = nil

   if found.n and found.n > 1 then
	  print(found.n .. " SAVE FILES, CHOOSE ONE")
	  choice = found[1]
	  local vs = vmu_select_create(nil,"Select a file", found)
   else
	  print("1 SAVE FILE, CHOOSE IT")
	  choice = found[1]
   end
			
   if not choice then
	  print("NO DCPLAYA SAVE FILE : CREATE ONE")
   else
	  print("LOAD VMU FILE ".. choice.name)
   end

end

vmu_init()
