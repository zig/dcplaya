--- @ingroup dcplaya_addons_ik
--- @file    ik.lua
--- @date    2002/12/19
--- @author  benjamin gerard <ben@sashipa.com>
--- @brief   Internationnal Karate Remix
--

--- @defgroup dcplaya_addons_ik Internationnal Karate Remix
--- @ingroup dcplaya_addons

if not dolib("sprite") then return nil end

function ik()

   local ik_attack_boxes = dofile("addons/ik/ik_attack_boxes.lua")
   if not ik_attack_boxes then return end

   local ik_target_boxes = dofile("addons/ik/ik_target_boxes.lua")
   if not ik_target_boxes then return end

   local ik_sprite_defs = dofile("addons/ik/ik_sprite_defs.lua")
   if not ik_sprite_defs then return end

   local ik_anim_defs = dofile("addons/ik/ik_anim_defs.lua")
   if not ik_anim_defs then return end

   if not dofile("addons/ik/ik_anim.lua") then return end

   local ik_sprites = {}
   local ik_anims = {}

   function ik_create_sprites()
      local i,v
      local texinfo
      local texname = fullpath("addons/ik/ik.tga")
      -- Load ik textures
      local iktex = tex_get("ik") or tex_new(texname)
      texinfo = tex_info(iktex)
      if not texinfo then return end

      local allocated_boxes = 0

      for i,v in %ik_sprite_defs do
	 local x,y,w,h,u1,v1,u2,v2,x1,y1,x2,y2

	 -- 		 print("Creating IK [".. i .. "]")
	 x = (v.pos and v.pos[1]) or 0
	 y = (v.pos and v.pos[2]) or 0
	 --		 print (x,y)

	 x1,y1,w,h = v.box[1],v.box[2],v.box[3],v.box[4]
	 x2,y2 = x1+w, y1+h 
	 u1,v1 = x1/texinfo.w, y1/texinfo.h
	 u2,v2 = x2/texinfo.w, y2/texinfo.h
	 local box = { x1, y1, x2, y2 }
	 %ik_sprites[i] = {
	    name = i, -- for debuggin
	    sprite = sprite(i, x, y, w, h, u1, v1, u2, v2, iktex),
	    box = box,
	 }

	 local in_box =
	    function(b1,b2) 
	       local x1,y1 = b2[1],b2[2]
	       local x2,y2 = x1+b2[3],y1+b2[4]
	       return (x1>=b1[1]) and (y1>=b1[2])
		  and (x2<=b1[3]) and (y2<=b1[4])
	    end
	 
	 local ia,va
	 for ia,va in %ik_attack_boxes do
	    if in_box(box, va) then
	       allocated_boxes = allocated_boxes+1
	       %ik_sprites[i].attack = va - { x1+x,y1+y,0,0,0 }
	    end
	 end
	 for ia,va in %ik_target_boxes do
	    if in_box(box, va) then
	       if not %ik_sprites[i].targets then
		  print ("creating targets for "..i)
		  %ik_sprites[i].targets = {}
	       end
	       %ik_sprites[i].targets[ia] =  va - { x1+x,y1+y,0,0,0 }
	       allocated_boxes = allocated_boxes+1
	    end
	 end

      end
      if allocated_boxes ~= getn(%ik_target_boxes)+getn(%ik_attack_boxes) then
	 print("ik_create_sprite : Invalid number of boxes")
	 return
      end

      return 1
   end

   function ik_display_all_sprites()
      local xdraw, ydraw, hmax = 0,0,0
      local i,v

      ik_dl = dl_new_list(0,1)
      dl_set_trans(ik_dl, mat_trans(80,80,4000))

      for i,v in %ik_sprites do
	 local w,h = v.box[3]-v.box[1], v.box[4]-v.box[2]
	 print("Display IK [".. i .. "]")
	 if xdraw + w > 512 then 
	    xdraw = 0
	    ydraw = ydraw + hmax + 4
	    hmax = 0
	 end
	 v.sprite:draw(ik_dl, mat_trans(xdraw,ydraw,0))
	 xdraw = xdraw + w
	 hmax = max(hmax,h)
      end
   end



   --- 
   function ik_create_anims()
      local i,v
      for i,v in %ik_anim_defs do
	 --		 print("Creating anim ["..i.."]")
	 local anim = {
	    name = i,
	    total_time = 0,
	 }
	 
	 local option = nil
	 local j,w
	 for j,w in v do
-- 	    print(" ["..i.."] ["..j.."] ["..tostring(w[1]).."]")
	    if type(w) == "table" then
	       if type(w[1]) == "string" then
		  tinsert(anim, {
			     sprite = %ik_sprites[w[1]],
			     time   = w[2],
			  })
		  anim.total_time = anim.total_time + w[2]
	       elseif option then
		  print("ik_create_anims : option setted more than once.")
		  return
	       else
		  option = w
	       end
	    end
	 end
	 if option then
	    for j,w in option do
--	       print("copying option ["..j.."] ["..w.."]")
	       anim[j] = w
	    end
	 end
	 tinsert(%ik_anims, anim)
      end

      -- Copy anim to named field
      for i,v in %ik_anims do
	 if type(v) == "table" then
	    %ik_anims[v.name] = v
	 end
      end

      return 1
   end

   function ik_play_anim(dl1,dl2, anim_ref)
      local i,n
      local anim = ik_anim_start(anim_ref, 0.1)

      while anim.step ~= 0 do
	 dl_clear(dl1)
	 ik_anim_draw(anim, dl1)
	 dl_set_active2(dl1,dl2,1)
	 dl1,dl2 = dl2,dl1
	 local key = evt_peekchar()
	 if key then
	    if key == 27 then
	       print("ESCAPE", anim.step)
	       return
	    else
	       anim.release = 1
	    end
	 end
	 ik_anim_play(anim, frame_to_second(framecounter()))
      end
   end
   
   function ik_test_anim()
      local key
      local dl, sdl1, sdl2

      dl = dl_new_list(0,1)
      dl1 = dl_new_list(0,0,1)
      dl2 = dl_new_list(0,0,1)
      dl_sublist(dl, dl1)
      dl_sublist(dl, dl2)
      dl_set_trans(dl, mat_trans(320,400,2000))
      dl_draw_box1(dl,-128,1,128,3,0, 1,1,0,0)

      cond_connect(1)

      local player = {
	 id = 2,
	 cont = nil,
	 status = "salut",
	 anim = nil,
      }
      player.anim = ik_anim_start(%ik_anims[player.status], 0.1)
      print("init : "..player.anim.ref.name)
      print(framecounter())
      evt_peekchar() -- Reset time counter

      -- wait a while.
      local i
      for i=1,60 do
	 framecounter()
	 evt_peekchar() -- Reset time counter
      end

      local key
      while not key or key ~= 27 do
	 local elapsed_time = frame_to_second(framecounter())

	 --- Read controler
	 player.cont = controler_read(player.id)
	 if player.cont then

	    if player.release_button and player.anim.wait
	       and not player.cont.buttons[player.release_button] then
	       player.release_button = nil
	       player.anim.release = 1
	    end

	    if player.status == "idle" then
	       if player.cont.buttons.a and player.cont.buttons.center then
		  player.status = "barai"
		  player.release_button = "a"
		  player.anim = nil

	       elseif player.cont.buttons.x then
		  player.release_button = "x"
		  player.anim = nil
		  if player.cont.buttons.down then
		     player.status = "mpunch"
		  else
		     player.status = "hpunch"
		  end

	       elseif player.cont.buttons.b then
		  player.release_button = "b"
		  player.anim = nil
		  if player.cont.buttons.down then
		     player.status = "lkick"
		  elseif player.cont.buttons.up then
		     player.status = "hkick"
		  elseif player.cont.buttons.left then
		     player.status = "backkick"
		  else
		     player.status = "mkick"
		  end
	       end

	    end

	 end

	 -- End of current anim : set "idle"
	 if player.anim then
	    if player.anim.step == 0 then
	       player.anim = nil
	       print("Going idle ["..tostring(player.status).."]")
	       player.status = "idle"
	    end
	 end

	 -- No anim : start new one
	 if not player.anim then
	    player.anim = ik_anim_start(%ik_anims[player.status], 0.1)
	    print("starting ["..tostring(player.status).."]")
	 end

	 -- Draw sprite
	 dl_clear(dl1)
	 if player.anim then
	    dl_draw_text(dl1,-16,24,20,1,1,1,1, 
			 format("%q %.2f", player.anim.ref.name,
				elapsed_time * 60))
	    ik_anim_draw(player.anim, dl1,1,1,1)
	    dl_set_active2(dl1,dl2,1)
	    dl1,dl2 = dl2,dl1
	    ik_anim_play(player.anim, elapsed_time)
	 end
	 
	 key = evt_peekchar()
      end

      cond_connect(-1)
   end


   ik_dl = nil
   if not ik_create_sprites() then return end
   if not ik_create_anims() then return end
   hideconsole()
   ik_test_anim()
   showconsole()
--   ik_display_all_sprites()

end

ik()
