--- @ingroup  dcplaya_lua_app
--- @file     io_control.lua
--- @author   benjamin gerard <ben@sashipa.com>
--- @date     2003/03/08
--- @brief    IO control application.
---
--- $Id: io_control.lua,v 1.1 2003-03-08 13:53:13 ben Exp $
---

if not dolib ("evt") then return end

--- @defgroup dcplaya_lua_ioctrl_event IO control events
--- @ingroup dcplaya_lua_event
---
---  @par IO control event introduction
---
---   The IO control events are send by the IO control application update
---   function when its detects an IO status change.
---
--- @see dcplaya_lua_ioctrl_app

--
--- CDROM change event structure.
--- @ingroup dcplaya_lua_ioctrl_event
--- @see cdrom_status()
--- struct cdrom_change_event {
---  event_key key;      /**< Always ioctrl_cdrom_event.            */
---  number cdrom_id;    /**< Disk identifier (0 for no disk).      */
---  string cdrom_state; /**< Represents the CDROM status.          */
---  string cdrom_disk;  /**< Ddescribes the type of disk inserted. */
--- };
--

--- @defgroup dcplaya_lua_ioctrl_app IO control application
--- @ingroup dcplaya_lua_app
---
---  @par IO control introduction
---
---   The IO control application is in charge to check IO change such as CDROM
---   or serial. When it detects a change it sends an event to its parent
---   application (usually root).
---
---  @warning Currently only CDROM has been implemented.
---
--- @see cdrom_status()
---

--
--- IO controler structure.
--- @ingroup dcplaya_lua_ioctrl_app
---
--- @warning Only specific fields are listed.
--- 
--- struct ioctrl_app {
---  number cdrom_check_timeout;   /**< Time left before next check.   */
---  number cdrom_check_interval;  /**< Interval between CDROM checks. */
---  number serial_check_timeout;  /**< Idem for serial.               */
---  number serial_check_interval; /**< Idem for serial.               */
--- };
--

--
--- @name IO control functions.
--- @ingroup dcplaya_lua_ioctrl_app
--- @{
--

--- IO control update handler.
---
---    The ioctrl_update() function checks IO status at given interval and
---    sends event if any change has occurred.
---
--- @internal
function ioctrl_update(app, frametime)
   local timeout

   -- cdrom check (if function is available)
   if cdrom_status and app.cdrom_check_timeout then
      local timeout = app.cdrom_check_timeout - frametime
      if timeout <= 0 then
	 timeout = app.cdrom_check_interval
	 local st,ty,id = app.cdrom_state, app.cdrom_disk, app.cdrom_id
	 app.cdrom_state, app.cdrom_disk, app.cdrom_id = cdrom_status(1)

	 -- Send event if any change has occurred
	 if (not st or st ~= app.cdrom_state)
	    or (not ty or ty ~= app.cdrom_disk)
	    or (not id or id ~= app.cdrom_id) then
	    evt_send(app.owner,
		     {
			key = ioctrl_cdrom_event,
			cdrom_state = app.cdrom_state,
			cdrom_disk  = app.cdrom_disk,
			cdrom_id    = app.cdrom_id,
		     })
	 end
      end
      app.cdrom_check_timeout = timeout
   end

end

--- IO control event handler.
--- @internal
function ioctrl_handle(app, evt)
--   printf("ioctrl event : %s %d", (app == ioctrl_app and "ME") or "NOT ME",evt.key)

   if evt.key == evt_shutdown_event then
      print("IO control shutdown!")
      -- Free only if this app is really the current IO controler
      if app == ioctrl_app then
	 print("IO control desactivated")
	 ioctrl_app = nil
      end
      return
   end
   -- continue event chaining
   return evt
end

--- Create IO control application.
--- @see ioctrl_app
--
function io_control()

   -- Only one !
   if ioctrl_app then
      print("IO control already exist : kick it !")
      evt_shutdown_app(ioctrl_app)
      ioctrl_app = nil
   end

   ioctrl_app = {
      -- Application
      name = "IO control",
      version = "0.1",
      handle = ioctrl_handle,
      update = ioctrl_update,

      -- Members
      cdrom_check_timeout = 0,
      serial_check_timeout = 0,
      cdrom_check_interval = 0.5101, -- not multiple of serial (on purpose)
      serial_check_interval = 1,
   }

   -- Create io event
   ioctrl_cdrom_event = ioctrl_cdrom_event or evt_new_code()
   ioctrl_serial_event = ioctrl_serial_event or evt_new_code()
      
   -- insert the application in root list
   evt_app_insert_last(evt_root_app, ioctrl_app)

   print("IO control running")

end

--
--- @}
--

io_control()

return 1
