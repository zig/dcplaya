--
-- event oriented application support
--
-- author : Vincent Penne
--
-- $Id: evt.lua,v 1.27 2004-06-30 15:17:35 vincentp Exp $
--


--
-- An application is just an array containing at least one of two methods :
--
-- update(app, frametime) : method called once per frame
-- handle(app, event)     : method that handle an event and return
--			    a (possibly different) event that will be passed
--                          to next application, or nil to stop the event 
--                          chaining.
--
-- and additionally the following informations
--
-- name                   : the name of the application
-- version                : the major/minor version in string format "MM.mm"
-- sub			  : first element in the sub list of application
-- sublast		  : last element of the sub list of application
-- owner		  : owner application (that is the one that contains
--			    this one in its sub list, i.e. the parent)
-- next			  : next app in the sub list
-- prev			  : prev app in the sub list
--

-- An event is an array containing at least a [key] entry
-- that is a number representing either an ascii character [0 .. 128],
-- either an extended keyboard code, either a joypad code, but also may
-- be equal to some other user defined values. This is the value that
-- identifies the event.
-- For extended keyboard code and joypad code, have a look in 
-- lua/keydefs.lua ...
-- To create a new event code ID, use "evt_new_code()" function that will 
-- return an unused ID

-- In the event chaining process, events are always first sent to the 
-- first element of the sub list before the application itself. 
-- The sub list is typically used
-- for an application that want to create a new dialog box : It inserts
-- the dialog box into its own sublist, the dialog box will then receives
-- events in priority until it is closed (removed from the sub list)

-- The root application list is the "system" list. The last application in this
-- list is the "desktop" application, whose sub list will contains any user
-- applications. This is the standard setup, but this can be changed in
-- any other arbitrary organisation.



dolib("keydefs")
dolib("display_init")
dolib("shell")

-- create a new event code
function evt_new_code()
   if not evt_free_code then
      -- free event code start at KBD_USER value ...
      evt_free_code = KBD_USER
   end

   evt_free_code = evt_free_code + 1
   return evt_free_code - 1
end


-- send an event to an application and its sub applications
function evt_send(app, evt, not_to_child, not_to_owner)
   local i = app.sub
   if not not_to_child and i then
      local n = i.next
      evt = evt_send(i, evt)
      if not evt then
	 return
      end
      i = n
   end

   if not not_to_owner and app.handle then
      evt = app:handle(evt)
      if not evt then
	 return
      end
   end
   
   if not not_to_child then
      while i do
	 local n = i.next
	 evt = evt_send(i, evt)
	 if not evt then
	    return
	 end
	 i = n
      end
   end
   
   return evt
end

-- INTERNAL
function evt_update(app, frametime)

   if app.update then
      app:update(frametime)
   end

   local i = app.sub
   while i do
      local n = i.next
      evt_update(i, frametime)
      i = n
   end
   
end


-- return next event or nil
function evt_peek()
   
   -- basic events
   local key
   local frametime

   repeat
      local com = shell_get_command()
      while com do
	 doshellcommand(com)
	 com = shell_get_command()
      end

      key = evt_origpeekchar()
      
      if key then
--	 vcolor(255, 0, 0)
	 evt = { key = key }
	 evt = evt_send(evt_root_app, evt)
	 if evt then
	    if evt.key == console_key_event then
	       -- translate back the console key
	       evt.key = evt.consolekey
	       evt.consolekey = nil
	       --print(console_key_event, evt.key)
	    end

	    return evt
	 end
--	 vcolor(0, 0, 0)
      else
	 -- do collect garbage once per frame for smoother animation
--	 vcolor(255, 255, 0)
	 collectgarbage(4096)
	 collect(300)
--	 vcolor(0, 0, 0)
	 --collectgarbage()
	 
	 -- calculate frame time
	 evt_curframecounter = evt_origframecounter(1)
	 frametime = frame_to_second(evt_curframecounter)
	 evt_curelapsedtime = frametime
      end
   until not key
   
   -- call update method of applications
--   vcolor(0, 0, 255)
   evt_update(evt_root_app, frametime)
--   vcolor(0, 0, 0)

end

-- peekchar wrapper
function evt_peekchar()
   local evt
   evt = evt_peek()
   if evt then
      return evt.key
   end
end

-- wait for next event and return it
function evt_wait()
   local evt
   repeat
      evt = evt_peek()
   until evt

   return evt
end

-- getchar wrapper
function evt_getchar()
   local evt
   evt = evt_wait()
   return evt.key
end

do

   evt_debug_table_tag = newtag()
   local om = gettagmethod(tag( {} ), "settable")

   local table_settable = function(t, i, v)
			     if i == "handle" and type(v) == "number" then
				local a
				a()
			     else
				rawset(t, i, v)
			     end
			  end

   settagmethod(evt_debug_table_tag, "settable", table_settable)

end


-- add a sub application to an application at the beginning of its sub list
function evt_app_insert_first(parent, app)

   --settag(app, evt_debug_table_tag)

   evt_app_remove(app)
   dlist_insert(parent, "sub", "sublast", app, "prev", "next", "owner")

   evt_send(parent, { key = evt_app_insert_event, app = app })
end

-- add a sub application to an application at the end of its sub list
function evt_app_insert_last(parent, app)

   --settag(app, evt_debug_table_tag)

   evt_app_remove(app)
   dlist_insert(parent, "sublast", "sub", app, "next", "prev", "owner")

   evt_send(parent, { key = evt_app_insert_event, app = app, owner = parent })
end


-- remove a sub application from an application sub list
function evt_app_remove(app)
   local owner = app.owner
   if owner then
      evt_send(owner, { key = evt_app_remove_event, app = app, owner = owner })

      dlist_remove("sub", "sublast", app, "prev", "next", "owner")
   end
end


function evt_framecounter()
   return evt_curframecounter
end

function evt_elapsed_time()
   return evt_curelapsedtime
end

---
--- call this to shutdown an application, send to it the shutdown event
---
--- @warning when the application receive the shutdown event, it should 
---          preserve the next, prev and owner fields so that it can be 
---          removed from the owner's list correctly
---
function evt_shutdown_app(app)
   if type(app) == "table" then 
      evt_send(app, { key = evt_shutdown_event } )
      evt_app_remove(app)
   else
      print("[evt_shutdown_app] : not an application ["..tostring(app).."]")
   end
end

-- create the console application
function evt_create_console_app()
   if console_app then
      evt_shutdown_app(console_app)
      console_app = nil
   end

   
   local app = {
      name = "console",
      version = "0.9",

      handle = function(app, evt)
		  local focused = app == app.owner.sub

		  if focused then
		     if evt.key == shell_toggleconsolekey
			or evt.consolekey == shell_toggleconsolekey then
			evt_app_insert_last(app.owner, app)
			return
		     end

		     if evt.key < KBD_CONT1_C then
			-- translate the console key
			evt.consolekey = evt.key
			evt.key = console_key_event
			return evt
		     end
		  else
		     if evt.key == shell_toggleconsolekey
			or evt.consolekey == shell_toggleconsolekey then
			evt_app_insert_first(app.owner, app)
			return
		     end
		  end

		  if evt.key == evt_shutdown_event
		     or (evt.key == gui_focus_event and evt.app == app) then
		     if ke_set_active then
			ke_set_active(1)
		     end
		     showconsole()

		     if evt.key == evt_shutdown_event then
			console_app = nil
		     end

		     return
		  end
		  if evt.key == gui_unfocus_event and evt.app == app then
		     if ke_set_active then
			ke_set_active(nil)
		     end
		     hideconsole()
		     return
		  end

		  if __DEBUG_EVT then
		     print("console leave, ",evt.key)
		  end

		  return evt

	       end

   }

   console_app = app
   console_key_event = evt_new_code()

   evt_app_insert_first(evt_desktop_app, app)
end


-- shutdown event system
function evt_shutdown()

   if evt_root_app and evt_shutdown_event then
      -- send shutdown event too all application
      evt_send(evt_root_app, { key = evt_shutdown_event } )
   end

   if evt_origgetchar then
      getchar = evt_origgetchar
      peekchar = evt_origpeekchar
      framecounter = evt_origframecounter
   end

   evt_root_app = nil
   evt_desktop_app = nil

   print [[EVT SYSTEM SHUT DOWN]]

end

-- initialize event system
function evt_init()
   evt_shutdown()

   if not check_display_driver or not check_display_driver() then
	  return
   end

   if not evt_origgetchar then
      -- get original getchar and peekchar
      -- the event system will replace these two variables later
      evt_origgetchar = getchar
      evt_origpeekchar = peekchar
      evt_origframecounter = framecounter
   end


   -- create basic event type
   evt_shutdown_event = evt_new_code()
   evt_app_insert_event = evt_new_code()
   evt_app_remove_event = evt_new_code()


   evt_command_queue = { n=0 }

   evt_root_app = {

      name = "root",
      version = "0.9"

   }

   evt_desktop_app = {

      name = "desktop",
      version = "0.9"

   }

   evt_app_insert_first(evt_root_app, evt_desktop_app)

   -- initialize framecounter and elapsed time
   evt_curframecounter = evt_origframecounter(1)
   evt_curelapsedtime = frame_to_second(evt_curframecounter)

   -- last step : replace getchar and cie
   getchar = evt_getchar
   peekchar = evt_peekchar
   framecounter = evt_framecounter

   evt_create_console_app()

   print [[EVT SYSTEM INITIALIZED]]
   return 1
end

function evt_run_standalone(app)
   if type (app) ~= "table" then return end
   local evt
   while app.owner do
      evt = evt_peek()
   end
   return app._result
end

evt_included = 1
evt_loaded = 1

if not evt_init() then return end

-- enhance the desktop application with the application switcher
if not dolib("desktop", 1) then return end

if nil then
   -- some tests
   evt_peek()
   print (evt_desktop_app.owner)
   evt_app_remove(evt_desktop_app)
   evt_app_remove(evt_desktop_app)
   evt_app_insert_first(evt_root_app, evt_desktop_app)
   getchar()
end

return 1
