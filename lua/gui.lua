--- @file   gui.lua
--- @author Vincent Penne <ziggy@sashipa.com>
--- @brief  gui lua library on top of evt system
---
--- $Id: gui.lua,v 1.37 2002-12-23 14:15:17 zigziggy Exp $
---

--
-- a gui item is an application (see in evt.lua for the definition) with
-- additionnal informations :
--
-- box : the box of the item { x1, y1, x2, y2 } used for focusing
-- dl  : the default display list used to draw inside
-- z   : the z position of the item for reference to draw upon it
-- event_table : table of function(app, evt) indexed by event ID
-- flags : an array whose entry describe some functionalities
--

-- flags can have :
--
-- "modal"    : the item (usually a dialog box) eats all events except shutdown
--              (can be dangerous !)
-- "inactive" : the item cannot be focused


-- IDEAS :
--
-- * Auto layout of items based on a generalized text justifying algorithm
--
-- * A general gui_ask function to build easy choose dialog boxes
--
-- * A text input item
--
-- * A browser item
--
-- * A valuator item
--
-- * Allowing nested display list, every items could use its own display list,
--   contained into the parent's display list,
--   focusing would then be able to modify color/placement of items 
--   individually with respect to its focused or active state ...
--

if not dolib "evt" or not dolib "box3d" or not dolib "taggedtext" then
   return
end

gui_box_color1 = { 0.8, 0.2, 0.3, 0.3 }
gui_box_color2 = { 0.8, 0.1, 0.1, 0.1 }
gui_button_color1 = { 0.8, 0.2, 0.7, 0.7 }
gui_button_color2 = { 0.8, 0.1, 0.3, 0.3 }
gui_text_color = { 0.9, 1.0, 1.0, 0.7 }
gui_text_shiftbox = { 5, 5, -5, -5 }
gui_input_color1 = { 0.8, 0.2, 0.3, 0.3 }
gui_input_color2 = { 0.8, 0.1, 0.1, 0.1 }
gui_input_cursor_color1 = { 1, 1, 0.5, 0 }
gui_input_cursor_color2 = { 1, 0.5, 1.0, 0 }
gui_focus_border_width = 2
gui_focus_border_height = 2

gui_keyup = { 
   [KBD_KEY_UP] = 1, 
   [KBD_CONT1_DPAD_UP] = 1, [KBD_CONT1_DPAD2_UP] = 1, 
   [KBD_CONT2_DPAD_UP] = 1, [KBD_CONT2_DPAD2_UP] = 1, 
   [KBD_CONT3_DPAD_UP] = 1, [KBD_CONT3_DPAD2_UP] = 1, 
   [KBD_CONT4_DPAD_UP] = 1, [KBD_CONT4_DPAD2_UP] = 1
}
gui_keydown = { 
   [KBD_KEY_DOWN] = 1, 
   [KBD_CONT1_DPAD_DOWN] = 1, [KBD_CONT1_DPAD2_DOWN] = 1, 
   [KBD_CONT2_DPAD_DOWN] = 1, [KBD_CONT2_DPAD2_DOWN] = 1, 
   [KBD_CONT3_DPAD_DOWN] = 1, [KBD_CONT3_DPAD2_DOWN] = 1, 
   [KBD_CONT4_DPAD_DOWN] = 1, [KBD_CONT4_DPAD2_DOWN] = 1
}
gui_keyleft = { 
   [KBD_KEY_LEFT] = 1, 
   [KBD_CONT1_DPAD_LEFT] = 1, [KBD_CONT1_DPAD2_LEFT] = 1, 
   [KBD_CONT2_DPAD_LEFT] = 1, [KBD_CONT2_DPAD2_LEFT] = 1, 
   [KBD_CONT3_DPAD_LEFT] = 1, [KBD_CONT3_DPAD2_LEFT] = 1, 
   [KBD_CONT4_DPAD_LEFT] = 1, [KBD_CONT4_DPAD2_LEFT] = 1
}
gui_keyright = { 
   [KBD_KEY_RIGHT] = 1, 
   [KBD_CONT1_DPAD_RIGHT] = 1, [KBD_CONT1_DPAD2_RIGHT] = 1, 
   [KBD_CONT2_DPAD_RIGHT] = 1, [KBD_CONT2_DPAD2_RIGHT] = 1, 
   [KBD_CONT3_DPAD_RIGHT] = 1, [KBD_CONT3_DPAD2_RIGHT] = 1, 
   [KBD_CONT4_DPAD_RIGHT] = 1, [KBD_CONT4_DPAD2_RIGHT] = 1
}
gui_keyconfirm = { 
   [KBD_ENTER] = 1, 
   [KBD_CONT1_A] = 1, 
   [KBD_CONT2_A] = 1, 
   [KBD_CONT3_A] = 1, 
   [KBD_CONT4_A] = 1, 
}
gui_keycancel = { 
   [KBD_ESC] = 1, 
   [KBD_CONT1_B] = 1, 
   [KBD_CONT2_B] = 1, 
   [KBD_CONT3_B] = 1, 
   [KBD_CONT4_B] = 1, 
}
gui_keymenu = { 
   [KBD_TAB] = 1, 
   [KBD_CONT1_Y] = 1, 
   [KBD_CONT2_Y] = 1, 
   [KBD_CONT3_Y] = 1, 
   [KBD_CONT4_Y] = 1, 
}
gui_keyselect = { 
   [KBD_BACKSPACE] = 1, 
   [KBD_CONT1_X] = 1, 
   [KBD_CONT2_X] = 1, 
   [KBD_CONT3_X] = 1, 
   [KBD_CONT4_X] = 1, 
}

-- compute an automatic guess if none is given
function gui_orphanguess_z(z)
   if not z then
--      z = gui_curz
--      gui_curz = gui_curz + 100
      z = 0
   end
   return z
end

-- compute an automatic guess if none is given, using parent's z
function gui_guess_z(owner, z)
   if not z then
      z = gui_orphanguess_z(owner.z) + 10
   end
   return z
end



-- place child of a dialog box
function gui_child_autoplacement(app)
   if not app._dl1 then
      app._dl1 = dl_new_list(256, 1)
      app._dl2 = dl_new_list(256, 1)
      dl_sublist(app.dl, app._dl1)
      dl_sublist(app.dl, app._dl2)
   else
      dl_clear(app._dl1)
   end

   local i = app.sub
   if not i then
      return
   end
   local n = 0
   while i do
      n = n + 1

      if not i._dl and i.dl and i.dl ~= app.dl then
	 i._dl = dl_new_list(256, 1)
	 dl_sublist(i._dl, i.dl)
      end

      i = i.next
   end

   n = n+1
   local scale = 1/n
   i = app.sub
--   n = 0
   while i do
      n = n - 1

      if i._dl then
	 dl_set_trans(i._dl, mat_trans(0, 0, 100/scale + 100*n) * mat_scale(1, 1, scale/2))
	 dl_sublist(app._dl1, i._dl)
      end
      
      i = i.next
   end

   -- swaping display lists
   dl_set_active2(app._dl1, app._dl2, 1)
   app._dl1, app._dl2 = app._dl2, app._dl1
end


-- change focused item
function gui_new_focus(app, f)
   local of = app.sub
   if f and f~=of then
--      if of then
--	 evt_send(of, { key = gui_unfocus_event, app = of })
--      end
      evt_app_remove(f)
      evt_app_insert_first(app, f)
--      evt_send(f, { key = gui_focus_event, app = f })

      return 1
   end
end


-- test focus item
function gui_is_focus(f)
   return f == f.owner.sub
end

function gui_less(b1, b2, i)
   return b1[i] < b2[i]
end
function gui_more(b1, b2, i)
   --	print (b1[i], b2[i])
   return b1[i] > b2[i]
end

function gui_boxdist(b1, b2)
   local x = b2[1] - b1[1]
   local y = b2[2] - b1[2]
   return x*x + y*y
end

-- find closest item to given box, satisfying given condition
gui_closestcoef = { 1, 1, 1, 1 }
gui_closestcoef_horizontal = { 1, 5, 1, 5 }
gui_closestcoef_vertical = { 5, 1, 5, 1 }
function gui_closest(app, box, coef, cond, condi)

   if not coef then
      coef = gui_closestcoef
   end
   box = box * coef
   local i = app.sub
   if not i then
      return
   end
   local imin = nil
   local min = 1000000000
   repeat
      local b = i.box * coef
      if not cond or cond(b, box, condi) then
	 local d = gui_boxdist(b, box)
	 if d < min then
	    imin = i
	    min = d
	 end
      end

      i = i.next
   until not i

   --	print (min, app.sub, imin)
   
   return imin
   
end

-- A minimal handle function
function gui_minimal_handle(app,evt)
   local key = evt.key
   if key == evt_shutdown_event then
      app.dl = nil
      return evt
   end
   local f = app.event_table[key]
   if f then
      return f(app, evt)
   end
   return evt
end

-- dialog

function gui_dialog_shutdown(app)
   if app.sub then
      evt_send(app.sub, { key = gui_unfocus_event, app = app.sub }, 1)
   end
   dl_set_active(app.dl)
   dl_set_active(app.focusup_dl)
   dl_set_active(app.focusdown_dl)
   dl_set_active(app.focusright_dl)
   dl_set_active(app.focusleft_dl)
   evt_app_remove(app)
--   cond_connect(1)
end

function gui_dialog_handle(app, evt)
   local key = evt.key

   local focused = app.sub

   if key == evt_shutdown_event then
      gui_dialog_shutdown(app)
      return evt -- pass the shutdown event to next app
   end
   
   --	print("dialog key:"..key)
   local f = app.event_table[key]
   if f then
      return f(app, evt)
   end

   if (key == evt_app_insert_event or key == evt_app_remove_event) and evt.app.owner == app then
      if key == evt_app_remove_event and focused and focused == evt.app then
	 focused = focused.next
      end

      if focused ~= app.focused then
	 if app.focused then
	    evt_send(app.focused, { key = gui_unfocus_event, app = app.focused }, 1)
	 end
	 if focused then
	    evt_send(focused, { key = gui_focus_event, app = focused }, 1)
	 end
	 app.focused = focused
	 app.focus_time = 0
      end

      gui_child_autoplacement(app)
      return
   end
   
   if focused then
      if key == gui_focus_event or key == gui_unfocus_event then
	 -- translate and pass down the event to the focused item
--	 print("unfocus", focused.name)
	 evt.app = focused
	 evt_send(focused, evt, 1)
	 return
      end

      if gui_keyconfirm[key] then
	 evt_send(focused, { key = gui_press_event })
	 return
      end

--      if gui_keymenu[key] then
--	 evt_send(focused, { key = gui_menu_event })
--	 return
--      end
      
      if gui_keyup[key] then
	 if gui_new_focus(app, 
			  gui_closest(app, focused.box, 
				      gui_closestcoef_horizontal, gui_less, 4)) then
	    return
	 end
      end
      
      if gui_keydown[key] then
	 if gui_new_focus(app, 
			  gui_closest(app, focused.box, 
				      gui_closestcoef_horizontal, gui_more, 2)) then
	    return
	 end
      end
      
      if gui_keyleft[key] then
	 if gui_new_focus(app, 
			  gui_closest(app, focused.box, 
				      gui_closestcoef_horizontal, gui_less, 3)) then
	    return
	 end
      end
      
      if gui_keyright[key] then
	 if gui_new_focus(app, 
			  gui_closest(app, focused.box, 
				      gui_closestcoef_horizontal, gui_more, 1)) then
	    return
	 end
      end
      
   end
   
   if app.flags.modal then
      print("modal !!")
      return nil
   end
   --	print("dialog unhandle:"..key)
   return evt
end

function gui_dialog_update(app, frametime)
   local focused = app.sub
   if focused and gui_is_focus(app) then
      -- handle focus cursor
      
      -- converge to focused item box
      app.focus_box = app.focus_box + 
	 20 * frametime * (focused.box - app.focus_box)
      
      -- set focus cursor position and color
      dl_set_trans(app.focusup_dl, 
		   mat_scale(app.focus_box[3] - app.focus_box[1], 
			     gui_focus_border_height, 1) *
		      mat_trans(app.focus_box[1],
				app.focus_box[2]-gui_focus_border_height, 0))
      dl_set_trans(app.focusdown_dl, 
		   mat_scale(app.focus_box[3] - app.focus_box[1], 
			     gui_focus_border_height, 1) *
		      mat_trans(app.focus_box[1], app.focus_box[4], 0))
      dl_set_trans(app.focusleft_dl, 
		   mat_scale(gui_focus_border_width, 
			     2*gui_focus_border_height	+ app.focus_box[4]
				- app.focus_box[2], 1)
		      * mat_trans(app.focus_box[1]-gui_focus_border_width,
				  app.focus_box[2]-gui_focus_border_height, 0))
      dl_set_trans(app.focusright_dl, 
		   mat_scale(gui_focus_border_width, 
			     2*gui_focus_border_height + app.focus_box[4] - app.focus_box[2], 1) *
		      mat_trans(app.focus_box[3], app.focus_box[2]-gui_focus_border_height, 0))
      
      app.focus_time = app.focus_time + frametime
      local ci = 0.5+0.5*cos(360*app.focus_time*2)
      
      local focus_dl
      for _, focus_dl in { app.focusup_dl, app.focusdown_dl, app.focusleft_dl, app.focusright_dl } do
	 dl_set_color(focus_dl, ci, 1, ci, ci)
	 dl_set_active(focus_dl, 1)
      end
      
   else
      -- no focus cursor
      if app.focusup_dl then 
	 local focus_dl
	 for _, focus_dl in { app.focusup_dl, app.focusdown_dl, app.focusleft_dl, app.focusright_dl } do
	    dl_set_active(focus_dl, nil)
	 end
      end
   end
   
end

function gui_dialog_box_draw(dl, box, z, bcolor, color)
   
   if not nil then
      local t, l, b, r = 1.6*bcolor, 0.8*bcolor, 0.2*bcolor, 0.4*bcolor
      t[1] = bcolor[1]
      l[1] = bcolor[1]
      b[1] = bcolor[1]
      r[1] = bcolor[1]

      local b3d

      b3d= box3d(box, 4, nil, t, l, b, r)
      box3d_draw(b3d,dl, mat_trans(0, 0, z))

      b3d= box3d(box + { 4, 4, -4, -4 }, 2, color, b, r, t, l)
      box3d_draw(b3d,dl, mat_trans(0, 0, z))
   else
      dl_draw_box(dl, box, z, gui_box_color1, gui_box_color2)
   end
end

function gui_button_box_draw(dl, box, z, bcolor, color)
   if not nil then
      local t, l, b, r = 1.6*bcolor, 0.8*bcolor, 0.2*bcolor, 0.4*bcolor
      t[1] = bcolor[1]
      l[1] = bcolor[1]
      b[1] = bcolor[1]
      r[1] = bcolor[1]
      local b3d = box3d(box, 2, color, t, l, b, r)
      box3d_draw(b3d,dl, mat_trans(0, 0, z))
   else
      dl_draw_box(dl, box, z, gui_button_color1, gui_button_color2)
   end
end

function gui_input_box_draw(dl, box, z, bcolor, color)
   if not nil then
      local t, l, b, r = 1.6*bcolor, 0.8*bcolor, 0.2*bcolor, 0.4*bcolor
      t[1] = bcolor[1]
      l[1] = bcolor[1]
      b[1] = bcolor[1]
      r[1] = bcolor[1]
      local b3d = box3d(box, 2, color, b, r, t, l)
      box3d_draw(b3d,dl, mat_trans(0, 0, z))
   else
      dl_draw_box(app.dl, app.box, z, bcolor, color)
   end
end


function gui_new_dialog(owner, box, z, dlsize, text, mode, name)
   local dial

   --  $$$ ben : default owner is desktop
   if not owner then owner = evt_desktop_app end
   if not owner then print("gui_new_dialog : no desktop") return nil end
   
   z = gui_guess_z(owner, z)
--   print(z)

   if not dlsize then
      dlsize = 10*1024
   end
   dial = { 

      name = name or "gui_dialog",
      version = "0.9",
      
      handle = gui_dialog_handle,
      update = gui_dialog_update,
      
      dl = dl_new_list(dlsize, 1),
      box = box,
      z = z,
      
      focusup_dl = dl_new_list(256, 0),
      focusdown_dl = dl_new_list(256, 0),
      focusleft_dl = dl_new_list(256, 0),
      focusright_dl = dl_new_list(256, 0),
      focus_box = box,
      focus_time = 0,  -- blinking time

      event_table = { },
      flags  = { }
      
   }

   for _, dl in { dial.focusup_dl, dial.focusdown_dl, dial.focusleft_dl, dial.focusright_dl } do
      dl_sublist(dial.dl, dl)
   end
   
   -- draw surrounding box
   gui_dialog_box_draw(dial.dl, box, z, gui_box_color1, gui_box_color1)
   
   -- draw the focus cursor
   local focus_dl
   for _, focus_dl in 
      { dial.focusup_dl, dial.focusdown_dl, 
      dial.focusleft_dl, dial.focusright_dl } do
      dl_draw_box(focus_dl, { 0, 0,  1, 1 }, 
		  z+99, { 1, 1, 1, 1 }, { 1, 1, 1, 1 })
   end
   
   if text then
      mode = mode or { }
      mode.x = mode.x or "left"
      mode.y = mode.y or "upout"
      mode.font_h = mode.font_h or 14
      gui_label(dial, text, mode)
   end
   
   evt_app_insert_first(owner, dial)
   
   -- disconnect joypad for main app
--   cond_connect(nil)
   
   return dial
end



-- button
function gui_button_shutdown(app)
end

function gui_button_handle(app, evt)
   local key = evt.key

   if key == evt_shutdown_event then
      gui_button_shutdown(app)
      return evt -- pass the shutdown event to next app
   end

   local f = app.event_table[key]
   if f then
      return f(app, evt)
   end

   return evt
end

-- warning : owner must be a gui item (we use its dl)
function gui_new_button(owner, box, text, mode, z)
   local app

   z = gui_guess_z(owner, z) + 10
--   print("button", z)

   app = { 

      name = "gui_button",
      version = "0.9",

      handle = gui_button_handle,
      update = gui_button_update,

      dl = owner.dl,
      box = box,
      z = z,

      event_table = { },
      flags = { }

   }

   --dl_draw_box(app.dl, app.box, z, gui_button_color1, gui_button_color2)
   gui_button_box_draw(app.dl, app.box, z, gui_button_color1, gui_button_color1)

   if text then
      gui_label(app, text, mode)
   end

   evt_app_insert_last(owner, app)

   return app
end



--- Input GUI shutdown.
--
function gui_input_shutdown(app)
   if app then
      app.input_dl = nil
   end
end

gui_input_edline_set = {
   [KBD_KEY_HOME] = 1,
   [KBD_KEY_END] = 1,
   [KBD_KEY_LEFT] = 1,
   [KBD_KEY_RIGHT] = 1,
   [KBD_KEY_DEL] = 1,
   [KBD_BACKSPACE] = 1,
}

--- Input GUI event handler.
--
function gui_input_handle(app, evt)
   local key = evt.key

   if key == evt_shutdown_event then
      gui_input_shutdown(app)
      return evt -- pass the shutdown event to next app
   end

   local f = app.event_table[key]
   if f then
      return f(app, evt)
   end
   
   if not app.prev then
      if ((key >= 32 and key < 128) or gui_input_edline_set[key]) then
	 app.input,app.input_col = zed_edline(app.input,app.input_col,key)
	 gui_input_display_text(app)
	 return nil
      elseif gui_keyconfirm[key] then
	 evt_send(app.owner, { key=gui_input_confirm_event, input=app })
	 return nil
      elseif gui_keycancel[key] then
	 gui_input_set(app,strsub(app.input,1,app.input_col-1))
	 return nil
      end
   end

   if key == gui_focus_event and evt.app == app and ke_set_active then
      ke_set_active(1)
      return nil
   end
   if key == gui_unfocus_event and evt.app == app and ke_set_active then
      ke_set_active(nil)
      return nil
   end
   
   return evt
end

function gui_input_display_text(app)
   local w, h = dl_measure_text(app.input_dl, app.input)
   local x = app.input_box[1]
   local y = (app.input_box[2] + app.input_box[4] - h) / 2
   local z = app.z + 1

   dl_clear(app.input_dl)
   dl_draw_text(app.input_dl, x, y, z, gui_text_color, app.input)

   w, h = dl_measure_text(app.input_dl, strsub(app.input, 1, app.input_col-1))
   dl_draw_box(app.input_dl, x+w, y, x+w+2, y+h, z,
	       gui_input_cursor_color1, gui_input_cursor_color2)
   --	print (x+w, y, x+w+2, y+h)
end

-- set the input text
function gui_input_set(app, string, col)
   if not string then
      string = ""
   end
   if not col then
      col = strlen(string)+1
   end
   app.input = string
   app.input_col = col
   gui_input_display_text(app)
end

-- insert text
function gui_input_insert(app, string, col)
   if not string or string == "" then return end
   if col then app.input_col = col end
   local i,len
   len = strlen(string)
   for i=1, len, 1 do
      app.input, app.input_col = zed_edline(app.input, app.input_col,
					    strbyte(string,i))
   end
   gui_input_display_text(app)
end

-- warning : owner must be a gui item (we use its dl)
function gui_new_input(owner, box, text, mode, string, z)
   local app
   
   z = gui_guess_z(owner, z)
   
   app = { 
      
      name = "gui_input",
      version = "0.9",
      
      handle = gui_input_handle,
      update = gui_input_update,
      
      dl = owner.dl,
      box = box,
      input_box = box + { 2, 2, -2, -2 },
      z = z,
      
      event_table = { },
      flags = { },
      
      input_dl = dl_new_list(1024, 1)
      
   }

   dl_sublist(app.dl, app.input_dl)
   
   gui_input_box_draw(app.dl, app.box, z, gui_input_color1, gui_input_color2)
   --dl_draw_box(app.dl, app.box, z, gui_input_color1, gui_input_color2)
   
   gui_input_set(app, string)
   
   if text then
      gui_label(app, text, mode)
   end
   
   evt_app_insert_last(owner, app)
   
   -- if we are the focused widget, then show the keyboard
   if app.sub == app and ke_set_active then
      ke_set_active(1)
   end
   
   return app
end

function gui_text_set(app, text)
   if not app then return end
   dl_clear(app.dl)
   dl_draw_box(app.dl, app.box, z, {0.1, 1, 1, 1} , {0.15, 1, 1, 1})
   if text and strlen(text) > 0 then
      gui_label(app, text, app.mode)
   end
   dl_set_active(app.dl,1)
end

function gui_destroy_text(app)
   if app then
      dl_set_active(app.dl)
   end
end

function gui_new_text(owner, box, text, mode, z)
   local app

   z = gui_guess_z(owner, z)

   app = { 
      name = "gui_text",
      version = "1.0",
      handle = gui_minimal_handle,
      dl = dl_new_list(1024),
      box = box,
      z = z,
      event_table = { },
      flags = { inactive = 1 },
      mode = mode
   }
   gui_text_set(app, text)
   evt_app_insert_last(owner, app)
   return app
end

-- Create a dialog child
function gui_new_children(owner, name, handle, box, mode, z)
   local app
   
   z = gui_guess_z(owner, z)
   
   if not name then
      if owner.name then name = "child_of_"..owner.name
      else name = "gui_child" end
   end
   
   if not handle then
      handle = gui_minimal_handle
   end
   
   app = { 
      name = name,
      handle = handle,
      dl = dl_new_list(1024),
      box = box,
      z = z,
      event_table = { },
      flags = {},
      mode = mode
   }
   evt_app_insert_last(owner, app)
   return app
end

-- display justified text into given box
function gui_justify(dl, box, z, color, text, mode)

   if tag(text) == tt_tag then
      mode = text
   else
      if not mode then
	 mode = { }
      end
      mode.x = mode.x or "center"
      mode.y = mode.y or "center"
      mode.box = box
      mode.z = z
      mode.color = color
      mode = tt_build(text, mode)
   end

   tt_draw(mode)

   dl_sublist(dl, mode.dl)

   return mode.total_w, mode.total_h

end



--- ask a question (given as a tagged text) and propose given answers
--- (array of tagged text), with given optional box width (default to 300) and
--- dialog box label
function gui_ask(question, answers, width, label)

   width = width or 300

   local text = '<dialog guiref="dialog" x="center" name="gui_ask"'

   if label then
      text = text..' label="'..label..'"'
   end
   if width then
      text = text..format(' hint_w="%d"', width)
   end
   text = text..'>'

   text = text..'<vspace h="16">'

   text = text..question

   text = text..'<vspace h="16">'

   local i
   for i=1, getn(answers), 1 do
      text = text..'<hspace w="16">'
      text = text..format('<button guiref="%d">', i)..answers[i]..'</button>'
   end
   text = text..'<hspace w="16">'

   text = text..'<vspace h="8">'

   text = text..'</dialog>'

   print(text)
   local tt = tt_build(text, {
			  x = "center",
			  y = "center",
			  box = { 0, 0, 640, 400 },
		       }
		    )
   
   tt_draw(tt)

   for i=1, getn(answers), 1 do
      tt.guis.dialog.guis[format("%d", i)].event_table[gui_press_event] = function(app, evt)
								 evt_shutdown_app(app.owner)
								 app.owner.answer = %i
								 return evt
							      end
   end

   while not tt.guis.dialog.answer do
      evt_peek()
   end

   return tt.guis.dialog.answer
   
end


--- Ask a question with two possible answers (default is yes/no) with value 1 and 2
function gui_yesno(question, width, label, yes, no)
   yes = yes or "Yes"
   no = no or "No"
   return gui_ask(question, { yes..'<img name="apply" src="stock_button_apply.tga" scale="1.5">', no..'<img name="cancel" src="stock_button_cancel.tga" scale="1.5">' }, width, label)
end


-- add a label to a gui item
function gui_label(app, text, mode)
   --	gui_justify(app.dl, app.box + gui_text_shiftbox, app.z+1, gui_text_color, text, mode)
   return gui_justify(app.dl, app.box, app.z+10, gui_text_color, text, mode)
   -- TODO use an optional mode.boxcolor and render a box around text if it is set ...
end


function gui_shutdown()
   gui_curz = 1000
end

function gui_init()
   gui_shutdown()
   gui_curz = 1000
   gui_press_event		= evt_new_code()
   gui_menu_event		= evt_new_code()
   gui_focus_event		= evt_new_code()
   gui_unfocus_event	= evt_new_code()
   gui_input_confirm_event	= evt_new_code()
   gui_item_confirm_event	= evt_new_code()
   gui_item_cancel_event	= evt_new_code()
   gui_item_change_event	= evt_new_code()
   gui_color_change_event	= evt_new_code() -- arg:color
end

gui_init()

gui_loaded = 1

-- --------------------------------------------------------------------
-- --------------------------------------------------------------------
-- SAMPLE TEST CODE

function dialog_test(parent)
   if not parent then
      parent = evt_desktop_app
   end

   -- create a dialog box with a label outside of the box
   local box = { 100, 100, 400, 400 }
   if parent.box then
      box[1] = box[1] + parent.box[1] + 100
      box[2] = box[2] + parent.box[2]
      box[3] = box[3] + parent.box[1] + 100
      box[4] = box[4] + parent.box[2]
   end
   dial = gui_new_dialog(parent, box, nil, nil,
			 "My dialog box", { x = "left", y = "upout" }, "dialog-test" )
   
   -- add some text inside the dialog box
   gui_label(dial, 
	     [[ <font size="14">
Hello World ! 
Ceci est un tres long texte on purpose !!!!
	     ]], { y="up" } )

   local x = box[1] - 100
   local y = box[2] - 100
   
   -- create a few buttons with labels
   but = gui_new_button(dial, { x + 150, y + 200, x + 240, y + 230 }, 'OK <img name="apply" src="stock_button_apply.tga" scale="1.5">')
   
   -- add a gui_press_event response
   but.event_table[gui_press_event] =
      function(but, evt)
	 print [[OK !!]]
	 evt_shutdown_app(but.owner)
	 return nil -- block the event
      end
   
   but = gui_new_button(dial, { x + 250, y + 200, x + 360, y + 230 }, 'CANCEL <img name="cancel" src="stock_button_cancel.tga" scale="1.5">')
   but.event_table[gui_press_event] =
      function(but, evt)
	 print [[CANCEL !!]]
	 evt_shutdown_app(but.owner)
	 return nil -- block the event
      end
   
   but = gui_new_button(dial, { x + 150, y + 250, x + 240, y + 290 }, 'VMU <img name="vmu" scale="0.25">')
   but.event_table[gui_press_event] =
      function(but, evt)
	 print [[TITI !!]]
	 return nil -- block the event
      end
   
   but = gui_new_button(dial, { x + 250, y + 250, x + 390, y + 290 }, 'DCPlaya <img name="dcplaya" src="dcplaya.tga" scale="1.25">')
   but.event_table[gui_press_event] =
      function(but, evt)
	 print [[TOTO !!]]
	 return nil -- block the event
      end
   
   -- create an input item
   input = gui_new_input(dial, { x + 120, y + 160, x + 380, y + 190 }, "Login :",
			 { x = "left", y="upout" }, "ziggy")


   -- create subdialog
   for i=1, 4, 1 do
      local subdial = gui_new_dialog(dial, { x + 120, y + 330, x + 190, y + 380 }, nil, nil, "Sub dialog", { x = "left", y="upout" })
      but = gui_new_button(subdial, { x + 130, y + 340, x + 180, y + 370 }, "TOTO")
--      but.event_table[gui_press_event] =
--	 function(but, evt)
--	    print [[TOTO !!]]
--	    return nil -- block the event
--	 end
      
      x = x + 20
      y = y + 5
   end

   return dial
end

if nil then
   dial = dialog_test()
   dialog_test(dial)
end


return 1
