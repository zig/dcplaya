--
-- This is main DCplaya lua script
--
-- $Id: dcplayarc.lua,v 1.23 2003-01-14 10:54:50 ben Exp $
--

showconsole()

print ("Welcome to DCplaya !\n")
print (format("Home is set to '%s'", home))

-- standard stuffs
dolib ("basic")
dolib ("evt")
dolib ("dirfunc")
dolib ("shell")
dolib ("zed")
dolib ("keyboard_emu")
dolib ("gui")

--dofile (home.."lua/basic.lua")
--dofile (home.."lua/evt.lua")
--dofile (home.."lua/dirfunc.lua")
--dofile (home.."lua/shell.lua")
--dofile (home.."lua/zed.lua")
--dofile (home.."lua/keyboard_emu.lua")
--dofile (home.."lua/gui.lua")

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

plug_el=home.."plugins/exe/entrylist/entrylist.lez"
plug_jpeg  = home.."plugins/img/jpeg/jpeg.lez"

-- reading user config
print ("Reading user config file 'userconf.lua'")
dofile (home.."userconf.lua")
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

