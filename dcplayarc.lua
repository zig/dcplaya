-- This is main DCplaya lua script
--
-- $Id: dcplayarc.lua,v 1.7 2002-09-18 12:19:30 zig Exp $
--

print ("Welcome to DCplaya !\n")

print ("Home is set to '", home, "'\n")


-- standard stuffs
dofile (home.."lua/dirfunc.lua")
dofile (home.."lua/shell.lua")
dofile (home.."lua/zed.lua")


-- reading directory on PC is slow through serial port, 
-- so we precalculate available plugins instead of doing a dir_load command
plug_spc	= home.."plugins/inp/spc/spc.lef"
plug_xing	= home.."plugins/inp/xing/xing.lef"
plug_sidplay	= home.."plugins/inp/sidplay/sidplay.lef"
plug_obj	= home.."plugins/obj/obj.lef"
plug_lpo	= home.."plugins/vis/lpo/lpo.lef"
plug_fftvlr	= home.."plugins/vis/fftvlr/fftvlr.lef"




-- reading user config
print ("Reading user config file 'userconf.lua'")
dofile (home.."userconf.lua")



-- print available commands
help()
print("")





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
