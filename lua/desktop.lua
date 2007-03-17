--- @todo Document me !!
--- @file   desktop.lua
--- @author vincent penne
--- @brief  desktop application
---
--- $Id: desktop.lua,v 1.38 2007-03-17 14:40:29 vincentp Exp $
---

if not dolib("evt") then return end
if not dolib("gui") then return end
if not dolib("textlist") then return end
if not dolib("sprite") then return end
if not dolib("menu") then return end
if not dolib("taggedtext") then return end

dskt_keytoggle = gui_keymenu or {}
dskt_keyswitchto = gui_keyconfirm or {}
dskt_keymenu = gui_keyselect or {}

function dskt_create_sprites(vs)
   vs.sprites = {}
end

function dskt_killmenu(dial)
   if dial.menu then
      evt_shutdown_app(dial.menu)
      dial.menu = nil
   end
end

function dskt_openmenu(dial, target, x, y)
--    local spr_name, wmm_name = "menu_close", "menu_wmm"
   dskt_killmenu(dial)

   local name = target.name or "app"
   local def
   local user_def = menu_create_defs(target.mainmenu_def, target)
   local root = ":" .. name ..":"
   if not target.flags or not target.flags.unfocusable then
      root = root ..
	 menu_any_menu(1, "switch to", "windowmanager", nil, nil, "switch", -1)
	 .. ","
   end
   root = root ..
      menu_any_menu(1, "kill", "close", nil, nil, "kill", -1)
   local default_def = menu_create_defs
   ({
       root = root,
       cb = {
	  kill = function(menu) 
		    local col = color_tostring(gui_text_color) or "#FFFFB0"
		    if gui_yesno(
'<macro macro-name="yellow" macro-cmd="font" color="#FFFF00" size="18">' ..
'<macro macro-name="nrm" macro-cmd="font" color="'..col..'" size="16">' ..
'Are you sure you want to kill the application <yellow>'..
%target.name..
'<nrm> ?', nil, 'Kill ' .. %target.name .. ' ?', 'Kill ' .. %target.name, 'cancel'
			      ) == 1 then
		       evt_shutdown_app(%dial)
		       evt_shutdown_app(%target)
		       evt_shutdown_app(menu)
		    end
		 end,
	  switch = function(menu) 
		    evt_shutdown_app(%dial)
		    evt_shutdown_app(menu)
		    local owner = %target.owner
		    if owner then
		       gui_new_focus(owner, %target)
		    end
		 end,
       },
    }, target)

   if not user_def then
      def = default_def
   else
      def = menu_merge_def(user_def, default_def)
   end

   dial.menu = gui_menu(dial, name.."-menu", def,
			{x, y, 100, 100})
   if tag(dial.menu) == menu_tag then
      dial.menu.target = target
   end

end

function dskt_switcher_create(owner, name, dir, x, y, z)
   local col = color_tostring(gui_text_color) or "#FFFFB0"
   local text =
'<macro macro-name="yellow" macro-cmd="font" color="#FFFF00">' ..
'<macro macro-name="nrm" macro-cmd="font" color="'..col..'">'

   name = name or "switcher"
   text = text..'<dialog guiref="dialog" label="Desktop" name="' ..name.. '">'
   text = text..'<linecenter>Running applications'..
      ' :<br><vspace h="8"><hspace w="16"><linedown>'

   local i
   for i=1,dir.n, 1 do
      local app = dir[i].app
      local icon_name, icon_file

      icon_name = app.icon_name or app.name or "app_dcplaya"
      icon_file = app.icon_file or (icon_name .. ".tga")
      
      local spr
      spr = sprite_get(icon_name)
      if tag(spr) ~= sprite_tag then
	 -- Sprite doesnot exit : create it
	 sprite_simple(icon_name,icon_file)
      end
      -- Test again
      spr = sprite_get(icon_name)
      if tag(spr) ~= sprite_tag then
	 -- Fallback to default sprite
	 icon_name = "app_dcplaya"
	 icon_file = "dcplaya.tga"
      end

      local name = app.name or "app"
      text = text .. format('<button name=%q total_w="64" guiref="r%d">',
			    (name) .. " switcher" , i)
      if i == 1 then
	 name = '<yellow>'..name
      else
	 name = '<nrm>'..name
      end
      text = text
	 .. '<img name="' .. icon_name
	 .. '" src="' .. icon_file
	 .. '" w="48"><br><center>' .. name
      text = text..'</button><hspace w="16">'
   end

--   text = text..'<br><vspace h="16"><left><linecenter>'..
--      'Launchable application ('..strchar(16)..' launch,'..strchar(18)..
--      ' info) :<br><vspace h="8"><hspace w="16"><linedown>'

   text = text..'<br><vspace h="16"><left><linecenter>'
   text = text..strchar(16)..' ... Switch to selected application<br>'
   text = text..strchar(18)..' ... Application menu<br>'
   text = text..strchar(19)..' ... Close this dialog<linedown>'

   text = text..'</dialog>'

   local tt = tt_build(text, {x="center", y="center"})

   tt_draw(tt)

   for i=1,dir.n, 1 do
      local idx = format("r%d", i)
      local but = tt.guis.dialog.guis[idx]
      if but then
	 but.target = dir[i].app

	 local oldhandle = but.handle
	 but.handle =
	    function(app, evt)
	       local key = evt.key
	       
	       if dskt_keytoggle[key]
		  --or (key == gui_item_confirm_event)
	       then
		  if app.owner then
		     local owner = app.owner.owner
		     evt_shutdown_app(app.owner)
--		     if owner then
--			gui_new_focus(owner, app.target)
--		     end
		     return
		  end
	       end
	       
	       if dskt_keymenu[key] then
		  dskt_openmenu(app.owner, app.target,
				app.box[3], (app.box[2] + app.box[4]) / 2)
		  return
	       end
	       
	       if key == gui_press_event then
		  if app.target.flags and app.target.flags.unfocusable then
		     dskt_openmenu(app.owner, app.target,
				   app.box[3], (app.box[2] + app.box[4]) / 2)
		     return
--		     gui_ask(
--			     'Sorry, this application is not switchable '..
--				'<img name="smiley" src="stock_smiley.tga" scale="1.5">'
--			     , { "OK !" })
		  else
		     if app.owner then
			local owner = app.owner.owner
			evt_shutdown_app(app.owner)
			if owner then
			   gui_new_focus(owner, app.target)
			end
		     end
		     return
		  end
	       end

	       if key == gui_menu_close_event then
--		  print("RECEIVE MENU-CLOSE")
		  dskt_killmenu(app.owner)
		  return
	       elseif key == gui_item_confirm_event then
--		  print("RECEIVE MENU-CONFIRM")
--		  evt_send(app, { key = rawget(dskt_keytoggle,1) })
-- 		  local owner = app.owner.owner
-- 		  evt_shutdown_app(app.owner)
-- 		  gui_new_focus(owner, app.target)

		  return
	       end

	       
	       return %oldhandle(app, evt)
	    end
      end
   end
   
   do return tt.guis.dialog end
end

--- Shutdown application attached to the desktop.
--- @param  app desktop application (nil for default evt_desktop_app)
--- @param  kill_console  kill console application only if set.
---
function dskt_shutdown_children(app, kill_console)
   app = app or evt_desktop_app
   if app then
      local sub = app.sub
      while sub do
	 local nxt = sub.next
	 if kill_console or sub ~= console_app then
	    print("desktop : shutdowning "..tostring(sub.name))
	    evt_shutdown_app(sub)
	 end
	 sub = nxt
      end
   end
end

function dskt_handle(app, evt)
   local key = evt.key

   --- $$$ ben : add missing shutdown. Was it on purpose ?
   ---           this shutdown kill all children app except console !
   if key == evt_shutdown_event then
      print("desktop recieve shutdown event : kill sub app")
      if app.switcher then
	 evt_shutdown_app(app.switcher)
	 app.switcher = nil
      end
      dskt_shutdown_children(app)
      return nil
   end

   --print(key)
   if gui_keynext[key] then
      app.visible = not app.visible
   end

   if dskt_keytoggle[key] then
      if app.switcher then
	 evt_shutdown_app(app.switcher)
	 app.switcher = nil
      else

	 local dir = { n = 0 }

	 local i = app.sub
	 while i do
	    tinsert(dir, { name = i.name, size = 0, app = i })
	    i = i.next
	 end

	 app.switcher =
	    dskt_switcher_create(app,nil,dir)
      end
      return
   end

   if key == evt_app_remove_event and evt.app == app.switcher then
      app.switcher = nil
   end

   if (key == evt_app_insert_event or key == evt_app_remove_event)
      and evt.app.owner == app then

      local focused = app.sub

      if key == evt_app_remove_event and focused and focused == evt.app then
	 focused = focused.next
      end

      if console_app and evt.app ~= console_app 
	 and focused ~= console_app and console_app.next then
	 -- force console to be last application (user friendly)
	 evt_app_insert_last(app, console_app)
      end

      if focused ~= app.focused then
	 if app.focused then
	    --	    print("unfocus", app.focused.name)
	    evt_send(app.focused, { key = gui_unfocus_event, app = app.focused }, 1)
	 end
	 if focused then
	    evt_send(focused, { key = gui_focus_event, app = focused }, 1)
	    --	    print("new focused", focused.name)
	 end
	 app.focused = focused
	 app.focus_time = 0
      end

      gui_child_autoplacement(app)

      return
   end

   if console_app and key < KBD_USER and key ~= shell_toggleconsolekey then
      -- prevent basic input event falling back from one top application to the other
      if __EVT_DEBUG then
	 print("dsk eat ",key)
      end

      return
   end

   if __EVT_DEBUG then
      print("dsk leave ",key)
   end

   return evt
end

-- $$$ Added by ben for debug
function dumpZ()
   local vtx = mat_new(5,4)
   vtx[1][3] = 0
   vtx[1][4] = 1
   vtx[2][3] = 50
   vtx[2][4] = 1
   vtx[3][3] = 100
   vtx[3][4] = 1
   vtx[4][3] = 150
   vtx[4][4] = 1
   vtx[5][3] = 200
   vtx[5][4] = 1

   function testzzz(app,mat,vtx,indent)
      if not app then return end
      indent = indent or 0
      printf(strrep(">", indent) .. "DUMP %q\n", app.name)
      local mtx
      if not app._dl then
	 print("no _dl !!")
	 mtx = mat_new()
      else
	 mtx = dl_get_trans(app._dl) * mat
      end
      mat_dump(vtx * mtx, 1)
      
      -- $$$ test debug
      local a
      a = app.sub
      while a do
	 testzzz(a,mtx,vtx,indent+1)
	 a = a.next
      end
   end

   testzzz(evt_desktop_app, mat_new(), vtx,0)
end

function dskt_update(app, ft)

   if gui_apparate_update then
      gui_apparate_update(app, ft)
   end

   if not app._dl then return end

   local scale = app.global_scale

   if (scale == 1 and app.visible) or 
      (scale == 10 and not app.visible) then
      return
   end

   local aim = 1
   if not app.visible then
      aim = 10
   end

   scale = scale + 7.5 * ft * (aim-scale)

   if scale < 1.005 then
      scale = 1
   end
   
   if scale > 9 then
      scale = 10
      dl_set_active(app._dl, nil)
   else
      dl_set_active(app._dl, 1)
      dl_set_trans(app._dl, 
		   mat_scale(scale, scale, 1)
		      * mat_trans(0*320 * (1-scale), 240 * (1-scale), 0)
		)
      dl_set_color(app._dl, 1 - (scale-1)/8, 1, 1 - (scale-1)/8, 1, 1)
   end

   app.global_scale = scale

end

function dskt_create()
   local app = evt_desktop_app

   if not app then
      print ("No desktop application ! Event system is probably be down.")
      return
   elseif app.handle == dskt_handle then
      print ("Desktop already installed.")
   else
      print("Installing desktop application")
   end

   app.global_scale = 1
   app.visible = 1

   app.handle = dskt_handle
   app.update = dskt_update

   if not app.dl then
      app.dl = dl_new_list(256, 1, nil, "desktop.dl")
   else
      dl_clear(app.dl)
   end
   app.z = 0
   return app
end

-- Create default application sprite
sprite_simple("app_dcplaya","dcplaya.tga")

-- Load application icons
for k,v in { "close", "console", "windowmanager" } do
   local tex = tex_exist(v) or
      tex_new(home .. "lua/rsc/icons/" .. v .. ".tga")
end

return dskt_create() ~= nil
