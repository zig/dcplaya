--
-- This is main DCplaya lua script
--
-- $Id: dcplayarc.lua,v 1.33 2003-03-12 22:03:24 ben Exp $
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
hideconsole()
dolib ("vmu_init")
if type(ramdisk_init) == "function" then
   ramdisk_init()
end
if type(vmu_init) == "function" then
   vmu_init()
end
showconsole()

-- reading user config
print ("Reading user config file 'userconf.lua'")
dofile (home.."userconf.lua")
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
