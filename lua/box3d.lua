--- @ingroup dcplaya_lua_gui
--- @file    box3d.lua
--- @author  ben(jamin) gerard
--- @date    2002/12/10
--- @brief   Draw a box with 3D border.
---

-- Load required libraries
if not dolib("color") then return end

--- @defgroup  dcplaya_lua_box3d  Bordered Boxes.
--- @ingroup   dcplaya_lua_graphics
--- @brief     draw bordered box
--- @{
---

--- box3d object.
--- struct box3d {
---
---    box3d(box, border, bkg, top, left, bottom, right); ///< @see ::box3d()
---
---    /** Box vertrices.
---     *
---     *  inner-box (vtx[1][1], vtx[1][2], vtx[4][1], vtx[4][2])
---     *
---     *   outer_box(vtx[5][1], vtx[5][2], vtx[20][1], vtx[20][2])
---     */
---    mtx vtx;
---    mtx trans; ///< Transformed vertrices
---    boolean background; ///< Draw box background default.
---
---    draw(b3d, dl, mat, clip); ///< @see box3d_draw()
---
---
---
--- };

--- Create a box3d object.
---
--- @param  box     box dimension {x1,y1,x2,y2}
--- @param  border  number or {x, y}, minus value for outer border
--- @param  bkg     inner box colors
--- @param  top     top border colors
--- @param  left    left border colors
--- @param  bottom  bottom border colors
--- @param  right   right border colors
---
--- @return box3d table
--- @retval nil on error
---
function box3d(box, border, bkg, top, left, bottom, right)

   --- Set a box3d box.
   --- @internal
   function b3d_set_vertex(vtx,i,pt,colors)
      local j
      for j=1, 4 do
	 set_vertex(vtx[i+j],
		    {
		       pt[j][1], pt[j][2], 0, 1,
		       colors[j][1], colors[j][2], colors[j][3], colors[j][4]
		    })
      end
   end

   --- Get a 4 color table from a color definition
   ---  @internal
   function box3d_colors(color)
      local colors = {}
      local j

      if type(color) ~= "table" then
	 color = color_new()
      end

      if type(color[1]) == "number" then
	 for j=1,4 do colors[j] = color end
      elseif type(color[1]) == "table" then
	 colors[1] = color[1] or color_new()
	 for j=2,4 do colors[j] = color[j] or colors[j-1] end
      end

      return colors
   end

   --- Get box3d inner box.
   ---  @return box table {x1,y1,x2,y2}
   function box3d_inner_box(b3d)
      return { b3d.vtx[1][1], b3d.vtx[1][2], b3d.vtx[4][1], b3d.vtx[4][2] }
   end

   --- Get box3d outer box.
   ---  @return box table {x1,y1,x2,y2}
   function box3d_outer_box(b3d)
      return { b3d.vtx[5][1], b3d.vtx[5][2], b3d.vtx[20][1], b3d.vtx[20][2] }
   end

   --- Draw a box3d.
   ---  @param  b3d  box3d
   ---  @param  dl   Display list
   ---  @param  mat  Transform matrix (or nil)
   ---  @param  clip Append dl_set_clipping() 
   function box3d_draw(b3d, dl, mat, clip)
      local vtx, i

      if tag(mat) == matrix_tag then
	 mat_mult_self(b3d.trans, b3d.vtx, mat)
	 vtx = b3d.trans
      else
	 vtx = b3d.vtx
      end

      for i=b3d.background or 5, 20, 4 do
	 dl_draw_strip(dl, vtx[i], 4)
      end

      if clip then
	 dl_set_clipping(dl,
			 vtx[1][1], vtx[1][2],
			 vtx[4][1], vtx[4][2])
      end
   end

   local border_x, border_y = 2,2

   if type(border) == "number" then
      border_x,border_y = border,border
   elseif type(border) == "table" then
      border_x,border_y = (border[1] or 2), (border[2] or 2)
   end

   local b3d = {
      vtx = mat_new(20,8),   -- Box vertrices
      trans = mat_new(20,8), -- Transformed vertrices
      draw = box3d_draw,     -- Draw method
      background = bkg ~= nil,
   }

   local inner_box, outer_box
   outer_box = box
   inner_box = box + { border_x, border_y, -border_x, -border_y }

   if inner_box[1] > inner_box[3] then
      inner_box[1] = (outer_box[1] + outer_box[3]) * 0.5
      inner_box[3] = inner_box[1]
   end

   if inner_box[2] > inner_box[4] then
      inner_box[2] = (outer_box[2] + outer_box[4]) * 0.5
      inner_box[4] = inner_box[2]
   end

   if border_x < 0 then
      inner_box[1], outer_box[1] = outer_box[1], inner_box[1]
      inner_box[3], outer_box[3] = outer_box[3], inner_box[3]
   end

   if border_y < 0 then
      inner_box[2], outer_box[2] = outer_box[2], inner_box[2]
      inner_box[4], outer_box[4] = outer_box[4], inner_box[4]
   end

   -- Inner box.
   b3d_set_vertex(b3d.vtx, 0, {
		     { inner_box[1], inner_box[2] },
		     { inner_box[3], inner_box[2] },
		     { inner_box[1], inner_box[4] },
		     { inner_box[3], inner_box[4] }}, box3d_colors(bkg))

   -- Top border.
   b3d_set_vertex(b3d.vtx, 4, {
		     { outer_box[1], outer_box[2] },
		     { outer_box[3], outer_box[2] },
		     { inner_box[1], inner_box[2] },
		     { inner_box[3], inner_box[2] }}, box3d_colors(top))

   -- Left border.
   b3d_set_vertex(b3d.vtx, 8, {
		     { outer_box[1], outer_box[2] },
		     { inner_box[1], inner_box[2] },
		     { outer_box[1], outer_box[4] },
		     { inner_box[1], inner_box[4] }}, box3d_colors(left))

   -- Bottom border.
   b3d_set_vertex(b3d.vtx, 12, {
		     { inner_box[1], inner_box[4] },
		     { inner_box[3], inner_box[4] },
		     { outer_box[1], outer_box[4] },
		     { outer_box[3], outer_box[4] }}, box3d_colors(bottom))

   -- Right border.
   b3d_set_vertex(b3d.vtx, 16, {
		     { inner_box[3], inner_box[2] },
		     { outer_box[3], outer_box[2] },
		     { inner_box[3], inner_box[4] },
		     { outer_box[3], outer_box[4] }}, box3d_colors(right))

   return b3d
end

--
--- @}
----

if nil then
   b=box3d({50,50,200,80},
	   {-8,-6},
	   { {1,1,1,1} , {1,1,0,0}, { 0.5,0,1,0}, { 0.5, 0, 1, 1}} ,
	   {1,1,1,0},
	   {1,1,0,1},
	   {1,1,0,0},
	   {1,0,1,1})
   
   dl = dl_new_list(1024,1)
   dl_set_active(dl,1)
   box3d_draw(b,dl, mat_trans(0,0,1))

   dump(box3d_inner_box(b),"INNER-BOX")
   dump(box3d_outer_box(b),"OUTER-BOX")
   
   getchar();
   b = nil
   dl = nil
end

return 1
