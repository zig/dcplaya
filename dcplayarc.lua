-- This is main DCplaya lua script
--
-- $Id: dcplayarc.lua,v 1.11 2002-09-20 01:24:33 vincentp Exp $
--

showconsole()

print ("Welcome to DCplaya !\n")

print ("Home is set to '", home, "'\n")


-- standard stuffs
dofile (home.."lua/dirfunc.lua")
dofile (home.."lua/shell.lua")
dofile (home.."lua/zed.lua")


-- reading directory on PC is slow through serial port, 
-- so we precalculate available plugins instead of doing a dir_load command
plug_spc	= home.."plugins/inp/spc/spc.lez"
plug_xing	= home.."plugins/inp/xing/xing.lez"
plug_sidplay	= home.."plugins/inp/sidplay/sidplay.lez"
plug_sc68   = home.."plugins/inp/sc68/sc68.lez"
plug_obj	= home.."plugins/obj/obj.lez"
plug_lpo	= home.."plugins/vis/lpo/lpo.lez"
plug_fftvlr	= home.."plugins/vis/fftvlr/fftvlr.lez"




-- reading user config
print ("Reading user config file 'userconf.lua'")
dofile (home.."userconf.lua")



-- print available commands
help()
print("")



-- launch the enhanced shell
shell()


--
-- Exemple of command to put into userconf.lua :
--
--    driver_load(plug_spc)
--
-- or
--
--    list=dir_load()
--    call(driver_load, list)
--
