--- @ingroup  dcplaya_lua_shell
--- @file     dirfunc.lua
--- @author   vincent penne <ziggy@sashipa.com>
--- @author   benjamin gerard <ben@sashipa.com>
--- @brief    Directory and filename support.
---
--- $Id: dirfunc.lua,v 1.15 2003-03-14 18:51:03 ben Exp $
---

PWD = PWD or home or "/"

--- @name Directory and file functions.
--- @ingroup dcplaya_lua_shell
--- @{
--

--- Get path and leaf from a filename.
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
--- @warning  No check for new path validity.
function cd(path)
	PWD = fullpath(path)
	if strsub(PWD, -1)~="/" then
		PWD=PWD.."/"
	end
	print("PWD="..PWD)
end
addhelp(cd, [[print[[cd(path) : set current directory]]]])


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
--- @param    path  Directory to delete
---
function deltree(path)
   path = canonical_path(path)
   if type(path) ~= "string" then
      print("deltree : bad argument")
      return
   end
   print("deltree "..path)
   if not test("-d",path) then
      print("deltree : [".. tostring(path) .. "] is not a directory.")
      return
   end

   local d,f = dirlist("-2h",path)
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

if not dolib("ls") then return end

--- Directory listing.
---
--- Display directory listing. If no path is given, @b PWD is used instead.
--- If the listing is longer than console height, ls wait a key strike.
--- If more than one path is found, detail [-d] is automatically set.
---
--- @par Syntax
--- @code
--- ls [-switch] [path1 ..]
--- @endcode
---
--- @par Switch
---
--- Valid switch are :
---   - @b -l : long listing.
---   - @b -s : sort by size. 
---   - @b -d : detail, display directory information summary.
---
--- Switch can be concatenate.
--- @code
--- ls -ld /ram /vmu
--- @endcode
--- 
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

--
--- @}
--

addhelp("deltree", [[
print[[deltree(path) : Recursively delete a directory and its files
]]]])


addhelp("fullpath_convert", [[
print[[fullpath_convert(function_name[, arglist]) : 
Reimplement given function with relative path support by converting its input parameters.

- function_name is the name of the function to reimplement
- arglist is the list of index of argument to convert or nil to convert all arguments
]]]])

addhelp(
	ls,
	[[
	      print[[ls(...) : file listing]]
	      print[[switches (can be concatenate e.g -ld)]]
	      print[[ -l : display one file per line is its size.]]
	      print[[ -s : sort by size instead of name.]]
	      print[[ -d : display for each directory a content summary. This option is autimatically set if more than one directory is given.]]
	]])

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
