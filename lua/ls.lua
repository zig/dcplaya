-- Directory listing LUA function
--
-- by benjamin gerard <ben@sashipa.com>
--
-- $Id: ls.lua,v 1.3 2002-10-09 00:51:17 benjihan Exp $
--

-- Display a directory in optimized column format.
--
function ls_column(dir,max)
	local w,h,i,n,l

	n=getn(dir)
	w,h = consolesize()
	if max > w then max = w end
	w = floor(w / max)
	l = 10
	local f=format("%%-%ds",max)
	for i=1, n, 1 do
		local j,r
		if l >= h then getchar() l = 1 end
		j=0
		r=""
		while j < w and i <= n do
			r = r..format(f,dir[i].name)
			i = i+1
			j = j+1
		end
		print(r)
		l = l + 1;
	end
end

-- Display a directory in long format [name size], ome file per line.
--
function ls_long(dir,max)
	local w,h,i,n,l

	n=getn(dir)
	w,h = consolesize()
	if max > w then max = w end
	w = floor(w / max)
	l = 1
	local f=format("%%-%ds",max)
	for i=1, n, 1 do
		local j,r
		if l >= h then getchar() l = 1 end
		if dir[i].size < 0 then
			print(format(f,dir[i].name))
		else
			print(format(f.." %d",dir[i].name,dir[i].size))
		end
		l = l + 1;
	end
end

-- The core ls function.
--  path         : pathname to list
--  is_long      : not nil display in long format else display column format
--  sort_by_size : not nil display sorted by name, else sorted by size
--  detail       : not nil display directory summary first
--
-- $$$TODO ben:  sort_by_size must be implemented in dirlist command first
function ls_core(path,is_long,sort_by_size,detail)
	if not path then
    	path = PWD
	else
    	path=fullpath(path)
	end
	local dir
	if sort_by_size then
		dir = dirlist("-s",path)
	else
		dir = dirlist("-n",path)
	end
	if (dir) then
		local i,n,bytes,files,dirs
		n=getn(dir)
		bytes=0
		files=0
		dirs=0
		if n > 0 then
			-- Convert dirname to [dirname]
			for i=1, n, 1 do
				if dir[i].size < 0 then
					dir[i].name = dir[i].name.."/"
					dirs = dirs+1
				else
					files=files+1
					bytes=bytes+dir[i].size
				end
			end
			if detail then
				print(format("{%s}, %d directories, %d bytes in %d files",
					path,dirs,bytes,files))
			end

			-- Get longest name
			local max = strlen(dir[1].name)
			for i=2, n, 1 do
				local l = strlen(dir[i].name)
				if l > max then max = l end
			end
			max = max + 1
			if is_long then
				ls_long(dir,max)
			else
				ls_column(dir,max)
			end
		end
	end
end

-- Directory listing ls() shell function
--
-- ls [-switch] [path1 ..]
-- Valid switch are :
--   -l : long listing.
--   -s : sort by size. 
--   -d : detail, display directory summary informaion.
--
-- If no path is given, PWD is used instead.
-- If more than one path is found, detail is automatically set.
-- between `{' and `}' first.
-- 
-- If the listing is longer than console height, ls wait a key strike.
--
function ls(...)
	local is_long, sort_by_size, found, detail, i

	-- Get options
	--
	for i=1, arg.n, 1 do
		if type(arg[i]) ~= "string" then
			print("ls : invalid parameter")
			return
		elseif strsub(arg[i],1,1) == "-" then
			local j, l
			l=strlen(arg[i])
			for j=2, l, 1 do
				local p = strsub(arg[i],j,j)
				if p == "l" then
					is_long = 1
				elseif p == "s" then
					sort_by_size = 1
				elseif p == "d" then
					detail = 1
				else
					print("ls : invalid switch -"..p)
					return
				end
			end
		else
			if (found) then found=found+1 else found=1 end
		end
	end

	-- Some directory path found, list them 
	if found then
		for i=1, arg.n, 1 do
			if strsub(arg[i],1,1) ~= "-" then
				if found>1 then detail = 1 end
				ls_core(fullpath(arg[i]), is_long, sort_by_size, detail)
			end
		end
	-- No directory found, list PWD
	else
		ls_core(PWD, is_long, sort_by_size, detail)
	end
end

addhelp(
	ls,
	[[
	print[[ls(...) : file listing]]
	print[[switches (can be concatenate e.g -ld)]]
	print[[ -l : display one file per line is its size.]]
	print[[ -s : sort by size instead of name.]]
	print[[ -d : display for each directory a content summary. This option is autimatically set if more than one directory is given.]]
	 ]])

ls_loaded=1
