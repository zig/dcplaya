--- @date 2002/12/06
--- @author benjamin gerard <ben@sashipa.com>
--- @brief  LUA script to initialize dcplaya VMU backup.
--- $Id: vmu_init.lua,v 1.10 2003-02-27 10:05:26 ben Exp $
---

-- Unload library
vmu_init_loaded = nil

--- Global VMU path variable
vmu_path = nil

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

-- Initialze ram disk has soon as possible
ramdisk_init()

-- Load required libraries
if not dolib("vmu_select") then return nil end
if not dolib("textviewer") then return nil end

--- Initialize the RAM disk.
--

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

function dcplaya_welcome()


end

--- Initialise VMU path.
--
function vmu_init()
   local done
   local force_choice -- Avoid infinite loop when only one file and corrupt

   while not done do
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

      --- Must selct a file if more than one was found
      if found.n and found.n > 1 then
	 local result
	 result = gui_ask('More than one dcplaya save file has been found. You are going to select the one you want to use.',
			  { "select file","cancel" },
			  nil,
			  "More than one dcplaya file")
	 if result and result == 1 then
	    local vs = vmu_select_create(nil,"Select a file", found)
	    if not vs then
	       print("vmu_init : error VMS selection")
	    else
	       choice = evt_run_standalone(vs)
	    end
	 end
      else
	 if not force_choice and found.n and found.n == 1 then
	    choice = found[1]
	 else
	    local ok
	    while not ok do
	       local result = gui_ask('<left>No dcplaya save has been file found.<p vspace="2">It may be the first time you launch dcplaya. If it is the case, you should read dcplaya information.<p><vspace h="20"><center><font size="20" color="#FFFF00">First aid<p vspace="4"><vspace h="10"><font size="16" color="#FFFFFF">\016 confirm<br>\017 cancel<br><font size="8">\004\006<font size="16"> move focus cursor<br><br>',
				      {'read info', 'create save', 'cancel'},
				      400,
				      "No dcplaya save file"
				)
	       result = result or 0
	       if result == 1 then
		  -- Read info
		  -- $$$ todo : add localisation folder...
		  local rscpath = home .. "lua/rsc/text/"
		  local newbie_text = rscpath .. "newbie.txt"
		  local introduction_text = rscpath .. "introduction.txt"
		  local plugins_text = rscpath .. "plugins.txt"
		  local welcome_text = rscpath .. "welcome.txt"
		  local warning_text = rscpath .. "warning.txt"
		  local greetings_text = rscpath .. "greetings.txt"
		  local authors_text = rscpath .. "authors.txt"
		  local tv = gui_text_viewer(nil,
					     {
						newbie    = newbie_text,
						welcome   = welcome_text,
						warning   = warning_text,
						plugins   = plugins_text,
						greetings = greetings_text,
						authors   = authors_text,
					     } , nil, "Welcome", nil)
		  if (tv) then
		     gui_text_viewer_set_tt(tv,"welcome");
		     evt_run_standalone(tv);
		  end
		  tv = nil
	       elseif result == 2 then
		  ok = 1
	       else
		  return
	       end
	    end
	 end
      end
      
      if not choice then
	 local vs = vmu_select_create(nil,"Select VMS")
	 if not vs then
	    print("vmu_init : error VMS selection")
	 else
	    choice = evt_run_standalone(vs)
	    if type(choice) == "table" then
	       vmu_path = "/vmu/" .. choice.name
	       done = 1
	    end
	 end
      else
	 vmu_path = "/vmu/" .. choice.name
	 done = vmu_load_file(vmu_path, "/ram/dcplaya")

	 if not done then
	    local result
	    result = gui_ask('<left>Invalid dcplaya save file. File may be corrupt.<br><center><img name="grimley" src="stock_grimley.tga" scale="1.5">',
			     { "try another","cancel" },
			     nil,
			     "vmu file error")
	    if not result or result ~= 1 then
	       return
	    end

	    -- avoid looping
	    force_choice = found.n and found.n == 1

	 end
      end
   end
end

vmu_init()
