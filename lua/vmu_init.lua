--- @date 2002/12/06
--- @author benjamin gerard <ben@sashipa.com>
--- @brief  LUA script to initialize dcplaya VMU backup.
--- $Id: vmu_init.lua,v 1.20 2003-03-11 15:18:06 ben Exp $
---

-- Unload library
vmu_init_loaded = nil

--- Global VMU path variable
vmu_path = nil
vmu_leaf = nil
ram_path = ram_path or nil

--- Get current VMU file.
function vmu_file(path,leaf)
   path = path or vmu_path
   leaf = leaf or vmu_leaf
   return type(path) == "string"
      and type(leaf) == "string"
      and canonical_path(path .. '/' .. leaf)
end

--- Set current VMU file.
function vmu_set_file(filepath)
   vmu_path,vmu_leaf = get_path_and_leaf(filepath)
   return vmu_file()
end

function ramdisk_init(path)
   print("Initializing RAM disk.")
   ram_path = path or ram_path or "/ram"

   if not test ("-d",ram_path) then
      ram_path = nil
      print("ramdisk_init : no ramdisk")
      return
   end
   
   local i,v
   for i,v in { "tmp", "dcplaya",
      "dcplaya/lua", "dcplaya/configs", "dcplaya/playlists" } do
      print("ramdisk_init : create [" .. v .. "] directory")
      mkdir(ram_path .. "/" .. v)
   end
end

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
   if type(vmu_file_save) ~= "function" or
      type(vmu_file_stat) ~= "function" then
      return
   end

   fname = canonical_path(fname)
   path = canonical_path(path)

   local hdl = vmu_file_save(fname,path)
   if hdl then
      local status = vmu_file_stat(hdl)
      printf("vmu_save_file [%s] : [%s]", fname, status)
      return status == "success"
   else
      printf("vmu_save_file [%s] : failed", fname)
   end

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
   if type(vmu_file_load) ~= "function" or
      type(vmu_file_stat) ~= "function" then
      return
   end

   fname = canonical_path(fname)
   path = canonical_path(path)

   local hdl = vmu_file_load(fname,path)
   if hdl then
      local status = vmu_file_stat(hdl)
      printf("vmu_load_file [%s] : [%s]", fname, status)
      return status == "success"
   else
      printf("vmu_load_file [%s] : failed", fname)
   end
end

function dcplaya_welcome()
end


--- Get pluged VMU list.
function vmu_list()
   -- Check available VMU
   local dir = dirlist("-nh", "/vmu")
   dump(dir,"vmu_list")
   if type(dir) ~= "table" or dir.n < 1 then
      print("vmu_list : no VMU found.")
      return
   end
   return dir
end

--- Get dcplaya VMU files.
function vmu_find_files(dir, expr)
   expr = expr or "^dcplaya.*"
   dir = dir or vmu_list()
   if type(dir) ~= "table" then return end

   local found = {}
   -- Check for available dcplaya save files.
   local i
   for i=1, dir.n do
      local v = dir[i]
      local vmudir
      
      if type(v) ~= "table" then
	 print(" ------------------------ ")
	 printf("v:%s i:%s n:%s", tostring(v), tostring(i), tostring(dir.n))
	 dump(dir,"vmu_list")
	 print(" ------------------------ ")
      else
	 local path = "/vmu/" .. v.name .. "/"
	 print("Scanning VMU .." .. path)
	 vmudir = dirlist("-nh", path)
	 if type(vmudir) == "table" then
	    local j
	    for j=1, vmudir.n do
	       local w = vmudir[j]
	       print(" -> " .. w.name)
	       if strfind(w.name,expr) then
		  w.path = v.name
		  tinsert(found, w)
		  print(" + Added " .. w.path.. "/" .. w.name)
	       end
	    end
	 end
      end
   end
   return getn(found) > 0 and found
end

--- Choose a VMU.
function vmu_choose()
   local vs = vmu_select_create(nil,"Select VMS")
   if not vs then
      print("vmu_choose : error VMS selection")
   else
      local choice = evt_run_standalone(vs)
      if type(choice) == "string" then
	 return "/vmu/" .. choice
      end
   end
end

function vmu_choose_file(files, dir, expr)
   files = files or vmu_find_files(dir, expr)
   if type(files) ~= "table" then return end
   local vs = vmu_select_create(nil,"Select a file", files)
   if not vs then
      print("vmu_choose_file : error VMS selection")
   else
      choice = evt_run_standalone(vs)
      if type(choice) == "string" then
	 return vmu_set_file("/vmu/" .. choice)
      end
   end
end

--- Initialise VMU path.
--
function vmu_init(force_choice)
   local done
   local choice = nil

   while not done do
      local found = vmu_find_files()

      --- Must select a file if more than one was found
      if found and found.n and found.n > 1 then
	 local result
	 result = gui_ask('More than one dcplaya save file has been found. You are going to select the one you want to use.',
			  { "select file","cancel" },
			  nil,
			  "More than one dcplaya file")
	 if result and result == 1 then
	    choice = vmu_choose_file(found)
	 else
	    choice = vmu_set_file(nil)
	    return
	 end
      else
	 if not force_choice and found and found.n and found.n == 1 then
	    choice = "/vmu/" .. found[1].path .. "/" .. found[1].name
	    printf("CHOICE UNIC %q",choice)
	 else
	    local ok
	    while not ok do
	       local result = gui_ask('<left>No dcplaya save has been file found.<p vspace="2">It may be the first time you launch dcplaya. If it is the case, you should read dcplaya information.<p><vspace h="20"><center><font size="20" color="#FFFF00">First aid<p vspace="4"><vspace h="10"><font size="16" color="#FFFFFF">\016 confirm<br>\017 cancel<br><font size="8">\004\006<font size="16"> move focus cursor<br><br>',
				      {'read info', 'create save', 'cancel'},
				      440,
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
	 choice = vmu_choose()
	 if choice then
	    choice = vmu_set_file(choice .. "/dcplaya.01")
	 end
      else
	 done = vmu_load_file(choice, "/ram/dcplaya")
	 if not done then
	    choice = vmu_set_file(nil)
	    local result
	    result = gui_ask('<left>Invalid dcplaya save file. File may be corrupt.<br><center><img name="grimley" src="stock_grimley.tga" scale="1.5">',
			     { "try another","cancel" },
			     nil,
			     "vmu file error")
	    if not result or result ~= 1 then
	       return
	    end

	    -- avoid looping
	    force_choice = found and found.n and found.n == 1

	 end
      end
   end
   printf("VMU [ path=%q leaf=%q path=%q ]",
	  tostring(vmu_path),tostring(vmu_leaf),
	  tostring(vmu_file()))
end

function vmu_confirm_write(path)
   local r
   if type(path) ~= "string" then
      return
   end

   local col = color_tostring(gui_text_color) or "#FFFFB0"
   r = gui_yesno(
'<macro macro-name="red" macro-cmd="font" color="#FF0000" size="18">' ..
'<macro macro-name="green" macro-cmd="font" color="#00FF00" size="18">' ..
'<macro macro-name="nrm" macro-cmd="font" color="'..col..'" size="16">' ..
'<nrm>You are going to write data on your VMU.' ..
'<p><vspace h="8"><green>['..path..']'..
'<p><vspace h="8"><left><nrm>This is a very <red>experimental<nrm> function of dcplaya. You may <red>lost<nrm> all data and/or <red>damage<nrm> your VMU.<p><vspace h="8">If you confirm <red>do not switch off<nrm> your dreamcast during the process or you will <red>lost<nrm> everything !!<br><center>', 400, "Confirm VMU write", "Write the VMU", "cancel")
   return r and r == 1;
end

vmu_init_loaded = 1
return vmu_init_loaded
