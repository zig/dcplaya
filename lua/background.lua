--- @ingroup dcplaya_lua
--- @file    background.lua
--- @author  benjamin gerard <ben@sashipa.com>
--- @date    2002/12/16
--- @brief   Background image.
---

-- Unload library
background_loaded = nil

-- Load required libraries
if not dolib("color") then return end

-- Create background tag
background_tag = background_tag or newtag()

--- Set background texture.
--- @param  bkg      Background
--- @param  texture  Image file or texture-id
--- @param  type     One of [ "scale", "center", "tile" ] default : "scale"
function background_set_texture(bkg, texture, type)
   if tag(bkg) ~= background_tag then return end
   local vtx = load_background(texture, v_align, h_align)
   if vtx then
	  dump(vtx)
	  local i,v
	  for i,v in vtx do
		 set_vertex(bkg.vtx[i],
					{ v[1], v[2], 0, 1,
					   nil,nil,nil,nil,
					   v[3],v[4] } )
	  end
   end
   bkg.tex = tex_get("background")
end

--- Set background colors.
function background_set_colors(bkg, color1, color2, color3, color4)
   if tag(bkg) ~= background_tag then return end
   color1 = color1 or color_new()
   color2 = color2 or color1
   color3 = color3 or color2
   color4 = color4 or color3
   local i,v
   for i,v in { color1, color2, color3, color4 } do
	  set_vertex(bkg.vtx[i],
					{ nil, nil, nil, nil,
					   v[1], v[2], v[3], v[4] })
-- 	  bkg.vtx[i][5] = v[1]
-- 	  bkg.vtx[i][6] = v[2]
-- 	  bkg.vtx[i][7] = v[3]
-- 	  bkg.vtx[i][8] = v[4]
   end
   bkg.noalpha = (color1[1] == 1) and (color2[1] == 1)
	  and (color3[1] == 1) and (color4[1] == 1)
end

--- Draw background.
function background_draw(bkg, dl)
   if tag(bkg) ~= background_tag then return end
   dl_clear(bkg.dl)
   dl_draw_strip(bkg.dl, bkg.vtx, bkg.tex, bkg.noalpha)
   if dl then dl_sublist(dl, bkg.dl) end
end

--- Create a background object.
function background_create()
   local bkg

   if not tex_get("background") then
	  tex_new("background", 1024,512, 1, 0, 0, 0)
   end

   bkg = {
	  -- Vertrices
	  vtx = mat_new(4,12),

	  -- Display list
	  dl = dl_new_list(256, 1, 1),

	  -- Set color method
	  set_color = background_set_colors,

	  -- Set texture method
	  set_texture = background_set_texture,
		 
	  -- Draw method
	  draw = background_draw,
   }
   settag(bkg, background_tag)

   return bkg
end

bkg = background_create()
if bkg then
   dl = dl_new_list(128,1)
   bkg:set_texture("/rd/dcpsprites.tga")
--   bkg:set_texture("/pc/pucca1_1024.jpg")
   bkg:set_color()
   dl_set_trans(bkg.dl, mat_scale(640,480,1))
   dl_set_trans(dl, mat_trans(0,0,0.0001))
   bkg:draw(dl)
   dl_set_active(dl,1)
end

background_loaded = 1
return background_loaded
