--- @date 2002/12/06
--- @author benjamin gerard <ben@sashipa.com>
--- @brief  LUA script to initialize dcplaya VMU backup.
--- $Id: vmu_init.lua,v 1.2 2002-12-09 16:26:49 ben Exp $
---

-- Unload library
vmu_init_loaded = nil

-- Load required libraries
dolib("vmu_select")

--- Global VMU path variable
vmu_path = nil

--- Initialize the RAM disk.
--
function ramdisk_init()
   print("Initializing RAM disk.")
   if not test ("-d","/ram") then
	  print("ramdisk_init : no ramdisk")
	  return
   end
   
   local i,v
   for i,v in { "tmp", "dcplaya",
	  "dcplaya/lua", "dcplaya/configs", "dcplaya/playlists" } do
	  print("ramdisk_init : create [" .. v .. "] directory")
	  mkdir("/ram/" .. v)
   end
end

--- Create dcplaya save file.
---
--- @param  fname  Name of dcplaya save file.
--- @param  path   Path to archive (typically "/ram/dcplaya")
---
--- @return error code
--- @retval 1   success
--- @retval nil failure
---
function vmu_save_file(fname, path)
   local errstr
   local tmpfile="/ram/tmp/backup.dcar"
   local headerfile="/rd/vmu_header.bin"

   fname = fullpath(fname)
   path = fullpath(path)

   -- Create temporary backup file
   unlink(tmpfile)
   if not copy("-fv", headerfile, tmpfile) then
	  errstr = "error copying header file"
   elseif not dcar("av9", tmpfile, path) then
	  errstr = "error creating temporary archive [" 
		 .. tmpfile .. "] from [" .. tostring(path) .. "]"
   elseif not copy("-fv", tmpfile, fname) then
	  errstr = "error copying [" .. tmpfile ..
		 "] to [" .. tostring(fname) .. "]"
   end
   print("vmu_save_file : " .. (errstr or "success"))
   unlink(tmpfile)
   return errstr == nil
end

--- Read and extract save file.
---
--- @param  fname  Name of dcplaya save file
--- @param  path   Extraction path (typically "/ram/dcplaya")
---
--- @return error code
--- @retval 1   success
--- @retval nil failure
---
function vmu_load_file(fname,path)
   if not dcar("xv1664",fname,path) then
	  print("vmu_load_file : failed")
	  return
   else
	  print("vmu_load_file : success")
   end
   return 1
end

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
	  local vs = vmu_select_create(nil,"Select a file", found)

	  if vs then
		 local evt
		 while vs.owner do
			evt = evt_peek()
		 end
		 choice = vs._result
	  end

   else
	  print("1 SAVE FILE, CHOOSE IT")
	  choice = found[1]
   end
			
   if not choice then
	  print("NO DCPLAYA SAVE FILE : CREATE ONE")
	  local vs = vmu_select_create(nil,"Select VMS")
	  if not vs then
		 print("vmu_init : error VMS selection")
	  else
		 local evt
		 while vs.owner do
			evt = evt_peek()
		 end
		 choice = vs._result
	  end

   else
	  print("LOAD VMU FILE ".. choice.name)
   end

   print("-----------------------")

end

ramdisk_init()
vmu_init()
