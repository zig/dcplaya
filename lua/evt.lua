--
-- event oriented application support
--
-- author : Vincent Penne
--
-- $Id: evt.lua,v 1.15 2002-12-15 22:43:08 zigziggy Exp $
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

   --	if not evt then
   --		print (format("[H] [%s]", name))
   --	end

   local i = app.sub
   if not not_to_child and i then
      local n = i.next
      evt = evt_send(i, evt)
      if not evt then
	 return
      end
      i = n
   end

   -- $$$
   --	local name = "unknown"
   --	if app.name then name = app.name end
   --	print (format("[S] [%d] [%s]",evt.key, name))

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
   
   --	print (format("[R] [%d] [%s]", evt.key, name))
   
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
      key = evt_origpeekchar()
      
      if key then
	 vcolor(255, 0, 0)
	 evt = { key = key }
	 evt = evt_send(evt_root_app, evt)
	 if evt then
	    return evt
	 end
	 vcolor(0, 0, 0)
      else
	 -- do collect garbage once per frame for smoother animation
	 collectgarbage()
	 
	 -- calculate frame time
	 evt_curframecounter = evt_origframecounter(1)
	 frametime = evt_curframecounter/60
      end
   until not key
   
   -- call update method of applications
   vcolor(100, 0, 0)
   evt_update(evt_root_app, frametime)
   vcolor(0, 0, 0)

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


---
--- call this to shutdown an application, send to it the shutdown event
---
--- @warning when the application receive the shutdown event, it should 
---          preserve the next, prev and owner fields so that it can be 
---          removed from the owner's list correctly
---
function evt_shutdown_app(app)
   evt_send(app, { key = evt_shutdown_event } )
   evt_app_remove(app)
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

   -- initialize framecounter
   evt_curframecounter = evt_origframecounter(1)

   -- last step : replace getchar and cie
   getchar = evt_getchar
   peekchar = evt_peekchar
   framecounter = evt_framecounter

   print [[EVT SYSTEM INITIALIZED]]

   return 1

end

evt_init()

evt_included = 1
evt_loaded = 1

if nil then

   -- some tests

   evt_peek()


   print (evt_desktop_app.owner)
   evt_app_remove(evt_desktop_app)
   evt_app_remove(evt_desktop_app)
   evt_app_insert_first(evt_root_app, evt_desktop_app)


   getchar()

end
