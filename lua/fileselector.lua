--- fileselector.lua
-- 
-- author : benjamin gerard <ben@sashipa.com>
-- date   : 2002/10/04
--
-- $Id: fileselector.lua,v 1.3 2002-10-06 22:47:29 vincentp Exp $
--

--- filelist - Display a filelist from a given path 
--
-- components :
-- "dir"     current direntory list
-- "dl"      current display list
-- "cdl"     cursor display list
-- "pos"     current select item
-- "top"     top displayed line
-- "lines"   max displayed lines
-- "entries" number of entry in "dir"
-- "w"       current max width to display this "dir"
-- "h"       current max height to display one entry of this dir
-- "boxw"    display box width
-- "boxh"    display box height
-- "box"     display bounding box
-- "border"  Size of border
-- "span"    Size of line separator (up and down)
-- "mtx"     Current transform matrix
-- "cmtx"    Cursor transform matrix (local, need to be x by mtx)
-- "pwd"     Current path

-- Filelist create:
-- 
-- lines : number of line displayed, default 8, min 1, max 20
-- x,y,z : Box position
--
function filelist_create(path, lines, x, y, z)
	local fl = {}

	if not path then
		path = PWD
	else
		path = fullpath(path)
	end
	if not lines then lines = 8 end
	if lines < 1 then lines = 1
	elseif lines > 20 then lines = 20 end

	if not x then x = 20 end
	if not y then y = 20 end
	if not z then z = 2  end

	fl.pos		= 0
	fl.dl		= nil
	fl.cdl		= nil
	fl.pos		= 0
	fl.lines	= lines
	fl.dir		= nil
	fl.border	= 3
	fl.span		= 1
	fl.top		= 0
	fl.boxw		= 0
	fl.boxh		= 0
	fl.pwd 		= path
	fl.box 		= {x,y,x,y}

	fl.cmtx   = mat_new()
	filelist_position(fl,x,y,z)
	filelist_path(fl,"")

	return fl
end

-- Filelist shutdown:
-- 
function filelist_shutdown(fl)
	if not fl then return end
	if fl.dl  then dl_destroy_list(fl.dl)  end
	if fl.cdl then dl_destroy_list(fl.cdl) end
	fl.mtx  = nil
	fl.cmtx = nil
	fl.dir  = nil
end

-- Allocate display_list, try to make an approx of the needed size
-- 
function filelist_create_dl(fl)
	if not fl then return end

	-- Get current size
	local cursize=0
	if fl.dl then cursize = dl_heap_size(fl.dl) end
--	print("current heap size:"..cursize)
	
	-- Compute an approx size for the new display list
	local i
	local dlsize = 128 -- For the dl_draw_box
	local max=fl.top+fl.lines
	if max > fl.entries then max = fl.entries end
	for i=fl.top+1, max, 1 do
		dlsize = dlsize + 64 + strlen(fl.dir[i].name);
	end
--	print("compute dlsize:"..dlsize)

	if dlsize > cursize then
		-- Need more room
		if fl.dl then dl_destroy_list(fl.dl) end
		fl.dl = dl_new_list(dlsize,0)
		dl_set_trans(fl.dl, fl.mtx)
	else
		dl_set_active(fl.dl,0)
		dl_clear(fl.dl)
	end

end

-- Filelist change current path:
--
function filelist_path(fl,path)
	if not fl or type(path) ~= "string" then return end
	if strsub(path,1,1) ~= "/" then
		path = fl.pwd.."/"..path
	end
	path = fullpath(path)

	-- Load new path --
	local dir=dirlist("-n",path)
	if not dir then 
		print(format("filelist: failed to load '%s'",path))
		return
	end
	fl.dir = dir
	fl.entries = getn(fl.dir)
	fl.pwd = path
	print(format("filelist: pwd '%s'",fl.pwd))
	fl.w   = 0
	fl.h   = 0

	-- Need a display list to compute text measure --
	if not fl.dl then fl.dl = dl_new_list(16,0) end

	-- Measure text --
	if fl.entries > 0 then
		local i, w, h
		w,h = dl_measure_text(fl.dl, fl.dir[1].name)
		for i=2, fl.entries, 1 do
			local w2,h2
			w2,h2 = dl_measure_text(fl.dl, fl.dir[i].name)
			if w2 > w then w = w2 end
			if h2 > h then h = h2 end
		end
		fl.w = w
		fl.h = h + fl.span 
	end

	fl.boxw = fl.w + 2*fl.border
	fl.boxh = (fl.h+2*fl.span)*fl.lines+2*fl.border
	local l = mat_li(fl.mtx,3)
	
	-- Do not create another box, there is reference once this one !
	fl.box[1] = l[1]
	fl.box[2] = l[2]
	fl.box[3] =	l[1]+fl.boxw
	fl.box[4] =	l[2]+fl.boxh

--	print(format("filelist : bbox'%dx%d'",fl.boxw,fl.boxh))

	-- Set invalid top and pos will force update for both dl --
	fl.top = 1
	fl.pos = 1
	filelist_movecursor(fl,-1)
end	

-- Filelist move cursor from 'mov'
--
function filelist_movecursor(fl,mov)
	if not fl then return end
	local top,pos
	top = fl.top
	pos = fl.pos + mov
	if pos >= fl.entries then pos = fl.entries-1 end
	if pos < 0 then pos = 0 end
	if pos < top then
		top = pos
	elseif pos >= top+fl.lines then
		top = pos - fl.lines + 1
		if top < 0 then top = 0 end
	end
	
	if pos ~= fl.pos then
--		print(format("filelist : pos '%d'",pos))
		fl.cmtx = mat_trans(0, (pos-top) * (fl.h + 2*fl.span), 0.1)
		fl.pos = pos
		filelist_updatecursor(fl)
	end

	if top ~= fl.top then
--		print(format("filelist : top '%d'",top))
		fl.top = top
		filelist_update(fl)
	end
end

function filelist_updatecursor(fl)
	if not fl then return end
	if fl.entries > 0 then
		if not fl.cdl then
			fl.cdl = dl_new_list(128,0);
		else
			dl_clear(fl.cdl)
		end
		dl_set_trans(fl.cdl, mat_mult(fl.mtx,fl.cmtx)) -- $$$ order ?
		dl_draw_box(fl.cdl,
					0, 0, fl.boxw, fl.h+2*fl.span, 0,
					0.5,1,1,0,
					0.5,0.5,0.5,0)
		dl_set_active(fl.cdl,1)
	end
end

function filelist_update(fl)
	if not fl then return end

	filelist_create_dl(fl)
	if not fl.dl then return end
	dl_draw_box(fl.dl,
				0, 0, fl.boxw, fl.boxh,	0,
				0.7, 0, 0, 0,
				0.7, 0, 0, 0)

	local i,y
	local max=fl.top+fl.lines
	if max > fl.entries then max = fl.entries end
	y = fl.span
	for i=fl.top+1, max, 1 do
		dl_draw_text(fl.dl,
					fl.border,y,0.1,
					1,1,1,1,
					fl.dir[i].name)
		y = y + fl.h+2*fl.span
	end
	dl_set_active(fl.dl,1)
end
	
function filelist_position(fl,x,y,z)
	if fl then
		if not x then x = fl.box[1] end
		if not y then y = fl.box[2] end
		if not z then z = mat_el(fl.mtx,3,3) end
		fl.box = { x, y, x+fl.boxw, y+fl.boxh }
		fl.mtx = mat_trans(x,y,z)
		if fl.dl  then dl_set_trans(fl.dl,  fl.mtx) end
		if fl.cdl then dl_set_trans(fl.cdl, mat_mult(fl.mtx, fl.cmtx)) end
	end
end

function filelist_center(fl, x, y, w, h, z)
	if x and w then
		x = x + (w-fl.boxw) * 0.5
	end
	if y and h then
		y = y + (h-fl.boxh) * 0.5
	end
	filelist_position(fl,x,y,z)
end

-- filelist confirm action
function filelist_confirm(fl)
	local result = nil
	if fl then
		local dir = fl.dir
		if dir then
			local entry = dir[fl.pos+1]
			if entry.size == -1 then
				filelist_path(fl, entry.name)
			else
				result = { name=entry.name, size=entry.size }
			end
		end
	end
	return result
end

--- Run a standalone filelist, do not need event system
--
function filelist_standalone_run(fl)
	if not fl then return nil end
	local done = nil
	local result = nil
	local actions = {
		[KBD_CONT1_DPAD_UP] 	= 1,	[KBD_CONT1_DPAD2_UP] 	= 1,
		[KBD_CONT2_DPAD_UP] 	= 1,	[KBD_CONT2_DPAD2_UP] 	= 1,
		[KBD_CONT3_DPAD_UP] 	= 1,	[KBD_CONT3_DPAD2_UP] 	= 1,
		[KBD_CONT4_DPAD_UP] 	= 1,	[KBD_CONT4_DPAD2_UP] 	= 1,
		[KBD_KEY_UP] 			= 1,

		[KBD_CONT1_DPAD_DOWN] 	= 2,	[KBD_CONT1_DPAD2_DOWN] 	= 2,
		[KBD_CONT2_DPAD_DOWN] 	= 2,	[KBD_CONT2_DPAD2_DOWN] 	= 2,
		[KBD_CONT3_DPAD_DOWN] 	= 2,	[KBD_CONT3_DPAD2_DOWN] 	= 2,
		[KBD_CONT4_DPAD_DOWN] 	= 2,	[KBD_CONT4_DPAD2_DOWN] 	= 2,
		[KBD_KEY_DOWN] 			= 2,

		[KBD_CONT1_B] 			= 3,	[KBD_CONT1_B] 			= 3,
		[KBD_CONT2_B] 			= 3,	[KBD_CONT2_B] 			= 3,
		[KBD_CONT3_B] 			= 3,	[KBD_CONT3_B] 			= 3,
		[KBD_CONT4_B] 			= 3,	[KBD_CONT4_B] 			= 3,
		[KBD_ESC] 				= 3,

		[KBD_CONT1_A] 			= 4,	[KBD_CONT1_A] 			= 4,
		[KBD_CONT2_A] 			= 4,	[KBD_CONT2_A] 			= 4,
		[KBD_CONT3_A] 			= 4,	[KBD_CONT3_A] 			= 4,
		[KBD_CONT4_A] 			= 4,	[KBD_CONT4_A] 			= 4,
		[KBD_ENTER] 			= 4}

	local cond_state = cond_connect(nil)
	repeat
		local action = actions[getchar()]
		if action == 4 then
			result = filelist_confirm(fl)
			if result then
				done = 1
			end
		elseif action == 1 then
			filelist_movecursor(fl, -1)
		elseif action == 2 then
			filelist_movecursor(fl,  1)
		elseif action == 3 then
			done = 1
		end
	until done
	filelist_shutdown(fl)
	cond_connect(cond_state)
	return result
end

-- filelist application event handler
--
function filelist_gui_handle(app,evt)
	local key = evt.key
	local fl = app.fl
	local dir = nil

	if fl then dir = fl.dir end

	if key == evt_shutdown_event then
		print("filelist : handle shutdown")
		filelist_shutdown(fl)
		app.done = 1
		return evt
	end

	if not dir or not gui_is_focus(app.owner,app) then
		-- No dir loaded or no focus, ignore event --
		return evt;
	elseif gui_keyup[key] then
		filelist_movecursor(fl,-1);
		return nil
	elseif gui_keydown[key] then
		filelist_movecursor(fl,1);
		return nil
	elseif gui_keycancel[key] then
		evt_send(app.owner, { key = gui_filelist_event })
		return nil
	elseif gui_keyconfirm[key] then
		local result = filelist_confirm(fl)
		if result then
			evt_send(app.owner, { key = gui_filelist_event, file=result })
		end
	else
		return evt
	end
end


function filelist_create_app(fl,owner)
	app = {	name = "filelist",
			version = "0.1",
			handle = filelist_app_handle,
--			update = filelist_app_update,
			fl = fl}
	evt_app_insert_first(owner, app)
	return app
end

function filelist_create_gui(fl, owner)
	if not fl or not owner then return nil end
	local app = filelist_create_app(fl,owner)
	if not app then return nil end

	if not gui_filelist_event then gui_filelist_event = evt_new_code() end

	app.z		= gui_guess_z(owner, nil)
	filelist_position(fl, owner.box[1], owner.box[2], gui_guess_z(app,nil))
	app.fl		= fl
	app.name	= "gui "..app.name
	app.box		= app.fl.box
	app.dl		= nil
	app.heritedhandle = app.handle
	app.handle = filelist_gui_handle
	app.event_table = {}
	app.flags 		= {}
	return app
end

--- Create filelist gui application
--
function gui_filelist(owner, path, lines, x, y, z)
	if not owner then return nil end
	local fl = filelist_create(path, lines, x, y, z)
	if not fl then return nil end
	return filelist_create_gui(fl, owner)
end



----------------------------------------------------------------------
-- SAMPLE CODE
----------------------------------------------------------------------

function test_gui_filelist()
	local dial = gui_new_dialog(nil, { 100, 100, 400, 300 }, nil, nil,
					"FILELIST TEST", { x = "left", y = "upout" })
	
	if not dial then return end
	local flist = gui_filelist(dial, "/pc")
	if not flist then
		evt_shutdown_app(dial)
		return
	end
	local but = gui_new_button(dial, { 250, 200, 340, 220 }, "CANCEL")
	if not but then
		evt_shutdown_app(dial)
		return
	end
	but.event_table[gui_press_event] =
		function(but, evt)
			evt_shutdown_app(but.owner)
			return nil
		end
	local input = gui_new_input(dial, { 120, 280, 370, 300 }, "File:",
				{ x = "left", y="upout" }, flist.pwd)
	if not input then
		evt_shutdown_app(dial)
		return
	end
	input.event_table[gui_filelist_event] =
		function(app,evt)
			if evt.file then
				gui_input_set(app,evt.file.name)
			end
			gui_new_focus(app.owner, app)
			return
		end
	return dial
end

----------------------------------------------------------------------
----------------------------------------------------------------------
--
--- filelist_standalone_run() sample code
--
if nil then
	local result = nil
	fl = filelist_create("/pc")
	if fl then
		filelist_center(fl, 0, 0, 640, 480, 2000)
		result = filelist_standalone_run(fl)
		if result then
			print (format("filelist_standalone_run()->{'%s', %d}",
							result.name, result.size))
		else
			print ("filelist_standalone_run->nil")
		end
	end
--
--- gui_filelist() sample code
--
elseif not nil then
	dial = test_gui_filelist()
end



----------------------------------------------------------------------
-- END OF SAMPLE CODE
----------------------------------------------------------------------

------------------------------------------------------------
if nil then
fl = gui_filelist(dial, "/pc", nil,
		dial.box[1]+5, dial.box[2]+5, dial.z+.2)

if fl == nil then
	gui_dialog_shutdown(dial)
end
end
------------------------------------------------------------

if nil then
   	filelist_center(fl, 0, 0, 640, 480, 2)
	print("Press any ENTER to exit")
	done = nil
	repeat
		framecounter(1)
		key = getchar()
		print(format("k:%08x",key))
		if key == KBD_KEY_DOWN then
			filelist_movecursor(fl,  1)
		elseif key == KBD_KEY_UP then
			filelist_movecursor(fl, -1)
		elseif key == KBD_ENTER then
			print(format("{%s} %d",fl.dir[fl.pos+1].name,
					fl.dir[fl.pos+1].size))
			if fl.dir[fl.pos+1].size == -1 then
				filelist_path(fl, fl.dir[fl.pos+1].name)
			end
		else
			done = 1
		end
	until done
	print("Shutdown filelist:")
	filelist_shutdown(fl)
	fl=nil
end

function fileselector(name,path,filename)
  local dial,but,input

-- Set default parameters
  if not name then name="File Selector" end
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
  dial.fname =
	gui_new_input(
    	dial,
        { 120, 160, 380, 190 },
	    "File:",
	    { x = "left", y="upout" },
	    filename)

  print("Filesector out")
  print("input:"..input.input)

  return dial
end

print("loaded fileselect.lua")
