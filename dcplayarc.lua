-- This is main DCplaya lua script

print ("Welcome to DCplaya !\n")

print ("Home is set to '", home, "'\n")

-- print available commands
help()
print("")


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


-- Exemple of command to put into userconf.lua :
--
--    driver_load(plug_spc)
