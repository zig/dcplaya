--- @ingroup dcplaya_lua_gui
--- @file    style.lua
--- @author  benjamin gerard <ben@sashipa.com>
--- @date    2002/12/17
--- @brief   gui styles.
--- 
-- 

style_loaded = nil

if not dolib("color") then return end

style_current = nil
style_tag = style_tag or newtag()  -- style tag
style_counter = style_counter or 0 -- for unnamed style

--- Style definition.
--- @ingroup dcplaya_lua_gui
--
--- struct style {
---   static tag style_tag;       ///< Tag for style table.
---   static style style_current; ///< Current style.
---   string name;                ///< Style name.
---   table colors;          ///< Style color table is composed with 4 colors.
---   color get_color(number x, number y); ///< Get style color.
--- };


--
--- @name Styles
--- @ingroup dcplaya_lua_gui
--- @{
--

--- Add a new style.
---
--- @param  style  style object.
---
--- @see style_remove()
--- @see style_create()
function style_add(style)
   if tag(style) ~= style_tag then return end
   if not styles then
      styles = {}
   end
   styles[style.name] = style
   --   dump(style, "New-Style")
   style_current = style_current or style
end

--- Remove existing style.
---
--- @param  style  Style object or style name to remove.
---
--- @see style_add()
function style_remove(style)
   if type(style) == "table" then style = style.name end
   if type(style) ~= "string" then return end
   styles[style] = nil
end

--- Get a style.
---
--- @param  name  Style name or style, default:current style.
---
--- @return style
function style_get(name)
   local style
   if type(name) == "string" then style = styles[name] 
   elseif tag(name) == style_tag then style = name end
   style = style or style_current
   return style
end

-- text   [ font, size, aspect, filter, color ]
-- border [ size, top.colors, left.colors, bottom.colors, right.colors ]
-- background [ colors ]

function style_name_to_index(name)
   local index = {}
   local pos
   pos = strfind(name,".",1,1)
   while pos do
      tinsert(index,strsub(name,1,pos))
      name = strsub(name,pos+1)
      pos = strfind(name,".",1,1)
   end
   tinsert(index,name)
   return index
end

--- Create and add a style.
---
--- @param  name  style name
--- @param  color_o   Origine color (default: {1,0,0,0})
--- @param  color_x   Color on X-axis (default: {1,1,1,1})
--- @param  color_y   Color on Y-axis (default: color_x)
--- @param  color_xy  Color on X/Y-axis (default: (color_x+color_y) * 0.5)
---
--- @see style_add()
--
function style_create(name, color_o, color_x, color_y, color_xy)

   if not styles then
      styles = {}
   end
   style_counter = style_counter+1
   name = name or format("unnamed%03d",style_counter)
   color_o = color_o or color_new(1,0,0,0)
   color_x = color_x or color_new(1,1,1,1)
   color_y = color_y or color_x
   color_xy = color_xy or (color_x * 0.5 + color_y * 0.5)

   local style = {
      name = name,
      colors = { color_o, color_x, color_y, color_xy },
      get_color = style_get_color,
   }
   settag(style,style_tag)
   style_add(style)
end

--- Default style get_color method. 
--- @internal
---
--- @param  style  style object
--- @param  x      X-axis coordinate [0..1], default:0
--- @param  y      Y-axis coordinate [0..1], default:0
---
--- @return color
--
function style_get_color(style,x,y)
   local colors = style_get_prop(style, "colors")
   local a,b,c,d =
      colors[1], colors[2], colors[3], colors[4]
   x = x or 0
   y = y or x
   -- (ABCD) = (1-Y-X+XY)*A + (X-XY)*B + (Y-XY)*C + XY*D
   return color_new((1-y-x+x*y) * a + (x-x*y) * b + (y-x*y) * c + (x*y) * d)
end

--- Get a style properties.
---
--- @param  style  style object
--- @param  prop   properties name
---
--- @return properties value for a given style.
--
function style_get_prop(style, prop)
   local sp = (tag(style) == style_tag) and style[prop];
   if tag(sp) == style_tag then
      sp = style_get_prop(sp, prop)
   end
   if not sp and style ~= style_current then
      sp = style_get_prop(style_current, prop)
   end
   return sp
end

--
--- @}
----

style_add(style_create("fire",
		       color_new(1,0,0,0),
		       color_new(1,1,1,0),
		       color_new(1,1,0,0))
       )

style_loaded = 1
return style_loaded
