--- @date 2002/12/06
--- @author benjamin gerard <ben@sashipa.com>
--- @brief  LUA script to initialize dcplaya VMU backup.
--- $Id: vmu_init.lua,v 1.6 2003-01-12 19:48:01 ben Exp $
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
function gui_text_viewer(owner, texts, box, label, mode)

   function gui_text_viewer_desactive_tt(tt)
      if tag(tt) ~= tt_tag then return end
      local a,r,g,b = dl_get_color(tt.dl)
      print("desactive!")
      dl_set_color(tt.dl,0,r,g,b)
      dl_set_active(tt.dl,0)
   end

   function gui_text_viewer_set_tt(dial, tt, x, y)
      if type(tt) == "string" then
	 tt = type(dial.tts) == "table" and dial.tts[tt]
	 if not tt then print ("dial=",dial.name) end
      end
      tt = (tag(tt) == tt_tag) and tt

      if tt ~= dial.cur_tt then
	 if dial.old_tt ~= tt then
	    gui_text_viewer_desactive_tt(dial.old_tt)
	 end
	 dial.old_tt = dial.cur_tt
	 dial.cur_tt = tt
      end
      dial.tt_x = x or 0
      dial.tt_y = y or 0
      dial.tt_a = 1
   end

   function gui_text_viewer_set_scroll(dial, x, y)
      local mat
      local tt = dial.cur_tt
      if tag(tt) ~= tt_tag or not dial.tbox then return end
      local tw,th = dial.tbox[3], dial.tbox[4]
      local ttw,tth = tt.total_w, tt.total_h

--       print(tw,th,ttw,tth)

      local xmin,xmax,ymin,ymax

      if tw > ttw then
	 xmin, xmax = 0, 0
      else
	 xmin, xmax = tw-ttw, 0
      end

      if th > tth then
	 ymin, ymax = 0
      else
	 ymin, ymax = th-tth, 0
      end

--       print("minmax ",xmin,ymin,xmax,ymax)
--       print("Scroll ","X:"..x,"Y:"..y)
 
      x = (x < xmin and xmin) or (x > xmax and xmax) or x
      y = (y < ymin and ymin) or (y > ymax and ymax) or y

      dial.tt_x = x
      dial.tt_y = y

--       print("Scroll to ",dial.tt_x,dial.tt_y)
   end

   local border = 8
   local dial
   local buttext = "close"

   -- Button size
   local butw,buth = dl_measure_text(nil,buttext)
   butw = butw + 12
   buth = buth + 8

   if not box then
      box = { 0,0,320,200 }
      local x,y = (640 - box[3]) * 0.5, (480 - box[4]) * 0.5
      box = box + { x, y, x, y }
   end

   local tbox
   tbox = box + { border, border,-border, -(2*border+buth) }

   local minx,miny = 32,32
   local tx,ty, tw, th = tbox[1], tbox[2], tbox[3]-tbox[1], tbox[4]-tbox[2]
   local x,y
   x = (tw < minx and (minx-tw)/2) or 0
   y = (th < miny and (miny-th)/2) or 0

   box = box + { -x, -y, x, y }
   tbox = tbox + { -x, -y, x, y }

   -- Get final text box dim
   tw = tbox[3]-tbox[1]
   th = tbox[4]-tbox[2]

   -- Creates tagged texts
   local tts = {}

   local ttbox0 = { 0,0,tw,th }

   if type(texts) == "string" then
      tts[1] = tt_build(texts, { dl = dl_new_list(1024,1,1) } )
   elseif type(texts) == "table" then
      local i,v
      for i,v in texts do
	 if type(v) == "string" then
	    tts[i] = tt_build(v, { box = ttbox0, dl = dl_new_list(1024,1,1) } )
	 elseif tag(v) == tt_tag then
	    tts[i] = v
	 end
      end
   end

   for i,v in tts do
      print(i)
   end

   -- Create dialog
   dial = gui_new_dialog(owner, box, nil, nil, label, mode, "gui_viewer")
   if not dial then return end

   dial.tbox = ttbox0

   -- Create button
   local bbox = {( box[1] + box[3] - butw) * 0.5, (box[4]-buth-border) }
   bbox[3] = bbox[1] + butw
   bbox[4] = bbox[2] + buth
   dial.button = gui_new_button(dial, bbox, buttext , mode)
   if not dial.button then return end
   dial.button.event_table[gui_press_event] =
	 function(app, evt)
	    evt_shutdown_app(app.owner)
	    app.owner.answer = "close"
	 end

   -- Display tagged-texts
   local tt_dl = dl_new_list(256,1,1)
   dl_set_clipping(tt_dl, 0, 0, tw, th)
   dl_set_trans(tt_dl, mat_trans(tx,ty,90))
   local tt_dl2 = dl_new_list(256,1,1)
   dl_sublist(tt_dl, tt_dl2)
   for i,v in tts do
      if tag(v) == tt_tag then
	 gui_text_viewer_desactive_tt(v)
	 if not dial.cur_tt then
	    gui_text_viewer_set_tt(dial, v, 0, 0)
	 end
	 dl_sublist(tt_dl2, v.dl)
	 tt_draw(v)
      end
   end
   dl_sublist(dial.dl,tt_dl)
   dial.tt_dl = tt_dl2
   dial.tts = tts

   function gui_text_viewer_fadeto(tt, alpha, spd)
      if not alpha or not tt or not spd then return end
      local a,r,g,b = dl_get_color(tt.dl)
      if a < alpha then
	 a = a + spd
	 if a >= alpha then
	    a = alpha
	    alpha = nil
	 end
      else
	 a = a - spd
	 if a <= alpha then
	    a = alpha
	    alpha = nil
	 end
      end
      dl_set_color(tt.dl,a,r,g,b)
      dl_set_active(tt.dl, a > 0)
      return alpha
   end

   function gui_text_viewer_scrollto(x, xgoal)
      if xgoal and x then
	 local scrollmax = 64
	 local newx = xgoal * 0.2 + x * 0.8
	 local movx = newx - x
	 local amovx = abs(movx)
	 if amovx > scrollmax then movx = movx * scrollmax / amovx end
	 x = x + movx
	 if abs(x-xgoal) < 0.01 then
	    x = xgoal
	    xgoal = nil
	 end
      end
      return x, xgoal
   end

   function gui_text_viewer_update(dial, frametime)
      local fadespd = 1 * frametime

      -- Fade in current TT 
      dial.tt_a = gui_text_viewer_fadeto(dial.cur_tt, dial.tt_a, fadespd)

      -- Fade out old TT
      if not gui_text_viewer_fadeto(dial.old_tt, 0, fadespd) then
	 dial.old_tt = nil
      end

--       dial.tv_scroll_step = max(0,(dial.tv_scroll_step or 1-1)

      -- Update scrolling
      if dial.tt_dl and (dial.tt_x or dial.tt_y) then
	 local mat = dl_get_trans(dial.tt_dl)
	 local x,y = mat[4][1], mat[4][2]
	 x, dial.tt_x = gui_text_viewer_scrollto(x, dial.tt_x)
	 y, dial.tt_y = gui_text_viewer_scrollto(y, dial.tt_y)
	 mat[4][1], mat[4][2] = x,y
	 dl_set_trans(dial.tt_dl, mat)
      end

      -- Run "original" dialog update
      if type (dial.tv_old_update) == "function" then
	 dial.tv_old_update(dial, frametime)
      end
   end

   -- Patch dialog event
   function gui_text_viewer_up(dial, evt)
      local mat = dl_get_trans(dial.tt_dl)
      if mat then
	 gui_text_viewer_set_scroll(dial, mat[4][1], mat[4][2] + 12)
      end
   end

   function gui_text_viewer_dw(dial, evt)
      local mat = dl_get_trans(dial.tt_dl)
      if mat then
	 gui_text_viewer_set_scroll(dial, mat[4][1], mat[4][2] - 12)
      end
   end

   function gui_text_viewer_menucreator(target)
      local dial = target;
      if not dial then return end
      
      local cb = {
	 settext = function(menu)
		      local dial, fl = menu.root_menu.target, menu.fl
		      if not fl or not dial then 
			 print("no dial fl ",dial,fl)
			 return
		      end
		      local pos = fl:get_pos()
		      if not pos then
			 print ("no pos")
			 return
		      end
		      local entry = fl.dir[pos]
		      if entry then
			 print("CHANGE TO "..entry.name)
			 gui_text_viewer_set_tt(dial, entry.name)
		      end
		   end,
      }
      local root = ":" .. target.name .. ":" .. 
	 "view >view"
      local view = ":view:"
      if type(dial.tts) == "table" then
	 local i,v
	 for i,v in dial.tts do
	    if tag(v) == tt_tag then
	       view = view .. tostring(i) .. "{settext},"
	    end
	 end
      end

      local def = {
	 root=root,
	 cb = cb,
	 sub = { view = view, }
      }
      return menu_create_defs(def , target)
   end


   for i,v in gui_keyup do
      dial.event_table[i] = gui_text_viewer_up
   end

   for i,v in gui_keydown do
      dial.event_table[i] = gui_text_viewer_dw
   end

   -- Install new update 
   dial.tv_old_update = dial.update
   dial.update = gui_text_viewer_update

   -- Install menu
   dial.mainmenu_def = gui_text_viewer_menucreator

   if nil then
      local header,footer

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

-- <left> <img name="dcplaya" src="dcplaya.tga">
-- <center> <font size="18" color="#FFFF90"> Welcome to dcplaya<br>
-- <br>

local newbie_text = 
[[
<font size="14">
<center><font color="#9090FF">Newbie
<vspace h="4"><p><left><font color="#909090">
If is this the first time you launch dcplaya, you should read this lines.
It contains a 
]]

local welcome_text =
[[
<font size="14">
<center><font color="#90FF00">Greetings
<vspace h="4"><p><left><font color="#909090">
<hspace w="4">The dcplaya team wants to greet people that make this project
possible.
<vspace h="6"><p><center>
GNU [http://www.gnu.org]<br>
The Free Software Foundation<br>
KOS developpers. Particulary Dan Potter & Jordan DeLong of Cryptic Allusion<br>
andrewk for its dcload program.<br>
<left>
]]

local warning_text = 
[[
<font size="14">
<center><font color="#FF9000">Warning
<vspace h="4"><p><left><font color="#909090">
This program is NOT official SEGA production. It is distributed in the
hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.<br>
In other words, dcplaya developpers have worked hard to make it possible.
They have make the best to do a nice program and test it for you during hours.
This is a lot of work and they made it for FREE. They do not want to be implied
with any purchasse for anything that happen to you or to anything while using
this program. Problems may be submit to them but this is without any
warranty !
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
-- <center><font color="#90FF00">Greetings
-- <vspace h="4"><p><left><font color="#909090">
-- The dcplaya team wants to greet people that make this project possible.<br>
-- o - GNU <http://www.gnu.org><br>
-- o - The Free Software Foundation<br>
-- o - KallistiOS (KOS) developpers. Particulary Dan Potter & Jordan DeLong of Cryptic Allusion<br>
-- o - andrewk for its dcload program.

-- <p><center>
-- ]]

gui_text_viewer(nil,
		{
		   welcome = welcome_text,
		   warning = warning_text,
		} , nil, "Welcome", nil)

--vmu_init()
