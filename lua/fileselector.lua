--

function filelist(path,entries,filter)
{
-- set default parameters
  if not path then path=PWD end
  if not entries then entries = 10 end
  if not filter then filter = function(name) return 1 end

  local dial = gui_new_dialog(
          evt_desktop_app,
	  { 0, 0, 400, 300 },
	  2000, nil,
	  name,
	{ x = "left", y = "upout" } )

}

function fileselector(name,path,filename)
  local dial,but,input

-- Set default parameters
  if not name then name="Fileselector" end
  if not path then path=PWD end

-- Create new event if not exist
   if not evt_mkdir_event then
     evt_mkdir_event = evt_new_code()
     print("create evt_mkdir_event="..evt_mkdir_event)
   end

-- create a dialog box with a label outside of the box
-- function gui_new_dialog(owner, box, z, dlsize, text, mode)
  dial = gui_new_dialog(
          evt_desktop_app,
	  { 100, 100, 400, 300 },
	  2000, nil,
	  name,
	{ x = "left", y = "upout" } )
  dial.event_table[evt_mkdir_event] =
    function(dial,evt)
      print("MKDIR-RECIEVE : "..dial.path.." "..dial.fname.input)
      return nil -- block the event
    end
  dial.path = path

  but = gui_new_button(dial, { 250, 200, 340, 220 }, "CANCEL")
  but.event_table[gui_press_event] =
    function(but, evt)
      print [[CANCEL !!]]
      evt_shutdown_app(but.owner)
      return nil -- block the event
    end

  but = gui_new_button(dial, { 250, 230, 340, 250 }, "CREATE DIR")
  but.event_table[gui_press_event] =
    function(but, evt)
      print [[CREATE-DIR]]
      evt_send(but.owner, { key = evt_mkdir_event })
      return nil -- block the event
    end


-- create an input item
  dial.fname = gui_new_input(
            dial,
            { 120, 160, 380, 190 },
	    "File:",
	    { x = "left", y="upout" },
	    filename)

  path="yoyo"
  print("Filesector out")
  print("input:"..input.input)

  return dial;
end

