--- @ingroup  dcplaya_lua_basics
--- @file     dirfunc.lua
--- @author   vincent penne <ziggy@sashipa.com>
--- @author   benjamin gerard <ben@sashipa.com>
--- @brief    Directory and filename support.
---
--- $Id: dirfunc.lua,v 1.14 2003-03-03 08:35:24 ben Exp $
---

PWD=home

--- Get path and leaf from a filename.
--- @ingroup  dcplaya_lua_basics
--- @param    pathname  full path
--- @param    pwd       optionnal pwd, if omitted PWD global is used.
--- @return   path,leaf
---
function get_path_and_leaf(pathname,pwd)
   if not pathname then return end
   local start,stop,path,leaf
   pathname = canonical_path(pathname)
   start,stop,path,leaf = strfind(pathname,"^(.*/)(.*)")
   if path then
      path = fullpath(path,pwd)
   elseif not leaf or leaf == "" then
      -- Find no path -- leaf only 
      leaf = pathname
   end
   if leaf and leaf == "" then leaf = nil end
   return path,leaf
end

addhelp(get_path_and_leaf,
	[[print[[get_path_and_leaf(pathname[,pwd]): return path,leaf from given filename. pwd is an optionnal pwd. See fullpath.]]]])

--- Get nude name (remove path and extension).
--- @ingroup  dcplaya_lua_basics
--- @param    pathname full 
--- @return   path,leaf
function get_nude_name(pathname)
   local path,leaf = get_path_and_leaf(pathname)
   if not leaf then
      return
   else
      local a,b,name = strfind(leaf,"(.+)[.].*")
      return name
   end
end

addhelp(get_nude_name,
	[[print[[get_nude_name(pathname): return nude filename (without path nor extension).]]]])

--- Create a full path name.
--- @ingroup  dcplaya_lua_basics
--- @return   fullpath of given filename
function fullpath(name, pwd)
   pwd = pwd or PWD
   if not name then
      return pwd
   end
   if strsub(name, 1, 1) ~= "/" then
      name = pwd.."/"..name
   end
   return canonical_path(name)
end

addhelp(fullpath,
	[[print[[fullpath(filename[,pwd]): return fullpath of given filename. Optionnal pwd will be prefixed instead of defaut PWD global variable.]]]])

--- Change current directory.
--- @ingroup  dcplaya_lua_basics
--- @warning  No check for new path validity.
function cd(path)
	PWD = fullpath(path)
	if strsub(PWD, -1)~="/" then
		PWD=PWD.."/"
	end
	print("PWD="..PWD)
end
addhelp(cd, [[print[[cd(path) : set current directory]]]])

-- Load the new ls() command
dolib("ls")

--
-- reimplement basic io function with relative path support
--

-- template reimplementation function
--
-- name is the name of the function to reimplement
-- arglist is the list of index of argument to convert or nil to convert all
-- arguments, in that case '-' starting arguments are ignored.
function fullpath_convert(name, arglist)
   local new
   local old = getglobal(name)

   if arglist then
      new =	function (...)
		   local i, v
		   for i, v in %arglist do
		      arg[v] = fullpath(arg[v])
		   end
		   return call (%old, arg)
		end
   else
      new =	function (...)
		   local i
		   for i=1, arg.n, 1 do
		      if strsub(arg[i], 1, 1) ~= "-" then
			 arg[i] = fullpath(arg[i])
		      end
		   end
		   return call (%old, arg)
		end
   end
   
   setglobal(name, new)
end

--- Recursively delete a directory and its files.
--- @ingroup  dcplaya_lua_basics
--- @param    path  Directory to delete
---
function deltree(path)
   path = fullpath(path)

   if not test("-d",path) then
      print("deltree : [".. tostring(path) .. "] is not a directory.")
      return
   end

   local d,f = dirlist("-2",path)
   local i,v

   -- Unlink files
   for i,v in f do
      if type(v) == "string" then
	 unlink(path .. "/" .. v)
      end
   end

   -- Recurse sub-dir
   for i,v in d do
      if type(v) == "string" then
	 deltree(path .. "/" .. v)
      end
   end

   -- Unlink path
   unlink(path)
   return
end

addhelp("deltree", [[
print[[deltree(path) : Recursively delete a directory and its files
]]]])


addhelp("fullpath_convert", [[
print[[fullpath_convert(function_name[, arglist]) : 
Reimplement given function with relative path support by converting its input parameters.

- function_name is the name of the function to reimplement
- arglist is the list of index of argument to convert or nil to convert all arguments
]]]])


fullpath_convert( "openfile", { 1 } )
fullpath_convert( "readfrom", { 1 } )
fullpath_convert( "writeto", { 1 } )
fullpath_convert( "appendto", { 1 } )
fullpath_convert( "remove" )
fullpath_convert( "dofile" )
fullpath_convert( "mkdir" )
fullpath_convert( "md" )
fullpath_convert( "unlink" )
fullpath_convert( "rm" )
fullpath_convert( "copy" )
fullpath_convert( "cp" )
fullpath_convert( "rename" )

dirfunc_loaded=1
return dirfunc_loaded
