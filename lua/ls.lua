--- @ingroup dcplaya_lua_shell
--- @file    ls.lua
--- @author  benjamin gerard
--- @brief   Directory listing core function.
---
--- $Id: ls.lua,v 1.7 2003-03-28 14:01:44 ben Exp $
---

-- Display a directory in optimized column format.
--
function ls_column(dir, maxi, nowait)
   if type(dir) ~= "table" then return end
   local i,name
   local w,h = consolesize()
   if type(maxi) ~= "number" then
      maxi = 0
      for i,name in dir do
	 if type(name) == "table" and type(i) == "number" then
	    name = name.name
	 end
	 if type(name) == "string" then
	    maxi = max(maxi,strlen(name))
	 end
      end
      maxi = maxi + 1
   end
   h = h - 2
   if maxi < 1 then maxi = 1 end
   w = floor(w / maxi)
   local f=format("%%-%ds",maxi)
   local l,j,r = 0,0,""
   for i,name in dir do
      if type(name) == "table" and type(i) == "number" then
	 name = name.name
      end
      if type(name) == "string" then
	 if j >= w then
	    j = 0
	    l = l + 1
	    if l >= h then
	       l = 0
	       if not nowait and getchar then getchar() end
	    end
	    print(r)
	    r = ""
	 end
	 r = r..format(f,name)
	 j = j+1
      end
   end
   if j > 0 then
      print(r)
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
	 local maxi = 0
	 for i=1, n, 1 do
	    maxi = max(maxi,strlen(dir[i].name))
	 end
	 maxi = maxi + 1
	 if is_long then
	    ls_long(dir,maxi)
	 else
	    dir.path = nil -- hack, remove path for ls_column !
	    ls_column(dir,maxi)
	 end
      end
   end
end


ls_loaded=1
return ls_loaded

