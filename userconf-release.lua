--- @ingroup  dcplaya_lua_files
--- @file     userconf.lua
--- @date     2003/03/23
---
--- @author   benjamin gerard
--- @author   penne vincent
---
--- @brief    dcplaya lua user script.
---
--- @par userconf.lua
---
---   There is two version of this file. The first version should be in dcplaya
---   home directory, the second should have been extracted from the VMU
---   into the ramdisk "/ram/dcplaya/". These two versions are optionnal files.
---   They are executed by the dcplayarc.lua in this order. By the way their
---   execution can be skip by setting respectively skip_home_userconf and
---   skip_vmu_userconf variables. Since the home userconf is executed before
---   the VMU one, the only possiblity to set these variables is to do it in
---   the VMU dcplayarc.lua. This mechanism should be enought for any user to
---   configure its dcplaya at will (or so I hope).
---   Having an home userconf.lua file instead of doing all initialization
---   stuff in dcplayarc.lua is a development facility purpose. Each
---   dcplaya developper can have its own userconf.lua depending on its
---   need and behaviour. Because this file is subject to a lot of
---   modifications it will be a real nightmare to include it to the CVS
---   repository.
---
--- @par Default behaviours
---
---   The default userconf.lua file do the following things in this order
---   only if the __RELEASE variable is set:
---     -# Loading almost all drivers (plugins) in suitable order.
---     -# Check and load if missing the entry_list driver that should
---        have been loaded by the dcplayarc.lua
---     -# Check and load if missing the jpeg driver that should
---        have been loaded by the dcplayarc.lua
---     -# Load some dcplaya lua libraries with dolib() function. This will
---        start all standard applications neccessary for most users:
---          -# @link dcplaya_lua_background background @endlink,
---             load default background.
---          -# @link dcplaya_lua_cc_app control_center @endlink,
---             launch control-center application
---          -# @link dcplaya_lua_si_app song_info @endlink,
---             launch song-info  application
---          -# @link dcplaya_lua_sb_app song_browser @endlink,
---             launch and get focus on the song-browser application.
---     -# Randomly set a vmu visual.
---     -# Randomly set a visual plugin if none is set.
---
--- @par Content
--- @code

print()
print("Executing "..home.."userconf.lua")
print(" This file : @BUILT-DATE@")
print(" Release   : " .. (__RELEASE and "yes" or "no"))
print(" Debug     : " .. (__DEBUG and "yes" or "no"))
print()

if __RELEASE then
   local msg = "Loading drivers ... please wait"
   print(msg)

   local plugins

   if SHADOCK_EDITION then
      plugins = {
	 plug_obj,
	 plug_cdda, plug_ffmpeg, 
	 plug_ogg, plug_mikmod, plug_sidplay, 
	 plug_sc68, plug_nsf,
	 plug_lpo, plug_fftvlr, plug_hyperpipe, plug_fime
      }
   else
      plugins = {
	 plug_obj,
	 plug_cdda, plug_ffmpeg, plug_ogg, plug_mikmod, plug_sidplay, 
	 plug_sc68, plug_lpo, plug_fftvlr, plug_hyperpipe, plug_fime, 
	 plug_spc
      }
   end

   -- Add entrylist driver if not already loaded.
   if type(entrylist_load) ~= "function" then
      tinsert(plugins,plug_el)
   end
   -- Try to get filetype of a fake jpeg file will tell us if the jpeg driver
   -- is already loaded.
   local ty,ma,mi = filetype("t.jpg")
   if not ma or ma ~= "image" then
      tinsert(plugins,plug_jpeg)
   end

   local i,v
   for i,v in plugins do
      if type(v) == "string" then
	 print(format(" '%s'", v))
	 driver_load(v)
      end
   end

   -- Really start dcplaya applications now !
   dolib("background")

   dolib("control_center")
   dolib("song_info")
   dolib("song_browser")
   dolib("fifo_tracker")

   if fifo_tracker_create then
      fifo_tracker_create()
   end

   -- Currently vmu visual are not plugins, just hard the list
   vmu_set_visual(random(3))

   -- Set visual only if no cuurent
   local vis_name = set_visual()
   if not SHADOCK_EDITION and not vis_name or vis_name == "" then
      vis_name = nil
      -- Get driver lists
      local drlist = get_driver_lists()
      if type(drlist) == "table" and drlist.vis and drlist.vis.n > 0 then
	 -- We add some visual driver, set a random one.
	 vis_name = drlist.vis[random(drlist.vis.n)].name
	 if vis_name then
	    print("Setting visual " .. vis_name)
	    set_visual(vis_name)
	 end
      end
   end
   -- test again
   -- vis_name = set_visual() 
   -- $$$ failed ? Probably becoz visual really be set in the next frame ...
   if not SHADOCK_EDITION then
      if not vis_name then
	 print("No visual plugin found :`(")
      else
	 print("Visual plugin " .. vis_name)
      end
   end

   if scroll_dl then
      dl_set_active(scroll_dl,nil)
   end
   hc()
   dolib("net")
   sc()
   if scroll_dl then
      dl_set_active(scroll_dl,1)
   end


   if SHADOCK_EDITION then
      print("Shadock edition")
      print("Loading ffmpeg codecs ...")
      cl(codec_misc)
      print("Playing Shadock video ...")
      playa_play("/cd/extra/shadock.avi")

      -- TO BE REMOVED !!
--      function t()
--	 dofile "/pc/home/zig/Dev/dc/newdcp/dcplaya/lua/taggedtext.lua"
--	 dofile "/pc/home/zig/Dev/dc/newdcp/dcplaya/lua/net.lua"
--      end
   end

   hc()

end

---@endcode