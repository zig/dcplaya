--- @ingroup dcplaya_apps
--- @file    ik.lua
--- @date    2002/12/19
--- @author  benjamin gerard
--- @brief   Internationnal Karate Remix
--

if not dolib("sprite") then return nil end

function ik()

   local ik_attack_boxes = {
	  { 502,  14,  10,   8, 1.00 },
	  { 413,  19,  12,   9, 1.00 },
	  { 483,  77,  10,   8, 1.00 },
	  { 397,  89,   9,   9, 1.00 },
	  { 264,  95,   8,   9, 1.00 },
	  { 347,  95,  14,   7, 1.00 },
	  { 277, 136,  14,   8, 1.00 },
	  { 366, 154,  12,   7, 1.00 },
	  { 103, 165,  18,   6, 1.00 },
	  { 194, 171,  12,   7, 1.00 },
	  { 235, 240,  21,   8, 1.00 },
	  { 147, 243,   9,   7, 1.00 },
   }

   local ik_target_boxes = {
	  {   5,   4,  15,  57, 1.00 },
	  { 272,   7,  14,  11, 1.00 },
	  {  49,   8,  14,  10, 1.00 },
	  { 211,   9,  13,  12, 1.00 },
	  { 351,  14,  11,  10, 1.00 },
	  {  95,  15,  11,  12, 1.00 },
	  { 438,  17,  15,  12, 1.00 },
	  {  40,  19,  16,  10, 1.00 },
	  {  86,  20,   8,  14, 1.00 },
	  { 402,  20,  13,  10, 1.00 },
	  { 334,  22,  16,  15, 1.00 },
	  {  73,  26,  12,  35, 1.00 },
	  { 459,  26,  16,  16, 1.00 },
	  {  37,  30,  12,  31, 1.00 },
	  { 278,  34,  11,  19, 1.00 },
	  { 383,  42,  10,  15, 1.00 },
	  { 330,  43,   9,  13, 1.00 },
	  { 466,  43,   9,   9, 1.00 },
	  { 135,  46,   9,  10, 1.00 },
	  { 162,  48,   7,   9, 1.00 },
	  { 471,  53,   5,  10, 1.00 },
	  { 214,  54,  10,  10, 1.00 },
	  { 278,  54,   9,   6, 1.00 },
	  { 130,  57,   7,   7, 1.00 },
	  { 328,  57,   7,   7, 1.00 },
	  { 383,  59,  12,   5, 1.00 },
	  { 167,  60,   9,   4, 1.00 },
	  {  77,  69,  12,  11, 1.00 },
	  {  15,  71,  13,  11, 1.00 },
	  { 449,  71,  13,  11, 1.00 },
	  { 190,  72,  13,  12, 1.00 },
	  { 386,  72,  13,  10, 1.00 },
	  { 135,  73,  13,  11, 1.00 },
	  { 253,  78,  12,  10, 1.00 },
	  {  76,  82,  16,  27, 1.00 },
	  { 385,  83,  14,   7, 1.00 },
	  { 321,  84,  12,  11, 1.00 },
	  { 134,  85,  16,  17, 1.00 },
	  { 190,  85,  16,  15, 1.00 },
	  { 446,  94,  16,  12, 1.00 },
	  { 315,  96,  16,  14, 1.00 },
	  {  15, 100,  17,   9, 1.00 },
	  { 196, 101,  10,   9, 1.00 },
	  { 389, 101,  12,  10, 1.00 },
	  { 132, 104,  21,   8, 1.00 },
	  { 187, 104,   8,  23, 1.00 },
	  { 251, 107,  20,   8, 1.00 },
	  { 272, 108,   8,  20, 1.00 },
	  {  33, 109,   8,   7, 1.00 },
	  {  95, 109,   7,   7, 1.00 },
	  { 402, 109,   7,   8, 1.00 },
	  {   9, 110,   7,   8, 1.00 },
	  {  69, 110,  10,   7, 1.00 },
	  { 379, 110,   7,   8, 1.00 },
	  { 205, 112,   9,  15, 1.00 },
	  { 463, 112,   9,  14, 1.00 },
	  { 129, 113,   7,  15, 1.00 },
	  { 153, 113,   7,   7, 1.00 },
	  {  39, 118,   9,   8, 1.00 },
	  { 240, 118,   9,   9, 1.00 },
	  {   3, 119,   9,   6, 1.00 },
	  {  65, 119,   9,   6, 1.00 },
	  { 101, 120,   7,   6, 1.00 },
	  { 374, 120,   5,   7, 1.00 },
	  { 157, 121,   7,   6, 1.00 },
	  { 407, 122,   8,   5, 1.00 },
	  { 327, 124,  10,   4, 1.00 },
	  { 142, 134,  12,  11, 1.00 },
	  {   9, 135,  12,  11, 1.00 },
	  {  68, 136,  11,  10, 1.00 },
	  { 305, 143,  12,  11, 1.00 },
	  { 219, 147,  13,  10, 1.00 },
	  { 237, 155,  17,  13, 1.00 },
	  { 319, 156,  18,  11, 1.00 },
	  {  76, 158,  15,  10, 1.00 },
	  { 156, 159,  11,  10, 1.00 },
	  { 329, 168,   5,  24, 1.00 },
	  {  12, 169,   9,  21, 1.00 },
	  {  83, 169,   8,  21, 1.00 },
	  { 248, 169,   8,  21, 1.00 },
	  { 155, 182,   9,   8, 1.00 },
	  {  69, 196,  12,  34, 1.00 },
	  {  15, 198,  12,  11, 1.00 },
	  { 129, 203,  12,  12, 1.00 },
	  {  16, 210,  10,  17, 1.00 },
	  { 188, 214,  14,  12, 1.00 },
	  {  26, 227,  10,  22, 1.00 },
	  {  10, 229,   8,  21, 1.00 },
   }

   local ik_spritedefs = {
	  ["salut"] =  {
		 pos  = {7,60},
		 box  = {1,1,30,61},
	  },
	  
	  ["salut1"] = {
		 pos  = {4,7},
		 box  = {33,4,34,58},
	  },

	  ["salut2"] = {
		 pos  = {4,49},
		 box  = {69,11,43,50},
	  },

	  ["barai"] = {
		 pos  = {5,57},
		 box  = {128,5,54,59},
	  },

	  ["back0"] = {
		 pos  = {30, 56},
		 box  = {186,7,61,57},
	  },
	  ["back1"] = {
		 pos  = {25,54},
		 box  = {250,7,46,56},
	  },
	  ["back2"] = {
		 pos  = {28,51},
		 box  = {304,12,64,52},
	  },
	  ["back3"] = {
		 pos  = {6,57},
		 box  = {380,6,45,58},
	  },
	  ["back4"] = {
		 pos  = {35,48},
		 box  = {436,14,76,49},
	  },

	  ["wait"] = {
		 pos  = {5,57},
		 box  = {2,67,58,59},
	  },

	  ["kamae0"] = {
		 pos  = {6, 58},
		 box  = {63,67,54,59},
	  },

	  ["kamae1"] = {
		 pos  = {8, 58},
		 box  = {124,70,52,59},
	  },
	  ["kamae2"] = {
		 pos  = {12, 58},
		 box  = {179,69,52,59},
	  },

	  ["mpunch0"] = {
		 pos  = {3-2, 52},
		 box  = {240,75,49,53},
	  },
	  ["mpunch1"] = {
		 pos  = {7-4, 46},
		 box  = {292,81,69,47},
	  },

	  ["hpunch0"] = {
		 pos  = {6, 58},
		 box  = {369,69,54,59},
	  },
	  ["hpunch1"] = {
		 pos  = {6, 58},
		 box  = {425,68,68,59},
	  },

	  ["kick0"] = {
		 pos  = {15, 57},
		 box  = {3,132,46,59},
	  },
	  [ "kick1"] = {
		 pos  = {34, 55},
		 box  = {52,134,69,56},
	  },

	  ["lkick3"] = {
		 pos  = {30, 60-3},
		 box  = {128,131,78,59},
	  },
	  ["hkick3"] = {
		 pos  = {39, 53},
		 box  = {211,136,80,54},
	  },
	  ["mkick3"] = {
		 pos  = {39, 51},
		 box  = {294,140,84,52},
	  },

	  ["ko"] = {
		 pos  = {39,14},
		 box  = {383,176,94,18},
	  },

	  ["jump1"] = {
		 pos  = {},
		 box  = {4,195,48,55},
	  },
	  ["jump2"] = {
		 pos  = {},
		 box  = {55,193,55,54},
	  },
	  ["jump_shadow"] = {
		 pos  = {},
		 box  = {59,249,49,5},
	  },

	  ["balayage0"] = {
		 pos  = { 126-113, 249-202 },
		 box  = {113,202,53,48},
	  },
	  ["balayage1"] = {
		 pos  = {23,34},
		 box  = {168,212,88,36},
	  },
   }

   local ik_sprites = {}

   function ik_create_sprites()
	  local i,v
	  local texinfo
	  local texname = fullpath("addons/ik/ik.tga")
	  -- Load ik textures
	  print(filetype(texname))
	  local iktex = tex_get("ik") or
		 tex_new(texname)
	  texinfo = tex_info(iktex)
	  if not texinfo then return end

	  local allocated_boxes = 0

	  for i,v in % ik_spritedefs do
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

   local ik_anim_defs = {
	  ["walk"] = {
		 { "kamae0", 1 },
		 { "kamae1", 1 },
		 { "kamae2", 1 },
		 { "kamae1", 1 },
		 { "kamae0", 1 },
	  },

	  ["mpunch"] = {
		 { "kamae0", 1 },
		 { "mpunch0", 1 },
		 { "mpunch1", 1 },
		 { "mpunch0", 1 },
		 { "kamae0", 1 },
	  },

	  ["hpunch"] = {
		 { "kamae0", 1 },
		 { "hpunch0", 1 },
		 { "hpunch1", 1 },
		 { "hpunch0", 1 },
		 { "kamae0", 1 },
	  },

	  ["hkick"] = {
		 { "kamae0", 1 },
		 { "kick0", 1 },
		 { "kick1", 1 },
		 { "hkick3", 1 },
		 { "kick1", 1 },
		 { "kick0", 1 },
		 { "kamae0", 1 },
	  },

	  ["mkick"] = {
		 { "kamae0", 1 },
		 { "kick0", 1 },
		 { "kick1", 1 },
		 { "mkick3", 1 },
		 { "kick1", 1 },
		 { "kick0", 1 },
		 { "kamae0", 1 },
	  },

	  ["lkick"] = {
		 { "kamae0", 1 },
		 { "kick0", 1 },
		 { "kick1", 1 },
		 { "lkick3", 1 },
--		 { "kick1", 1 },
		 { "kick0", 1 },
		 { "kamae0", 1 },
	  },


   }

   local ik_anims = {}

   --- 
   function ik_create_anims()
	  local i,v
	  for i,v in %ik_anim_defs do
--		 print("Creating anim ["..i.."]")
		 local anim = { name = i }
		 
		 local j,w
		 for j,w in v do
--			print(" ["..i.."] ["..j.."] ["..w[1].."]")
			tinsert(anim, {
					   sprite = %ik_sprites[w[1]],
					})
		 end
		 tinsert(%ik_anims, anim)
	  end
	  return 1
   end

   function ik_play_anim(dl1,dl2, anim)
	  local i,n
	  n = getn(anim)
	  for i=1, n do
		 local spr1 = anim[i].sprite
		 local spr2 = anim[i+1] and anim[i+1].sprite
		 for r=0,1,0.1 do
			dl_clear(dl1)
			spr1.sprite:draw(dl1)
			if spr1.attack then
			   local ab = spr1.attack
			   if spr2 and spr2.attack then
				  ab = ab * (1-r) + spr2.attack * r
			   end
			   dl_draw_box1(dl1, ab[1],ab[2], ab[1]+ab[3], ab[2]+ab[4], 10,
							0.8*ab[5], 1, 1, 0)
			end

			dl1,dl2 = dl2,dl1
			dl_set_active2(dl1,dl2,2)
			evt_peek()
		 end
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
		 local num = key - strbyte("0")
--		 print(num, getn(%ik_anims))

		 if not num then return end
		 local anim = %ik_anims[num]
		 if not anim then return end
		 ik_play_anim(sdl1, sdl2, anim)
	  until nil
   end

   if not ik_create_sprites() then return end
   if not ik_create_anims() then return end
   hideconsole()
   ik_test_anim()
   showconsole()
--   ik_display_all_sprites()

end

ik()
