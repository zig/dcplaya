
-- basic things used into other library (evy, keyboard_emu, gui)



-- doubly linked list support

-- insert a new element in list as first element
-- 
-- o      : owner
-- oindex : index of the owner pointing to first element of list
-- i      : item to insert
-- iprev  : index of prev element
-- inext  : index of next element
function dlist_insert(o, oindex, i, iprev, inext)
	local f = o[oindex]
	i[iprev] = nil
	i[inext] = o[oindex]
	o[oindex] = i
	if f then
		f[iprev] = i
	end
end

-- remove an element from a list
-- 
-- o      : owner
-- oindex : index of the owner pointing to first element of list
-- i      : item to insert
-- iprev  : index of prev element
-- inext  : index of next element
function dlist_remove(o, oindex, i, iprev, inext)
	local p = i[iprev]
	local n = i[inext]
	if p then
		p[inext] = n
	else
		o[oindex] = n
	end
	if n then
		n[iprev] = p
	end

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

settagmethod(tag( {} ), "add", table_add)
settagmethod(tag( {} ), "sub", table_sub)
settagmethod(tag( {} ), "mul", table_mul)
settagmethod(tag( {} ), "pow", table_sqrdist)

