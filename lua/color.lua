-- dcplaya - lua color type
--
-- author : benjamin gerard <ben@sashipa.com>
-- date   : 2002/10/14
--
-- $Id: color.lua,v 1.1 2002-10-14 16:23:09 benjihan Exp $
--

color_loaded = nil

-- Loaded required libraries
--
if not dolib("basic") then return end

-- Create a new tag for color (once).
--
if not color_tag then color_tag = newtag() end

-- Create name to index table for .a .r .g .b access
--
if not color_name_to_index then
	color_name_to_index = { ["a"]=1, ["r"]=2, ["g"]=3, ["b"]=4 }
end

-- Create a new color object
--
function color_new(a,r,g,b)
	if type(a) == "table" then
		b = rawget(a,4)
		g = rawget(a,3)
		r = rawget(a,2)
		a = rawget(a,1)
	end
	if not a then a = 1 end
	if not r then r = 0 end
	if not g then g = 0 end
	if not b then b = 0 end
	local c = { a, r, g, b}
	settag(c,color_tag)
	color_clip(c)
	return c
end

-- Clip color components.
--  If "positive" is not nil range is [0..1] else range is [-1..1]
function color_clip(c, positive)
	local min = -1
	if positive then min = 0 end
	rawset(c,1,clip_value(rawget(c,1),min,1))
	rawset(c,2,clip_value(rawget(c,2),min,1))
	rawset(c,3,clip_value(rawget(c,3),min,1))
	rawset(c,4,clip_value(rawget(c,4),min,1))
end

-- Get color luminosity
--
function color_lum(c)
	if tag(c) == color_tag then
		return getraw(c,2)*0.5 + getraw(c,3)*0.3 + getraw(c,4)*0.2
	end
end

-- Get index of first hightest component
--
function color_max(c)
	if tag(c) == color_tag then
		return table_max(c)
	end
end

-- Get index of first lowest component
--
function color_min(color)
	if tag(color) == color_tag then
		return table_min(color)
	end
end

-- Scale color to have the highest component equal to 1
--
function color_maximize(c)
	local max = color_max(c)
	if max and rawget(c,max) > 0.0001 then
		local scale=rawget(c,max)
		rawset(c,1,rawget(c,1)/scale)
		rawset(c,2,rawget(c,2)/scale)
		rawset(c,3,rawget(c,3)/scale)
		rawset(c,4,rawget(c,4)/scale)
	else
		rawset(c,1,1)
		rawset(c,2,1)
		rawset(c,3,1)
		rawset(c,4,1)
	end
end

-- Transform color to string
--
function color_tostring(c)
	if tag(c) == color_tag then
		return format("{ %5.4f, %5.4f, %5.4f, %5.4f }",
			rawget(c,1),rawget(c,2),rawget(c,3),rawget(c,4))
	end
end

-- "concat" method
-- 
function color_concat(a,b)
	local s1,s2
	s1 = color_tostring(a) or tostring(a)
	s2 = color_tostring(b) or tostring(b)
	return s1..s2
end

-- "index" method
--
function color_index(c,i)
	if tag(c) == color_tag then
		local idx = color_name_to_index[i]
		if idx then	return c[idx] end
	end
end

-- "settable" method
function color_settable(c,i,v)
	if tag(c) == color_tag then
		if type(i) == "string" then
			i = color_name_to_index[i]
		end
		if type(i) == "number" and i>=1 and i<=4 then
			rawset(c,i,v)
		end
	end
end

-- Overload "tostring()" function
--
function tostring(c)
	return color_tostring(c) or %tostring(color)
end

function color_add(a,b)
	local r=table_add(a,b)
	if type(r) == "table" then
		settag(r,color_tag)
		color_clip(r)
	end
	return r
end

function color_sub(a,b)
	local r=table_sub(a,b)
	if type(r) == "table" then
		settag(r,color_tag)
		color_clip(r)
	end
	return r
end

function color_mul(a,b)
	local r=table_mul(a,b)
	if type(r) == "table" then
		settag(r,color_tag)
		color_clip(r)
	end
	return r
end

function color_div(a,b)
	local r=table_div(a,b)
	if type(r) == "table" then
		settag(r,color_tag)
		color_clip(r)
	end
	return r
end

function color_minus(a,b)
	local r=table_minus(a,b)
	if type(r) == "table" then
		settag(r,color_tag)
		color_clip(r)
	end
	return r
end

-- Absolutes color components
--
function color_abs(c)
	if tag(c) == color_tag then
		rawset(c,1,abs(rawget(c,1)))
		rawset(c,2,abs(rawget(c,2)))
		rawset(c,3,abs(rawget(c,3)))
		rawset(c,4,abs(rawget(c,4)))
	end
end

-- Set tag method for color object
--
settagmethod(color_tag, "add",		color_add)
settagmethod(color_tag, "sub",		color_sub)
settagmethod(color_tag, "mul",		color_mul)
settagmethod(color_tag, "div",		color_div)
settagmethod(color_tag, "pow",		table_sqrdist)
settagmethod(color_tag, "unm",		color_minus)
settagmethod(color_tag, "concat",	color_concat)
settagmethod(color_tag, "index",	color_index)
settagmethod(color_tag, "settable",	color_settable)

color_loaded = 1
return color_loaded
