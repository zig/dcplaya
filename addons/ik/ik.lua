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
	       print("copying option ["..j.."] ["..w.."]")
	       anim[j] = w
	    end
	 end
	 tinsert(%ik_anims, anim)
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
      sdl1 = dl_new_list(0,0,1)
      sdl2 = dl_new_list(0,0,1)
      dl_sublist(dl, sdl1)
      dl_sublist(dl, sdl2)
      dl_set_trans(dl, mat_trans(320,400,2000))
      dl_draw_box1(dl,-128,1,128,3,0, 1,1,0,0)

      repeat
	 key = getchar()
	 if key == 27 then return end
	 local num = mod(key,getn(%ik_anims))
	 if not num then return end
	 local anim = %ik_anims[num+1]
	 if not anim then return end
	 ik_play_anim(sdl1, sdl2, anim)
      until nil
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
