--- @date 2002/12/06
--- @author benjamin gerard <ben@sashipa.com>
--- @brief  LUA script to initialize dcplaya VMU backup.
--- $Id: vmu_init.lua,v 1.3 2003-01-10 13:00:15 ben Exp $
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

-- $$$ To be move 
function gui_text_viewer(text, label)
   local header, footer
   header = '<dialog guiref="dialog"'
   header = header .. ' x="center"'
   header = header .. ' name="gui_viewer"'
   header = header .. ' hint_w="500"'
   header = header .. '>'

   footer = '<p><center><button guiref="close">close</button></dialog>'

   text = '<dialog guiref="text" x="center" name="gui_viewer_text">' ..
      text .. '</dialog>'

   text = header..text..footer

   local tt = tt_build(text, {
			  x = "center",
			  y = "center",
			  box = { 0, 0, 640, 400 },
		       }
		    )
   tt_draw(tt)
   tt.guis.dialog.event_table[evt_shutdown_event] =
      function(app)
	 app.answer = "shutdown"
      end

   local i,v 
   for i,v in tt.guis.dialog.guis do
      print (i,tostring(v))
   end

   tt.guis.dialog.guis["close"].event_table[gui_press_event] =
      function(app, evt)
	 evt_shutdown_app(app.owner)
	 app.owner.answer = "close"
	 return evt
      end

   tt.guis.dialog.guis["text"].event_table[gui_press_event] =
      function(app, evt)
	 print("press")
	 return
      end


   function gui_text_viewer_up_evt(app, evt)

      print(app.owner.name, app.name, "up")
   end

   function gui_text_viewer_down_evt(app, evt)
      print("down")
   end

   local i,v
   for i,v in gui_keyup do
      tt.guis.dialog.guis["text"].event_table[i] = gui_text_viewer_up_evt
   end
   for i,v in gui_keydown do
      tt.guis.dialog.guis["text"].event_table[i] = gui_text_viewer_down_evt
   end

   while not tt.guis.dialog.answer do
      evt_peek()
   end

   print("answer="..tt.guis.dialog.answer)

   return tt.guis.dialog.answer
end

function gui_file_viewer(fname)
end

function dcplaya_welcome()


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

local welcome_text =
[[
<left> <img name="dcplaya" src="dcplaya.tga">
<center> <font size="18" color="#FFFF90"> Welcome to dcplaya<br>
<br>

<font size="14">
<vspace h="8"><p>
<center><font color="#90FF00">Greetings
<vspace h="4"><p><left><font color="#909090">
The dcplaya team wants to greet people that make this project possible.
<vspace h="6"><p><center>
GNU [http://www.gnu.org]<br>
The Free Software Foundation<br>
KOS developpers. Particulary Dan Potter & Jordan DeLong of Cryptic Allusion<br>
andrewk for its dcload program.<br>

<left>
]]

-- [[

-- <left> <img name="dcplaya" src="dcplaya.tga">
-- <center> <font size="18" color="#FFFF90"> Welcome to dcplaya<br>
-- <br>
-- <font size="14">

-- <vspace h="8"><p>
-- <center><font color="#9090FF">Newbie
-- <vspace h="4"><p><left><font color="#909090">
-- If is this the first time you launch dcplaya, you should read this lines.
-- It contains a 

-- <vspace h="8"><p>
-- <center><font color="#9090FF">Presentation
-- <vspace h="4"><p><left><font color="#909090">
-- dcplaya is a music player for the dreamcast.<br> toto

-- <vspace h="8"><p>
-- <center><font color="#FF9000">Warning
-- <vspace h="4"><p><left><font color="#909090">
-- This program is NOT official SEGA production. It is distributed in the
-- hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
-- implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.<br>
-- In other words, dcplaya developpers have worked hard to make it possible.
-- They have make the best to do a nice program and test it for you during hours.
-- This is a lot of work and they made it for FREE. They do not want to be implied
-- with any purchasse for anything that happen to you or to anything while using
-- this program. Problems may be submit to them but this is without any
-- warranty !

-- <vspace h="8"><p>
-- <center><font color="#90FF00">Greetings
-- <vspace h="4"><p><left><font color="#909090">
-- The dcplaya team wants to greet people that make this project possible.<br>
-- o - GNU <http://www.gnu.org><br>
-- o - The Free Software Foundation<br>
-- o - KallistiOS (KOS) developpers. Particulary Dan Potter & Jordan DeLong of Cryptic Allusion<br>
-- o - andrewk for its dcload program.

-- <p><center>
-- ]]


gui_text_viewer(welcome_text, "Welcome")

--vmu_init()
