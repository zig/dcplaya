
-- basic things used into other library (evy, keyboard_emu, gui)



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

settagmethod(tag( {} ), "add", table_add)
settagmethod(tag( {} ), "sub", table_sub)
settagmethod(tag( {} ), "mul", table_mul)
settagmethod(tag( {} ), "pow", table_sqrdist)

