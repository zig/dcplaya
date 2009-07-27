
dolib "gui"

function sub_close(app)
   if app.file then
      closefile(app.file)
      app.file = nil
   end
   app.filename = nil
   app.line = nil

   dl_clear(app.realdl)

   print("subtitles closed")
end

function sub_getline(app)
   if app.filename and not app.file then
      app.file = openfile(app.filename, "r")
   end
   local file = app.file
   if not file then
      return
   end

   local l = read(file, "*l")
   if not l then
      return
   end

   l = read(file, "*l")
   --print(l)
   local i, j, h1, m1, s1, h2, m2, s2 = 
      strfind(l, "(%d*):(%d*):(%d*,%d*).* (%d*):(%d*):(%d*,%d*)")
   print(h1, m1, s1, h2, m2, s2)
   local t1 = h1*3600+m1*60+gsub(s1, ",", ".")
   local t2 = h2*3600+m2*60+gsub(s2, ",", ".")
   --print(t1, t2)

   app.line = { 
      t1=t1,
      t2=t2,
      str = '<font size="28">',
   }

   repeat
      l = read(file, "*l")
      --print(l)
      if l and strlen(l)>1 then
	 app.line.str = app.line.str.."<br>"..l
      end
   until not l or strlen(l)<2
   --print(app.line.str)
end

function sub_update(app)
   if not ff_vtime then return end
   local time = ff_vtime()
   if time == 0 then
      dl_clear(app.realdl)
      return
   end

   local line = app.line
   if not line then
      sub_getline(app)
   end
   line = app.line
   if not line then
      return
   end

   while line.t2 < time do
      if app.display then
	 dl_clear(app.realdl)
	 app.display = nil
      end
      sub_getline(app)
      line = app.line
      if not line then
	 return
      end
   end
   
   if not app.display and line.t1 < time then
      app.display = 1
      local str = line.str
      --str = gsub(str, strchar(233), "e") -- aigu
      --str = gsub(str, strchar(232), "e") -- grave
      --str = gsub(str, strchar(234), "e") -- circonflexe
      --str = gsub(str, strchar(238), "i") -- circonflexe
      --str = gsub(str, strchar(231), "c")
      --str = gsub(str, strchar(199), "C")
      --str = gsub(str, strchar(224), "a")
      --str = gsub(str, strchar(244), "o") -- circonflexe
      gui_justify(app.realdl, app.box, app.z, {0.7,1,1,1}, str, 
		  { x = "center", y="down" })
   end
end


function sub_create(owner, box, z, dlsize, text, mode, name, flags)

   local dial

   if not owner then owner = evt_desktop_app end
   if not owner then print("sub_create : no desktop") return nil end
  
   z = gui_guess_z(owner, z)
--   print(z)

   if not dlsize then
      dlsize = 1*1024
   end
   name = name or "subtitles"
   dial = { 

      name = name,
      version = "0.9",
      
      handle = gui_dialog_basic_handle,
      update = sub_update,
      
      dl = dl_new_list(32, 1, nil, nil, name .. ".dl"),
      realdl = dl_new_list(dlsize, 1, nil, nil, name .. ".realdl"),
      box = box,
      z = z,
      
      event_table = { },
      flags  = flags or { unfocusable = 1 }
   }

   dial.event_table[evt_shutdown_event] = sub_close

   evt_app_insert_last(owner, dial)
   
   return dial

end

function sub_play(filename, app)
   app = app or subtitles
   if not app then
      return
   end

   sub_close(app)
   app.filename = filename

   if filename then
      print("subtitles set to", filename)
   end

   return 1
end

if subtitles then
   evt_shutdown_app(subtitles)
end

subtitles = sub_create(nil, {0, 0, 640, 440})

--sub_play "/pc/sub.srt"


if type(filetype_add) == "function" then
   filetype_add("sub")
   filetype_add("sub", nil, ".srt\0")
end

dolib "song_browser"
song_browser_filelist_actions.confirm["sub"] = 
   function(fl, sb, action, entry_path, entry)
      sub_play(entry_path)
   end


return 1
