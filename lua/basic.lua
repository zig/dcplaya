-- basic things used into other library (evt, keyboard_emu, gui)
--
-- author : vincent penne <ziggy@sashipa.com>
--
-- $Id: basic.lua,v 1.7 2002-10-15 06:02:47 benjihan Exp $
---

-- Unload library
basic_loaded=nil

-- doubly linked list support

-- insert a new element in list as first element (or as last simply by swaping
-- ofirst and olast, iprev and inext)
-- 
-- o      : owner
-- ofirst : index in the owner pointing to first element of list
-- olast  : index in the owner pointing to last element of list
-- i      : item to insert
-- iprev  : index in the item of prev element
-- inext  : index in the item of next element
-- iowner : index in the item of owner element
function dlist_insert(o, ofirst, olast, i, iprev, inext, iowner)
	local f = o[ofirst]
	i[iprev] = nil
	i[inext] = f
	o[ofirst] = i
	if f then
		f[iprev] = i
	end
	if not o[olast] then
		o[olast] = i
	end
	i[iowner] = o
end

-- remove an element from a list
-- 
-- ofirst : index in the owner pointing to first element of list
-- olast  : index in the owner pointing to last element of list
-- i      : item to insert
-- iprev  : index in the item of prev element
-- inext  : index in the item of next element
-- iowner : index in the item of owner element
function dlist_remove(ofirst, olast, i, iprev, inext, iowner)
	local o = i[iowner]
	if not o then
		return
	end
	local p = i[iprev]
	local n = i[inext]
	if p then
		p[inext] = n
	else
		o[ofirst] = n
	end
	if n then
		n[iprev] = p
	else
		o[olast] = p
	end

	i[iowner] = nil
	i[iprev] = nil
	i[inext] = nil
end


-- define +, -, * and ^ operators for tables
-- this is not complete ...
-- the ^ operator calculate the square distances between two tables

function table_sqrdist(t1, t2)
	local sum = 0
	local i, v
	for i, v in t1 do
		local d = v - t2[i]
		sum = sum + d*d
	end
	
	return sum
end

function table_add(t1, t2)
	local r = {}
	local i, v
	for i, v in t1 do
		r[i] = v + t2[i]
	end
	
	return r
end

function table_sub(t1, t2)
	local r = {}
	local i, v
	for i, v in t1 do
		r[i] = v - t2[i]
	end
	
	return r
end

function table_mul(a, t)
	local r = {}
	local i, v
	if type(a) == "table" then
		a, t = t, a
	end
	if type(a) == "table" then
		-- two tables case
		for i, v in a do
			r[i] = v * t[i]
		end
	else
		-- number * table case
		for i, v in t do
			r[i] = v * a
		end
	end
	
	return r
end

function table_div(a, t)
	local r = {}
	local i, v
	if type(a) == "table" then
		if type(t) == "table" then
			-- two tables case
			for i, v in a do
				r[i] = v / t[i]
			end
		else 
		-- tables / number
			for i, v in a do
				r[i] = v / t
			end
		end
	else
		-- number / table case
		for i, v in t do
			r[i] = a / v
		end
	end
	return r
end

function table_minus(a)
	local r = nil
	local i, v

	if type(a) == "table" then
		r = {}
		for i, v in a do
			r[i] = -v
		end
	end
	return r
end

function table_max(a)
	local imax = nil
	if type(a) == "table" and getn(a) > 0 then
		local i, v, max
		imax = 1
		max = a[1]
		for i, v in a do
			if (v > max) then
				imax = i
				max = v
			end
		end
	end
	return imax
end

function table_min(a)
	local imin = nil
	if type(a) == "table" and getn(a) > 0 then
		local i, v, min
		imin = 1
		min = a[1]
		for i, v in a do
			if (v < min) then
				imin = i
				min = v
			end
		end
	end
	return imin
end

-- Duplicate data 
--
function dup(v)
	local t = type(v)

	if t == "table" then
		local tbl = {}
		local i,w
		for i,w in v do
			rawset(tbl,i,dup(w))
		end
		if tag(tbl) ~= tag(v) then settag(tbl,tag(v)) end
		return tbl
	else
		return v
	end
end

-- Get a lua compatible string describing this object.
--
function type_dump(v, name, indent)
	if not indent then indent = 0 end
	local t = type(v)
	local istr = strrep(" ",indent*2)
	local s = istr
	if type(name) == "string" then
		s = s..format("[%q]=",name)
	end

	if t == "number" then
		 s=s..v
	elseif t == "string" then
		s=s..format("%q",v)
	elseif t == "table" then
		s=s.."{\n"
		local i,w
		for i,w in v do
			s=s..type_dump(w,i,indent+1)..",\n"
		end
		s=s..istr.."}"
	elseif t == "function" then
		s=s..getinfo(v).name
	else
		local a = tostring(v)
		if type(a) == "string"  then s=s..a else s=s.."???" end
	end
	return s
end

function clip_value(v,min,max)
	if min and v < min then v = min end
	if max and v > max then v = max end
	return v
end

settagmethod(tag( {} ), "add", table_add)
settagmethod(tag( {} ), "sub", table_sub)
settagmethod(tag( {} ), "mul", table_mul)
settagmethod(tag( {} ), "div", table_div)
settagmethod(tag( {} ), "pow", table_sqrdist)
settagmethod(tag( {} ), "unm", table_minus)

basic_loaded=1
return basic_loaded
