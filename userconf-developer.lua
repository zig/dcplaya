-- This is default userconf.lua for dcplaya developers.
--
-- This file will be copyed if missing as userconf.lua in dcplaya top dir.
--
-- Do NOT modify this file, but feel free to modify userconf.lua as you wish.

if __RELEASE or not __DEBUG then
   print("Executing userconf.lua ... wrapping to userconf-release.lua")
   dofile("userconf-release.lua")
   return
end
