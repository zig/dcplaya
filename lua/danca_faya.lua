--- @ingroup  dcplaya_lua_danca_faya_app
--- @file     danca_faya.lua
--- @author   benjamin gerard
--- @date     2003/09/13
--- @brief    Dance Dance Revolution clone application.
---
--- $Id: danca_faya.lua,v 1.1 2003-09-15 21:41:20 benjihan Exp $
---

--- @defgroup dcplaya_lua_danca_faya_app Danca Faya
--- @ingroup  dcplaya_lua_app
--- @brief    Dance Dance Revolution clone application
---
---  @par danca-faya introduction
---
---   danca-faya application is an game application 
---   based on the concept of dancing game like the famous
---   Dance Dance Revolution.
---
--- @author   benjamin gerard
---
--- @{
---

if not dolib("gui") then return end
if not dolib("sprite") then return end
if not dolib("box3d") then return end

--------------------------------------------------------
function create_pipo_pattern(n)
   local i
   local pat = {}

   n = n or (random(16) + 16)
   for i=1,n do
      pat[i] = random(32) - 1
      if pat[i] > 15 then pat[i] = 0 end
   end
   return pat
end

function create_pipo_song(n,npat)
   local i
   local song = {}
   n = n or random(6) + 4
   for i=1,n do
      song[i] = random(npat)
   end
   return song
end

function create_pipo_ddr()
   local ddr = {}

   ddr.beat = 256
   ddr.factor = -2
   ddr.patterns = {}

   local n = random(4) + 1
   for i=1,n do
      ddr.patterns[i] = create_pipo_pattern()
   end

   ddr.positions = create_pipo_song(nil,n)

   printf("Pipo DDR: %d pos, %d pat, beat:%d factor:%d",
	  getn(ddr.positions), getn(ddr.patterns), ddr.beat, ddr.factor)

   return ddr
end

--------------------------------------------------------

function df_build_pattern(ddr, dfsong, song, num)
   local factor = dfsong.factor
   local spr = ddr.arrow_spr
   local h = spr.h
   local tbl_bit = { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4 }
   local pat = song.patterns[num]
   local dfpat = {}
   local zmax = 30

   -- Count number of sprite to draw for this pattern
   local cnt,n,i = 0,getn(pat)
   for i=1,n do
      cnt = cnt + tbl_bit[pat[i]+1]
   end

   -- Try to alloc optimized list
   local size_of_sprite = (6 * 4) + (4 * 4 * 10)
   local dl = dl_new_list(512 + cnt * size_of_sprite,1,1)

   printf("danca-faya: pat:#%d nspr:%d", num, cnt)

   -- Display pattern number
   dl_draw_text(dl, -32,0,zmax, 1,1,1,0, format("%03d",num))

   local colors = {
      { 1.0, 0.5, 1.0, 0.5 },
      { 1.0, 0.5, 0.7, 1.0 },
      { 1.0, 1.0, 0.5, 0.5 },
      { 1.0, 1.0, 1.0, 0.5 },
   }

   -- Build pattern display list
   local mat = mat_trans(-spr.w*0.5, -spr.h*0.5, 0)
   local rmat = mat_new()
   rmat[1][1] = 0
   rmat[1][2] = 1
   rmat[2][1] = -1
   rmat[2][2] = 0
   for i=1,n do
      local v,y,j = pat[i], spr.h*i*factor
      for j=1,4 do
	 if intop(v,'&',1) ~= 0 then
	    local c = colors[j]
	    spr:set_color(c[1],c[2],c[3],c[4])
	    spr:draw(dl, mat * mat_trans((j-1)*64,y,(i-1)*zmax/n))
	 end
	 v = intop(v,'>',1)
	 mat_mult_self(mat,rmat)
      end
   end
   spr:set_color(1,1,1,1)

   dfsong.patterns[num] = {
      dl=dl,
      beats=n*dfsong.beat,
      pattern=pat
   }

   return 1 
end

function df_build_position(ddr, dfsong, song, i)
   local beat = 0
   if i > 1 then
      local prevpos=dfsong.positions[i-1]
      beat = prevpos.beat + prevpos.pattern.beats
   end

   local pattern = dfsong.patterns[song.positions[i]]

   dfsong.positions[i] = {
      beat=beat,
      pattern = dfsong.patterns[song.positions[i]]
   }
   printf("danca-faya: pos #%02d beat:%04d-%04d",
	  i,beat,beat+dfsong.positions[i].pattern.beats)

   return 1
end

function df_check_song(song)

   if type(song) ~= "table" then
      printf("danca-faya: not a song")
      return
   end

   if type(song.beat) ~= "number" then
      printf("danca-faya: missing beat duration")
      return
   end

   if type(song.factor) ~= "number" then
      printf("danca-faya: missing beat factor")
      return
   end

   if type(song.patterns) ~= "table" or getn(song.patterns) < 1 then
      printf("danca-faya: missing patterns")
      return
   end

   if type(song.positions) ~= "table" or getn(song.positions) < 1 then
      printf("danca-faya: missing positions")
      return
   end

   song.nb_pat = getn(song.patterns)
   song.nb_pos = getn(song.positions)

   local i
   for i=1,song.nb_pos do
      local pat = song.positions[i]
      if pat < 1 or pat > song.nb_pat then
	 printf("danca-faya: pattern at position #%d (%d) out of range",i,pat)
	 return
      end
   end

   print("danca-faya: check song OK")

   return 1
end

function df_prepare_song(ddr, song)
   local dfsong

   if not df_check_song(song) then
      return
   end

   dfsong = {}

   -- beat duration in 1024th of second
   dfsong.beat = song.beat
   -- display subdivision ratio (1 is the height of arrow sprite)
   dfsong.factor = song.factor
   if dfsong.factor < 0 then dfsong.factor = -1/dfsong.factor end

   dfsong.nb_pos = song.nb_pos
   dfsong.nb_pat = song.nb_pat
   dfsong.positions = {}
   dfsong.patterns  = {}

   -- Setup display parameters
   dfsong.y_per_beat = ddr.arrow_spr.h * dfsong.factor / dfsong.beat

   printf("danca-faya : y per beat=%.3f",dfsong.y_per_beat)

   -- Build patterns
   for i=1, dfsong.nb_pat do
      if not df_build_pattern(ddr, dfsong, song, i) then return end
   end

   -- Build song
   local start_beat = 0
   for i=1, dfsong.nb_pos do
      if not df_build_position(ddr, dfsong, song, i) then return end
   end

   ddr.dfsong = dfsong;
   return 1
end


function df_start_song(ddr)
   print("danca-faya: START")
   if type(ddr.dfsong) == "table" then
      ddr.dfsong.cur_pos = 0
   end
end

function df_stop_song(ddr)
   print("danca-faya: STOP")
   dl_set_active(ddr.pat_display[1].dl,0)
   dl_set_active(ddr.pat_display[2].dl,0)
   if type(ddr.dfsong) == "table" then
      ddr.dfsong.cur_pos = nil 
   end
end

function df_kill_song(ddr)
   df_stop_song(ddr)
   ddr.dfsong = nil
end


function danca_faya_update(ddr, frametime)

   -- Check if there is a song loaded
   local dfsong = ddr.dfsong
   if not dfsong then return end
   -- Check if it is playing
   local oldpos = dfsong.cur_pos
   if not oldpos then return end

   local curbeat, deltabeat, pat, pos
   if oldpos == 0 then
      -- restarting
      curbeat = 0
      deltabeat = 0
      curpos  = 1
      ddr.phy_display = ddr.pat_display[1]
      ddr.log_display = ddr.pat_display[2]
      dfsong.display_pat = { }
   else
      -- stepping
      curbeat = dfsong.cur_beat + frametime * 1024 
      curpos = oldpos
      repeat
	 pos = dfsong.positions[curpos]
	 pat = pos.pattern

	 deltabeat = curbeat - pos.beat
	 if deltabeat < 0 then
	    curpos = curpos - 1;
-- 	    printf("danca-faya: rewind to %d", curpos)
	 elseif deltabeat >= pat.beats then
	    curpos = curpos + 1
-- 	    printf("danca-faya: seek to %d", curpos)
	 else
	    break
	 end
      until curpos < 1 or curpos > dfsong.nb_pos
   end

   local ypb,y = dfsong.y_per_beat
   local phydisp, logdisp = ddr.phy_display, ddr.log_display

   if curpos ~= oldpos then
--       printf("Change position (%d->%d)", oldpos, curpos)
      if curpos < 1 or curpos > dfsong.nb_pos then
	 printf("This is the end")
	 df_stop_song(ddr)
	 return
      end

      dl_set_active(logdisp.dl, 0)
      dl_set_active(logdisp.dl0,0)
      dl_set_active(logdisp.dl1,1)
      dl_set_active(logdisp.dl2,0)

      if curpos > 1 then
	 pat = dfsong.positions[curpos-1].pattern
	 dl_clear(logdisp.dl0)
	 dl_sublist(logdisp.dl0, pat.dl)
	 dl_set_trans(logdisp.dl0, mat_trans(0,-pat.beats*ypb,1))
	 dl_set_active(logdisp.dl0,1)
      end

      pat = dfsong.positions[curpos].pattern
      dl_clear(logdisp.dl1)
      dl_sublist(logdisp.dl1, pat.dl)
      dl_set_trans(logdisp.dl1, mat_trans(0,0,31))
      y = pat.beats * ypb

      if curpos < dfsong.nb_pos then
	 pat = dfsong.positions[curpos+1].pattern
	 dl_clear(logdisp.dl2)
	 dl_sublist(logdisp.dl2, pat.dl)
	 dl_set_trans(logdisp.dl2, mat_trans(0,y,61))
	 dl_set_active(logdisp.dl2,1)
      end

      phydisp,logdisp = logdisp,phydisp
   end

   local y = deltabeat * dfsong.y_per_beat
   dl_set_trans(phydisp.dl, mat_trans(0,-y,0))
   dl_set_active2(phydisp.dl, logdisp.dl, 1)

   dfsong.cur_beat = curbeat
   dfsong.cur_pos  = curpos
   ddr.phy_display = phydisp
   ddr.log_display = logdisp
end


function danca_faya_handle(ddr, evt)
   -- call the standard dialog handle (manage child autoplacement)
   evt = gui_dialog_basic_handle(ddr, evt)
   if not evt then
      return
   end

   local key = evt.key
   if key == evt_shutdown_event then
      ddr:shutdown()
      return evt
   elseif gui_keyconfirm[key] then
      df_start_song(ddr)
      return
   elseif gui_keycancel[key] then
      df_stop_song(ddr)
      return
   end
   
   return evt
end

--
--- Danca-Faya shutdown.
--- @internal
--- @warning Do not used directly. To kill a danca-faya application
---  use danca_faya::kill() or danca_faya_kill().
--
function danca_faya_shutdown(ddr)
   if not ddr then return end

   dl_set_active(ddr.dl, nil)
   ddr.dl = nil
   ddr.pat_display = nil
end

--
--- Create a danca-faya application.
---
--- @param  owner  Owner application (default to desktop).
--- @param  name   Name of application (default to "danca faya").
---
--- @return  application
--- @retval nil Error
--
function danca_faya_create(owner, name)
   -- Default parameters
   owner = owner or evt_desktop_app
   name = name or "danca faya"

   local ddr = {
      -- Application
      name = name,
      version = 1.0,
      handle = danca_faya_handle,
      update = danca_faya_update,
      icon_name = "danca_faya",

      -- Methods
      shutdown = danca_faya_shutdown,
      kill = danca_faya_kill,
      
      -- Members
      z = gui_guess_z(owner,z),
      dl = dl_new_list(128, 0, 0),
      arrow_spr = sprite_get("danca_faya") or
	 sprite_simple(nil,"danca_faya.tga")
   }

   -- Create display lists.
   local spr = ddr.arrow_spr
   local w,h = spr.w, spr.h
   local box_w,box_h = w*5, h*6
   local border_w,border_h = (box_w-w*4)*0.5, h
   local center_x, center_y = border_w + w*0.5, border_h + h*0.5
   local i

   -- background display list
   local bkg_dl = dl_new_list(1024,1,1)
   box = box3d({-center_x, -center_y, box_w-center_x, box_h-center_y},
	       2, nil,
	       "#FFFFFF", "#BBBBBB", "#DDDDDD", "#999999")
   box:draw(bkg_dl, nil, 1)

   local mat = mat_trans(-w*0.5, -h*0.5, 0)
   local rmat = mat_new()
   rmat[1][1] = 0   rmat[1][2] = 1   rmat[2][1] = -1   rmat[2][2] = 0
   spr:set_color(0.65,1,1,1)
   for i=0,3 do
      spr:draw(bkg_dl, mat * mat_trans(i*w,0,0))
      mat_mult_self(mat,rmat)
   end
   spr:set_color(1,1,1,1)
   dl_sublist(ddr.dl, bkg_dl)

   dl_set_trans(bkg_dl,mat_trans(center_x, center_y, 0))
   dl_set_trans(ddr.dl, mat_trans( (640-box_w)*0.5, (480-box_h)*0.5, 0))
   dl_set_active(ddr.dl,1)

   -- Create patterns sublist
   ddr.pat_display = {}
   for i=1,2 do
      local pat_dl, pat0_dl, pat1_dl, pat2_dl
      pat_dl  = dl_new_list(128,0,1)
      pat0_dl = dl_new_list(128,0,1)
      pat1_dl = dl_new_list(128,0,1)
      pat2_dl = dl_new_list(128,0,1)
      dl_sublist(pat_dl, pat0_dl)
      dl_sublist(pat_dl, pat1_dl)
      dl_sublist(pat_dl, pat2_dl)
      dl_sublist(bkg_dl, pat_dl)
      ddr.pat_display[i] = {
	 dl = pat_dl, dl0 = pat0_dl, dl1 = pat1_dl, dl2 = pat2_dl,
      }
   end

   df_prepare_song(ddr, create_pipo_ddr())

   evt_app_insert_first(owner, ddr)
   return ddr
end


--
--- Kill a danca-faya application.
---
---   The danca_faya_kill() function kills the given application by
---   calling the evt_shutdown_app() function. If the given
---   application is nil or danca_faya the default danca-faya
---   (danca_faya) is killed and the global variable danca_faya is
---   set to nil.
---
--- @param  ddr  application to kill (default to danca_faya)
--
function danca_faya_kill(ddr)
   ddr = ddr or danca_faya
   if ddr then
      evt_shutdown_app(ddr)
      if ddr == danca_faya then
	 danca_faya = nil
	 print("danca-faya shutdowned")
      end
   end
end


-- Load texture for application icon
local tex = tex_exist("danca_faya")
   or tex_new(home .. "lua/rsc/icons/danca_faya.tga")

danca_faya_kill()
danca_faya = danca_faya_create()
if danca_faya then
   print("danca-faya is running")
   return 1 
end

-- end of defgroup
--- @}
---
