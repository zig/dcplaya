--- @file      sprite.lua
--- @ingroup   dcplaya_lua_graphics
--- @author    benjamin gerard <ben@sashipa.com>
--- @date      2002/11/06
--- @brief     lua sprite
---

sprite_loaded = nil
if not dolib("basic") then return end

if not sprite_tag then sprite_tag = newtag() end

--- Sprite object.
--- @ingroup   dcplaya_lua_graphics
--- struct sprite {
---
---   string name;  ///< Sprite name
---   number  x;    ///< Origine X coordinate
---   number  y;    ///< Origine Y coordinate
---   number  w;    ///< Width
---   number  h;    ///< Height
---   texture tex;  ///< texture id
---   matrix  vtx;  ///< Sprite vertex definition
---   matrix  tra;  ///< Sprite vertex transform buffer
---
---   draw();		///< Draw sprite into display list
---   
--- };

--- Create a sprite.
--- @ingroup   dcplaya_lua_graphics
---
--- @param  name  sprite name.
--- @param  x     X coordinate of sprite origine.
--- @param  y     Y coordinate of sprite origine.
--- @param  w     Sprite width
--- @param  h     Sprite height
--- @param  u1    texture left coodinate
--- @param  v1    texture top coodinate
--- @param  u2    texture right coodinate
--- @param  v2    texture bottom coodinate
--- @param  texture  texture-id, texture-name, or texture-file
---
--- @return  sprite object
--- @retval  nil  Error.
---
function sprite(name, x, y, w, h, u1, v1, u2, v2, texture)
	local spr = {}

	spr.name = name
	spr.vtx  = mat_new(4,12);
	spr.tra  = mat_new(spr.vtx);
	spr.x    = x
	spr.y    = y
	spr.w    = w
	spr.h    = h
	spr.tex  = tex_get(texture)
	if not spr.tex and type(texture) == "string" then
		spr.tex = tex_new(texture)
	end
	spr.draw = sprite_draw
	spr.set_color = sprite_set_color

	local x1,y1,x2,y2
	x1 = -x
	y1 = -y
	x2 = x1+w
	y2 = y1+h

	set_vertex(spr.vtx[1], { x1, y1, 0, 1, 1,1,1,1, u1, v1 } )
	set_vertex(spr.vtx[2], { x2, y1, 0, 1, 1,1,1,1, u2, v1 } )
	set_vertex(spr.vtx[3], { x1, y2, 0, 1, 1,1,1,1, u1, v2 } )
	set_vertex(spr.vtx[4], { x2, y2, 0, 1, 1,1,1,1, u2, v2 } )

	settag(spr, sprite_tag)

	return spr
end

--- Default sprite drawing function.
function sprite_draw(spr, dl, x, y, z, sx, sy)
   local mat
   
   if tag(x) == matrix_tag then
	  mat = x
   else
	  sx = sx or 1
	  sy = sy or sx
	  mat = mat_mult_self(mat_scale(sx, sy, 1), mat_trans(x,y,z))
   end
   mat_mult_self(spr.tra, spr.vtx, mat)
   dl_draw_strip(dl, spr.tra, spr.tex, flags)
end

--- Default sprite set color function.
function sprite_set_color(spr, a, r, g, b)
   if tag(spr) == sprite_tag then
	  local i
	  for i=1, 4 do
		 if a then spr.vtx[i][5] = a end
		 if r then spr.vtx[i][6] = r end
		 if g then spr.vtx[i][7] = g end
		 if b then spr.vtx[i][8] = b end
	  end
   end
end


sprite_loaded = 1
return sprite_loaded
