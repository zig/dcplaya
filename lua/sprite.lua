--- @file      sprite.lua
--- @ingroup   dcplaya_lua_graphics
--- @author    benjamin gerard <ben@sashipa.com>
--- @date      2002/11/06
--- @brief     lua sprite
---

sprite_loaded = nil
if not dolib("basic") then return end
if not dolib("dirfunc") then return end

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
---   set_color();  ///< Set sprite color
---   
--- };


if not sprite_list then
   sprite_list = { }
end

--- Get a named sprite
--- @ingroup   dcplaya_lua_graphics
---
--- @param  name  sprite name.
function sprite_get(name)
   return sprite_list[name]
end


--- Create a sprite and add it in the list of named sprite.
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
--- @param  rotate   non nil to perform a -90 degree rotation.
---
--- @return  sprite object
--- @retval  nil  Error.
---
function sprite(name, x, y, w, h, u1, v1, u2, v2, texture, rotate)
   local spr = {}

   spr.name = name
   spr.vtx  = mat_new(4,12);
   spr.tra  = mat_new(spr.vtx);
   spr.tex  = tex_get(texture)
   if not spr.tex and type(texture) == "string" then
      spr.tex = tex_new(texture)
   end
   spr.draw = sprite_draw
   spr.set_color = sprite_set_color

   set_vertex(spr.vtx[1], { 0, 0, 0, 1, 1,1,1,1, u1+0.5/w, v1+0.5/h } )
   set_vertex(spr.vtx[2], { w, 0, 0, 1, 1,1,1,1, u2+0.5/w, v1+0.5/h } )
   set_vertex(spr.vtx[3], { 0, h, 0, 1, 1,1,1,1, u1+0.5/w, v2+0.5/h } )
   set_vertex(spr.vtx[4], { w, h, 0, 1, 1,1,1,1, u2+0.5/w, v2+0.5/h } )

   local org
   local mat
   if not rotate then
      mat = mat_trans(-x, -y, 0)
      org = 1
   else
      local tr = mat_new()
      tr[1][1] = 0
      tr[1][2] = -1
      tr[2][1] = 1
      tr[2][2] = 0
      mat = mat_trans(-w,0,0) * tr * mat_trans(-y, -x, 0)
      w,h = h,w
      org = 2
   end

   mat_mult_self(spr.vtx, mat)

   spr.x = -spr.vtx[org].x
   spr.y = -spr.vtx[org].y
   spr.w = w
   spr.h = h

   settag(spr, sprite_tag)

   if spr.tex then
      local info = tex_info(spr.tex)
      sprite_list[spr.name] = spr
   end

   return spr
end


--- Create a sprite from an image file.
--- @ingroup   dcplaya_lua_graphics
---
--- @param  name      optionnal sprite name.
--- @param  filename  image file path (can be a texture name too).
---
--- @return  sprite object
--- @retval  nil  Error.
---
function sprite_simple(name, filename)
   name = name or get_nude_name(filename) or filename
   if not name then
      return
   end
   local tex = tex_get(filename)
      or tex_new(home.."lua/rsc/icons/"..filename)
   if not tex then return end
   local info = tex_info(tex)
   if not info then return end
   local info_w, info_h = info.w, info.h
   local orig_w, orig_h = info.orig_w, info.orig_h
   return sprite(name, 0, 0, orig_w, orig_h,
		   0, 0, orig_w/info_w, orig_h/info_h, tex)
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
