-- implement directory support

PWD=home

-- return fullpath of given filename
function fullpath(name)

	if not name then
		return PWD
	end

	if strsub(name, 1, 1) ~= "/" then
		name = PWD..name
--		print ("Relative : ", name)
	end

	local i, j
	-- remove relative parts
	repeat
		i, j=strfind(name, "/[^/]+/%.%.")
		if i then
			name = strsub(name, 1, i-1) .. strsub(name, j+1)
--			print ("Found pair ", i, j, name)
		end
	until not i

	return name

end
addhelp(fullpath, [[print[[fullpath(filename): return fullpath of given filename]]]])

function cd(path)
	PWD = fullpath(path)
	if strsub(PWD, -1)~="/" then
		PWD=PWD.."/"
	end	
	print("PWD="..PWD)
end
addhelp(cd, [[print[[cd(path) : set current directory]]]])



function ls(path)

	if not path then
		path = PWD
	else
		path=fullpath(path)
	end

	local l = strlen(path)

	local list = dirl(path, "", 1)
	print("Files in "..path)
	local i
	local j
	local n=getn(list)
	local w
	local h
	w,h = consolesize()
	h = h-2
	for i=0,n/h, 1 do
		for j=1, h, 1 do
			local k = i*h + j
			if list[k] then
				print (strsub(list[k], l+2))
			end
		end
		if list[(i+1)*h] then
			getchar()
		end
	end

end
addhelp(ls, [[print[[ls([path]) : list files into current or given directory]]]])


-- reimplement basic io function with relative path support
function openfile(filename, ...)
	return %openfile(fullpath(filename), arg)
end
function readfrom(filename, ...)
	return %readfrom(fullpath(filename), arg)
end
function writeto(filename, ...)
	return %writeto(fullpath(filename), arg)
end
function appendto(filename, ...)
	return %appendto(fullpath(filename), arg)
end
function remove(filename, ...)
	return %remove(fullpath(filename), arg)
end
function rename(from, to)
	return %rename(fullpath(from), fullpath(to))
end

