--- @ingroup dcplaya_lua_shell
--- @file    ls.lua
--- @author  benjamin gerard
--- @brief   Directory listing core function.
---
--- $Id: ls.lua,v 1.6 2003-03-26 23:02:49 ben Exp $
---

-- Display a directory in optimized column format.
--
function ls_column(dir,maxi, nowait)
   local w,h,i,n,l
   if type(dir) ~= "table" then return end
   w,h = consolesize()
   maxi = min(w, maxi or floor(w/3))
   n=getn(dir)
   w = floor(w / maxi)
   l = 10
   local f=format("%%-%ds",maxi)
   for i=1, n, 1 do
      local j,r
      if l >= h then
	 l = 1
	 if not nowait then getchar() end
      end
      j=0
      r=""
      while j < w and i <= n do
	 local name
	 if type(dir[i]) == "table" then
	    name = dir[i].name
	 end
	 if type(name) ~= "string" then
	    name = tostring(dir[i])
	 end
	 r = r..format(f,name)
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
      dir = dirlist("-sh",path)
   else
      dir = dirlist("-nh",path)
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


ls_loaded=1
return ls_loaded

