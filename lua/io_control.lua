--- @ingroup  dcplaya_lua_app
--- @file     io_control.lua
--- @author   benjamin gerard <ben@sashipa.com>
--- @date     2003/03/08
--- @brief    IO control application.
---
--- $Id: io_control.lua,v 1.4 2003-03-17 03:31:21 ben Exp $
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

--
--- RAM change event structure.
--- @ingroup dcplaya_lua_ioctrl_event
--- @see ramdisk_is_modified()
--- @see ramdisk_notify_path()
--- struct ramdisk_change_event {
---  event_key key;      /**< Always ioctrl_ramdisk_event.          */
--- };
--

--
--- Keyboard change event structure.
--- @ingroup dcplaya_lua_ioctrl_event
--- @see keyboard_present()
--- struct keyboard_change_event {
---  event_key key;      /**< Always ioctrl_keyboard_event.          */
---  boolean present;    /**< Set if keyboard is present             */
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
---  @warning Currently only CDROM and RAMDISK has been implemented.
---
--- @see cdrom_status()
--- @see ramdisk_is_modified()
--- @see ramdisk_notify_path()
---

--
--- IO controler structure.
--- @ingroup dcplaya_lua_ioctrl_app
---
--- @warning Only specific fields are listed.
--- 
--- struct ioctrl_app {
---  number cdrom_check_timeout;     /**< Time left before next check.   */
---  number cdrom_check_interval;    /**< Interval between CDROM checks. */
---  number serial_check_timeout;    /**< Idem for serial.               */
---  number serial_check_interval;   /**< Idem for serial.               */
---  number ramdisk_check_timeout;   /**< Idem for ramdisk.              */
---  number ramdisk_check_interval;  /**< Idem for ramdisk.              */
---  number keyboard_check_timeout;  /**< Idem for keyboard.             */
---  number keyboard_check_interval; /**< Idem for keyboard.             */
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

   -- ramdisk check (if function is available)
   if ramdisk_is_modified and app.ramdisk_check_timeout then
      local timeout = app.ramdisk_check_timeout - frametime
      if timeout <= 0 then
	 timeout = app.ramdisk_check_interval
	 if ramdisk_is_modified() then
	    evt_send(app.owner,
		     {
			key = ioctrl_ramdisk_event,
		     })
	 end
      end
      app.ramdisk_check_timeout = timeout
   end

   -- keyboard check (if function is available)
   if keyboard_present and app.keyboard_check_timeout then
      local timeout = app.keyboard_check_timeout - frametime
      if timeout <= 0 then
	 timeout = app.keyboard_check_interval
	 local present, change = keyboard_present()
	 if change and ke_app then
	    evt_send(ke_app,
		     {
			key = ioctrl_keyboard_event,
			present = present == 1
		     })
	 end
      end
      app.keyboard_check_timeout = timeout
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
   io_control_kill()

   ioctrl_app = {
      -- Application
      name = "IO control",
      version = "0.1",
      handle = ioctrl_handle,
      update = ioctrl_update,

      -- Members
      cdrom_check_timeout = 0,
      serial_check_timeout = 0,
      ramdisk_check_timeout = 0,
      keyboard_check_timeout = 0,

      cdrom_check_interval = 0.5103, -- not multiple (on purpose)
      serial_check_interval = 1,
      ramdisk_check_interval = 3.007,
      keyboard_check_interval = 4.017,
   }

   -- Create io event
   ioctrl_cdrom_event = ioctrl_cdrom_event or evt_new_code()
   ioctrl_serial_event = ioctrl_serial_event or evt_new_code()
   ioctrl_ramdisk_event = ioctrl_ramdisk_event or evt_new_code()
   ioctrl_keyboard_event = ioctrl_keyboard_event or evt_new_code()
      
   -- insert the application in root list
   evt_app_insert_last(evt_root_app, ioctrl_app)

   print("IO control running")
end

--
--- Kill an io-control application.
---
---   The io_control_kill() function kills the given application by
---   calling sending the evt_shutdown_app() function. If the given
---   application is nil or ioctrl_app the default io-control
---   (ioctrl_app) is killed and the global variable ioctrl_app is
---   set to nil.
---
--- @param  io  application to kill (default to ioctrl_app)
--
function io_control_kill(io)
   io = io or ioctrl_app
   if io then
      evt_shutdown_app(io)
      if io == ioctrl_app then
	 ioctrl_app = nil
      end
   end
end

--
--- @}
--

io_control_kill()
io_control()

return 1
