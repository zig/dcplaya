--- @ingroup  dcplaya_lua_basics
--- @file     dirfunc.lua
--- @author   vincent penne <ziggy@sashipa.com>
--- @author   benjamin gerard <ben@sashipa.com>
--- @brief    Directory and filename support.
---
--- $Id: dirfunc.lua,v 1.11 2002-12-04 10:47:25 ben Exp $
---

PWD=home

--- Get path and leaf from a filename.
--- @ingroup  dcplaya_lua_basics
--- @param    pathname full 
--- @return   path,leaf
---
function get_path_and_leaf(pathname)
	if not pathname then return end
	local start,stop,path,leaf
	pathname = canonical_path(pathname)
	start,stop,path,leaf = strfind(pathname,"^(.*/)(.*)")
	if path then
		path = fullpath(path)
	elseif not leaf or leaf == "" then
		-- Find no path -- leaf only 
		leaf = pathname
	end
	if leaf and leaf == "" then leaf = nil end
	return path,leaf
end

--- Create a full path name.
--- @ingroup  dcplaya_lua_basics
--- @return   fullpath of given filename
function fullpath(name)
   if not name then
	  return PWD
   end
   if strsub(name, 1, 1) ~= "/" then
	  name = PWD.."/"..name
   end
   return canonical_path(name)
end

addhelp(fullpath,
		[[print[[fullpath(filename): return fullpath of given filename]]]])

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
