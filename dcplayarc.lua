--
-- This is main DCplaya lua script
--
-- $Id: dcplayarc.lua,v 1.31 2003-03-08 18:30:44 ben Exp $
--

showconsole()

-- Display some welcome text
print ("dcplaya version : " .. (__VERSION or "unknown"))
if (__DEBUG) then print ("dcplaya debug level: " .. __DEBUG) end
if (__RELEASE) then print ("dcplaya release mode activated") end

print ("Welcome to dcplaya !\n")
print (format("Home is set to '%s'", home))

-- standard stuffs
dolib ("basic")
dolib ("evt")
dolib ("dirfunc")
dolib ("shell")
dolib ("zed")
dolib ("keyboard_emu")
dolib ("io_control")
dolib ("gui")

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

-- vmu initialisation.
hideconsole()
dl(plug_jpeg)
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
   print("Executing local 'userconf.lua'")
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

-- FINAL STEPS :
-- print available commands
help()
-- launch the enhanced shell
shell()
