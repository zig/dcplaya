--- @ingroup  dcplaya_lua_files
--- @file     dcplayarc.lua
--- @author   benjamin gerard <ben@sashipa.com>
--- @author   penne vincent <ziggy@sashipa.com>
--- @date     2002
--- @brief    Main dcplaya lua script.
---
--- $Id: dcplayarc.lua,v 1.40 2003-03-25 09:28:00 ben Exp $
---
---   The @b home.."dcplayarc.lua" file is dcplaya main script.
---   It is executed after the dynshell has been loaded.
---   This script setup much stuff like loading dcplaya lua library as well
---   as plugin files and more ressources.
---
--- @par Roadmap:
---
---   Order things have to be done :
---
---   -# The @b "/ram/dcplaya/dcplayarc.lua" file should have been extracted
---      from the fist dcplaya file found on your VMU. This file is called
---      really early in that script before any lua library have been loaded.
---      @b `home`, @b `__DEBUG`, @b `__RELEASE` @b `__VERSION` global
---      variables should be defined.
---   -# The @b home.."userconf.lua" file is called after all standard
---      initialization have been done (lua library loading...).
---      Setting the global variable skip_home_userconf prevents this script
---      to be launched.
---   -# The @b "/ram/dcplaya/userconf.lua" file is called just after
---      @b home.."userconf.lua". Since it is call near the end and 
---      after all other config files and it is a local file, this
---      file defines the user configuration. It could be use to patch
---      dcplaya standard behaviours.
---      Setting the global variable skip_vmu_userconf prevents this script
---      from launching.
---
--- @par Useful variables:
---
--- Here is a @b non @b exhaustive list of @b global @b variables that could
--- be set to modify this script and dcplaya behaviours :
---
---  - shell_substitut [nil] [Table of type that the shell is allowed to
---    substitute] [shell.lua]
---  - vmu_auto_save [nil] [nil for no autosaving.] [control_center.lua]
---  - vmu_never_confirm_write [nil] [Disable VMU write confirmation.
---    Use with @b CAUTION] [vmu_init.lua]
---  - vmu_no_default_file [nil] [Disable VMU default file.
---    User should beprompted for a file. @b Not @b TESTED] [dynshell.c]
---  - skip_vmu_userconf [nil] [Disable the execution of the ramdisk
---    userconf.lua] [dcplayarc.lua]
---  - skip_home_userconf Disable the execution home userconf.lua
---    [dcplayarc.lua]
---
--- @warning This script or/and scripts called assumes the dynshell to be
---          loaded for proper execution.
---
--

showconsole()

-- Display some welcome text
print ("dcplaya version : " .. (__VERSION or "unknown"))
if (__DEBUG) then print ("dcplaya debug level: " .. __DEBUG) end
if (__RELEASE) then print ("dcplaya release mode activated") end
print ("Welcome to dcplaya !\n")
print (format("Home is set to '%s'", home))

-- reading directory on PC is slow through serial port, 
-- so we precalculate available plugins instead of doing a dir_load command
plug_spc	= home.."plugins/inp/spc/spc.lez"
plug_xing	= home.."plugins/inp/xing/xing.lez"
plug_sidplay	= home.."plugins/inp/sidplay/sidplay.lez"
plug_entrylist	= home.."plugins/exe/entrylist/entrylist.lez"
plug_el		= home.."plugins/exe/entrylist/entrylist.lez"
plug_sc68	= home.."plugins/inp/sc68/sc68.lez"
plug_mikmod	= home.."plugins/inp/mikmod/mikmod.lez"
plug_ogg	= home.."plugins/inp/ogg/ogg.lez"
plug_cdda	= home.."plugins/inp/cdda/cdda.lez"
plug_obj	= home.."plugins/obj/obj.lez"
plug_lpo	= home.."plugins/vis/lpo/lpo.lez"
plug_fftvlr	= home.."plugins/vis/fftvlr/fftvlr.lez"
plug_hyperpipe	= home.."plugins/vis/hyperpipe/hyperpipe.lez"
plug_fime	= home.."plugins/vis/fime/fime.lez"
plug_el         = home.."plugins/exe/entrylist/entrylist.lez"
plug_jpeg       = home.."plugins/img/jpeg/jpeg.lez"

-- Little function for fun !
-- $$$ should be move ...

local scroll_dl = nil

function scrolltext(dl, msg, txtcolor, bkgcolor, z)
   if type(msg) ~= "string" then return end
   if type(dl_new_list) ~= "function" then
      return
   end
   if tag(dl) ~= dl_tag then
      dl = dl_new_list(strlen(msg) + 512)
   end
   dl_clear(dl)
   if tag(dl) ~= dl_tag then
      return
   end
   txtcolor = txtcolor or {1, 1, 0, 0}
   bkgcolor = bkgcolor or {0.8,0.2,0.2,0.4}
   z = z or 400

   local size = 16

   dl_text_prop(dl,0,size)
   local w,h = dl_measure_text(dl, msg, 1 ,size)
   w = w * 1.5
   local x,y = (640-w) * 0.5, (480-h) * 0.5
   dl_set_clipping(dl, x, y-3, x+w, y+h+3)

   local c = bkgcolor
   dl_draw_box1(dl, x, y,x+w, y+h+3, z-10,
		c[1],c[2],c[3],c[4])

   c = txtcolor
   dl_draw_scroll_text(dl, x, y, z,
		       c[1],c[2],c[3],c[4], msg,
		       w, 1, 1)
   dl_set_active(dl,1)
   return dl
end


-- Execute user dcplayarc (extracted from vmu into ramdisk)
if not dcplayarc_vmu_loading and
   type(test) == "function" and test("-f","/ram/dcplaya/dcplayarc.lua") then

   -- to avoid infinite loop
   dcplayarc_vmu_loading = 1

   print("Running [/ram/dcplaya/dcplayarc.lua]")
   dofile("/ram/dcplaya/dcplayarc.lua")

   return
end

-- Add filetype some useful filetypes.
if type(filetype_add) == "function" then
   filetype_add("lua")
   filetype_add("lua", nil, ".lua\0")
   filetype_add("text")
   filetype_add("text", nil, ".txt\0.doc")
end

-- Load this driver as soon as possible
if type(test) == "function" and type(driver_load) == "function" then
   if type(plug_jpeg) == "string" and test("-f",plug_jpeg) then
      driver_load(plug_jpeg)
   end
   if type(plug_el) == "string" and test("-f",plug_el) then
      driver_load(plug_el)
   end
end

-- Standard libraries
dolib ("basic")
dolib("display_init")
if type(scrolltext) == "function" then
   scroll_dl = scrolltext(scroll_dl,
			  "Welcome to dcplaya " .. __VERSION
			     .. ". Please wait while loading resources ...")
end

dolib ("evt")
dolib ("dirfunc")
dolib ("shell")
dolib ("zed")
dolib ("keyboard_emu")

dolib ("sprite")

-- Loading sprite ressource
if __RELEASE and type(dirlist) == "function" then
   local path = home.."lua/rsc/icons"
   printf("Loading icon resources from %q",path)
   local dir,files = dirlist("-h2",path)
   if type(files) == "table" then
      local n,i = getn(files)
      for i=1,n do
	 if not tex_exist(files[i]) then
	    tex_new(path .. "/" .. files[i])
	 end
      end
   end
end

dolib ("io_control")
dolib ("gui")

-- vmu initialisation.
dolib ("vmu_init")

if type(ramdisk_init) == "function" then
   ramdisk_init()
end

if type(vmu_file) == "function" then
   local defvmu = vmu_file()
   if defvmu then
      printf("Default VMU file is %q.",defvmu)
   else
      print("No default VMU file.")
      local result
      if type(vmu_init) == "function" then
	 hideconsole()
	 -- Disable scrolltext ...
	 if scroll_dl then
	    dl_set_ative(scroll_dl,nil)
	 end
	 result = vmu_init(1) -- Select only 
	 -- Enable scrolltext ...
	 if scroll_dl then
	    dl_set_ative(scroll_dl,1)
	 end
	 showconsole()
      end
   end
end

-- reading user config
print ("Reading user config files ...")

if not skip_home_userconf then
   local configfile = home .. "userconf.lua"
   printf("Running [%s]",configfile)
   dofile (configfile)
end

if not skip_vmu_userconf and test("-f","/ram/dcplaya/userconf.lua") then
   print("Running [/ram/dcplaya/userconf.lua]")
   dofile ("/ram/dcplaya/userconf.lua")
end

scroll_dl = nil

-- Final steps :
help()  -- print available commands
shell() -- launch the enhanced shell
hideconsole()
