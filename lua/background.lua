--- @ingroup dcplaya_lua_background
--- @file    background.lua
--- @author  benjamin gerard
--- @date    2002/12/16
--- @brief   Background image.
---

--- @defgroup  dcplaya_lua_background  Background
--- @ingroup   dcplaya_lua_graphics
--- @brief     background graphic objects
--- @author    benjamin gerard
--- @{
---

--- Background object.
--: struct background {
--:
--:   texture tex;     ///< texture id
--:   matrix  vtx;     ///< Backgound vertex definition.
--:   display_list dl; ///< Background display list
--:
--:  static display_list background_dl;  ///< Displayed background
--:  static tag background_tag;          ///< Background object unic tag
--:  
--:   set_color();   ///< @see background_set_colors()
--:   set_texture(); ///< @see background_set_texture()
--:   draw();        ///< @see background_draw()
--:   kill();        ///< @see background_kill()
--:   
--: };


-- Load required libraries
if not dolib("color") then return end

-- Create background tag
background_tag = background_tag or newtag()

--- Set background texture.
--- @param  bkg      Background
--- @param  texture  Image file or texture-id
--- @param  type     One of [ "scale", "center", "tile" ] default : "scale"
function background_set_texture(bkg, texture, type)
   local vtx
   if tag(bkg) ~= background_tag then return end
   local i,v
   if not texture then
      bkg.tex = nil
      set_vertex(bkg.vtx[1],{ 0, 0, 0, 1 })
      set_vertex(bkg.vtx[2],{ 1, 0, 0, 1 })
      set_vertex(bkg.vtx[3],{ 0, 1, 0, 1 })
      set_vertex(bkg.vtx[4],{ 1, 1, 0, 1 })
      vtx = 1
   else
      vtx = load_background(texture, type)
      if vtx then
	 for i,v in vtx do
	    set_vertex(bkg.vtx[i],
		       { v[1], v[2], 0, 1,
			  nil,nil,nil,nil,
			  v[3],v[4] } )
	 end
      end
      bkg.tex = tex_get("__background__")
   end
   bkg:draw()
   return vtx ~= nil
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
   bkg:draw()
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

   if not tex_exist("__background__") then
      tex_new("__background__", 1024,512, 1, 0, 0, 0)
   end

   bkg = {
      -- Vertrices
      vtx = mat_new(4,12),

      -- Display list
      dl = dl_new_list(256, 1, 1, "bkg.dl"),

      -- Set color method
      set_color = background_set_colors,

      -- Set texture method
      set_texture = background_set_texture,
      
      -- Draw method
      draw = background_draw,

      -- Kill method
      kill = background_kill,

   }
   settag(bkg, background_tag)
   bkg:set_texture()
   bkg:set_color()
   return bkg
end

function background_shutdown(bkg)
   return background_kill(bkg)
end

--- Kill background.
---
---  @param  bkg  background to kill (nil for default background)
---  
function background_kill(bkg)
   bkg = bkg or background
   if tag(bkg) ~= background_tag then return end
   bkg.dl = nil
   if bkg == background then
      background_dl = nil
      background = nil
      print("background killed")
   end
end

background_kill()
background = background_create()

if background then
   print("background created")

   if tag (background_dl) ~= dl_tag then
      background_dl = dl_new_list(128,nil,nil,"background_dl")
   end
   background:set_texture("/rd/dcpbkg2.jpg", "scale")
   dl_set_trans(background.dl, mat_scale(640,480,1))
   dl_set_trans(background_dl, mat_trans(0,0,0.0001))
   background:draw(background_dl)
   dl_set_active(background_dl,1)
end

background_loaded = 1
return background_loaded

--
--- @}
---
