--- @date 2002/12/06
--- @author benjamin gerard
--- @brief  LUA script to initialize dcplaya VMU backup.
--- $Id: vmu_init.lua,v 1.28 2003-03-29 15:33:07 ben Exp $
---

-- Unload library
vmu_init_loaded = nil

--- Global VMU path variable
ram_path = ram_path or nil

--- Get current VMU file.
function vmu_file()
   return type(vmu_file_default) == "function"
      and vmu_file_default()
end

addhelp("vmu_file",nil,"vmu",
	'vmu_file() : see vmu_file_default(nil)')

--- Set current VMU file.
function vmu_set_file(filepath)
   return type(vmu_file_default) == "function"
      and vmu_file_default(filepath)
end

addhelp("vmu_set_file",nil,"vmu",
	'vmu_set_file(path) : see vmu_file_default(path)')

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
if not dolib("fileselector") then return nil end

function vmu_xxx_file_check_arguments(fctname, fname, path)
   local hd = "["..fctname.."] : "
   if type(fname) ~= "string" then
      print(hd.."missing filename argument")
      return
   end
   if type(path) ~= "string" then
      if __DEBUG then
	 print(hd.."missing path argument, try default")
      end
      if type(ram_path) == "string" then
	 path = ram_path .. "/dcplaya/"
      end
   end
   if type(path) ~= "string" then
      print(hd.."bad path argument (ram_path undefined)")
      return
   end
   return canonical_path(fname), canonical_path(path)
end


--- Initialize the RAM disk.
--

--- Create dcplaya save file.
---
--- @param  fname  Name of dcplaya save file.
--- @param  path   Path to archive (default to ram_path.."/dcplaya")
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

   fname,path = vmu_xxx_file_check_arguments("vmu_save_file",fname,path)
   if not fname then return end

   local hdl = vmu_file_save(fname,path)
   if hdl then
      local status = vmu_file_stat(hdl)
      printf("[vmu_save_file] [%s] : [%s]\n", fname, status)
      return status == "success"
   else
      printf("[vmu_save_file] [%s] : failed\n", fname)
   end

end

addhelp("vmu_save_file",nil,"vmu",
	'vmu_save_file(fname [,path]) : run vmu_file_save() with default path')


--- Read and extract save file.
---
--- @param  fname  Name of dcplaya save file
--- @param  path   Extraction path (default ram_path.."/dcplaya")
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

   fname,path = vmu_xxx_file_check_arguments("vmu_load_file",fname,path)
   if not fname then return end

   local result
   local hdl = vmu_file_load(fname,path)
   if hdl then
      local status = vmu_file_stat(hdl)
      printf("[vmu_load_file] [%s] : [%s]\n", fname, status)
      result = status == "success"
   else
      printf("[vmu_load_file] [%s] : failed\n", fname)
   end
   ramdisk_is_modified() -- clear modified since we don't care here
   return result
end

addhelp("vmu_load_file",nil,"vmu",
	'vmu_load_file(fname [,path]) : run vmu_file_load() with default path')

--- Get pluged VMU list.
function vmu_list()
   -- Check available VMU
   local dir = dirlist("-nh", "/vmu")
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
	 printf("v:%s i:%s n:%s\n", tostring(v), tostring(i), tostring(dir.n))
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
	 return "/vmu/" .. choice
      end
   end
end


function welcome(standalone)
   -- Read info
   -- $$$ todo : add localisation folder...
   local rscpath = home .. "lua/rsc/text/"
   local introduction_text = rscpath .. "introduction.txt"
   local plugins_text = rscpath .. "plugins.txt"
   local warning_text = rscpath .. "warning.txt"
   local greetings_text = rscpath .. "greetings.txt"
   local authors_text = rscpath .. "authors.txt"
   local tv =
      gui_text_viewer(nil,
		      {
			 warning   = warning_text,
			 introduction = introduction_text,
			 plugins   = plugins_text,
			 greetings = greetings_text,
			 authors   = authors_text,
		      } , 500, "Welcome", nil)
   if (tv) then
      gui_text_viewer_set_tt(tv,"warning");
      if not standalone then
	 return tv
      end
      evt_run_standalone(tv);
   end
end

--- Initialise VMU path.
--
--- @param  not_load      if set choosen file will be setted as default but
---                       not loaded.
--- @param  force_choice  If set force to choose a vmu unit and file.
function vmu_init(not_load, force_choice)

   force_choice = (force_choice ~= nil) or 0 -- force to 0/1

   -- Infinite looooooopzzzz !!
   while not nil do
      local choice = nil
      local found = force_choice == 0 and vmu_find_files()
      local count = (found and found.n) or 0
      local tmp_not_load = not_load

      --- Must select a file if more than one was found
      if count > 1 then
	 local result
	 result = gui_ask('More than one dcplaya save file has been found. You are going to select the one you want to use.',
			  { "select file","cancel" },
			  nil,
			  "More than one dcplaya file")
	 if result and result == 1 then
	    -- User don't want to choose ! Deletes the found !
	    count = 0
	    choice = nil 
	 else
	    choice = vmu_choose_file(found)
	    if not choice then
	       -- User don't makes a choice, delete the found and continue. May
	       -- be he wants to choose another the in another vmu.
	       count = 0
	    end
	 end
      end

      if count == 1 then
	 -- found one file only and don'y force choice :
	 -- just get  it and continue.
	 choice = "/vmu/" .. found[1].path .. "/" .. found[1].name
      end

      -- If we don't has a choice here, we had to choose a vmu. If the
      -- default vmu is not setted, we assume is is a new user and display
      -- a welcome dialog.
      if not choice then
	 local ok = (force_choice ~= 2) and (vmu_file_default() ~= nil)

	 while not ok do
	    local result =
	       gui_ask('<left>No dcplaya save has been file found.<p vspace="2">It may be the first time you launch dcplaya. If it is the case, you should read dcplaya information.<p><vspace h="20"><center><font size="20" color="#FFFF00">First aid<p vspace="4"><vspace h="10"><font size="16" color="#FFFFFF">\016 ... confirm<br>\017 ... cancel<br><font size="8">\004.\006<font size="16"> ... move focus cursor<br><br>',
		       {'read info', 'create save', 'cancel'},
		       440,
		       "No dcplaya save file"
		    )
	    result = result or 0
	    if result == 1 then
	       -- Read info choosen
	       welcome(1)
	    elseif result == 2 then
	       -- Create save file choose.
	       ok = 1 -- exit loop
	    else
	       -- cancel choosen. Exit with no further modification.
	       return
	    end
	 end

	 -- Select our vmu unit
	 choice = vmu_choose()
	 if choice then
	    -- Now we have choosen a vmu, we must choose a file
	    local fs = fileselector("Choose VMU file",choice,"dcplaya.01")
	    choice = fs and evt_run_standalone(fs)
	    if choice and not test("-f", choice) then
	       -- User choose a non-existing file. This is for creation only
	       -- disable loading for this loop.
	       tmp_not_load = 1
	    end
	 end

	 if not choice then
	    force_choice = 2 -- No choice made here, Next loop display welcome.
	 end

      end

      if choice then
	 if __DEBUG then
	    printf ("%s for %q", tmp_not_load and "set_file" or "load_file",
		    choice)
	 end
	 -- Now we have a choice, try to do it !
	 local done
	 if tmp_not_load then
	    done = vmu_set_file(choice)
	 else
	    done = vmu_load_file(choice)
	 end
	 if done then
	    -- Everything is OK. Bye-Bye !
	    return 1
	 end
	 printf("[vmu_init] : %q not a valid vmu file.\n",choice)
	 result =
	    gui_ask('<left>Invalid dcplaya save file. File may be corrupt.<br><center><img name="grimley" src="stock_grimley.tga" w="40">',
		    { "try another","cancel"},
		    nil,
		    "vmu file error")
	 if not result or result ~= 1 then
	    return
	 end
	 choice = nil
	 force_choice = 1 -- After one failure, we force choice !
      end
   end
end

function vmu_confirm_write(path)
   local r
   if type(path) ~= "string" then
      return
   end
   if vmu_never_confirm_write then
      return 1
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
