--- @ingroup  dcplaya_lua_files
--- @file     dcplayarc.lua
--- @author   Benjamin Gerard <ben@sashipa.com>
--- @author   Penne Vincent <ziggy@sashipa.com>
--- @date     2002
--- @brief    Main dcplaya lua script.
--- $Id: dcplayarc.lua,v 1.34 2003-03-13 23:03:37 ben Exp $
---
---   The @b home..dcplayarc.lua file is dcplaya main script. It is executed
---   after the dynshell has been loaded.
---
---   This script does much setup stuff. He should load lua library as well
---   as plugin files and does many other things.
---
---   Users have different entry points to this file.
---   -# The @b "/ram/dcplaya/dcplayarc.lua" file should have been extracted
---      from the fist dcplaya file found on your VMU. This file is called
---      really early in that script before any lua library have been loaded.
---      @b `home`, @b `__DEBUG`, @b `__RELEASE` @b `__VERSION` global
---      variables should be defined.
---   -# The @b "/ram/dcplaya/userconf.lua" file is called at near the end
---      of this script if it exists and @b skip_vmu_userconf is not set.
---      User can do what they want here, such as lauching application or
---      setting options. Notice that it could be useful to set the
---      @b skip_home_userconf variable here to prevent the
---      @b home.."userconf.lua" file to overide choices.
---   -# The @b home.."userconf.lua" file is called just after the
---      @b "/ram/dcplaya/userconf.lua" file if @b skip_home_userconf is not
---      set.
---
--- @warning This script or/and scripts called from it need the dynshell to be
---          loaded for proper execution.

--
-- Here is a NON EXHAUSTIVE list of GLOBAL variables that could be set
-- to modify this script and dcplaya behaviours.
--
-- +--------------------------------------------------------------------------+
--      VARIABLE NAME      | DEF |       DESCRIPTION       |       FILES
-- +--------------------------------------------------------------------------+
-- shell_substitut         | nil | Table of type that the  | shell.lua
--                         |     | shell is allowed to     |
--                         |     | substitute.             |
-- +--------------------------------------------------------------------------+
-- vmu_auto_save           | nil | nil for no autosaving.  | control_center.lua
-- +--------------------------------------------------------------------------+
-- vmu_never_confirm_write | nil | Disable VMU write       | vmu_init.lua
--                         |     | confirmation.           |
--                         |     | !!! USE WITH CAUTON     | 
-- +--------------------------------------------------------------------------+
-- vmu_no_default_file     | nil | Disable VMU default     | dynshell.c
--                         |     | file. User should be    |
--                         |     | prompted for a file.    |
--                         |     | !!! NOT TESTED          |
-- +--------------------------------------------------------------------------+
-- skip_vmu_userconf       | nil | Disable the execution   | dcplayarc.lua
--                         |     | ramdisk userconf.lua    |
-- +--------------------------------------------------------------------------+
-- skip_home_userconf      | nil | Disable the execution   | dcplayarc.lua
--                         |     | home userconf.lua       |

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

-- Execute user dcplayarc (extracted from vmu into ramdisk)
if type(test) == "function" and test("-f","/ram/dcplaya/dcplayarc.lua") then
   print("Running [/ram/dcplaya/dcplayarc.lua]")
   dofile("/ram/dcplaya/dcplayarc.lua")
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

-- Standard stuffs
dolib ("basic")
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
	 tex_new(path .. "/" .. files[i])
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
      if type(vmu_init) == "function" then
	 hideconsole()
	 vmu_init()
	 showconsole()
      end
   end
end

-- reading user config
print ("Reading user config file 'userconf.lua'")

if not skip_vmu_userconf and test("-f","/ram/dcplaya/userconf.lua") then
   print("Running [/ram/dcplaya/userconf.lua]")
   dofile ("/ram/dcplaya/userconf.lua")
end

if not skip_home_userconf then
   dofile (home.."userconf.lua")
end

if test("-f","/ram/dcplaya/userconf.lua") then
   print("Running [/ram/dcplaya/userconf.lua]")
   dofile ("/ram/dcplaya/userconf.lua")
end

--
-- Example of command to put into userconf.lua :
--
--    driver_load(home.."plugins/inp/spc/spc.lez")
-- or driver_load(plug_spc)
--
-- or
--
--    list=dir_load()
--    call(driver_load, list)
--

-- Final steps :
help()  -- print available commands
shell() -- launch the enhanced shell
