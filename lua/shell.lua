
-- We reimplement the doshellcommand function to extend the shell syntax.
-- The new syntax is backward compatible with lua normal syntax (? I hope !)
-- and add possibility to type more usual command like "ls" without trailing ()
-- or "cd plugins" without the string quotes.
-- This only replace syntax at shell level, the lua syntax remains unchanged
-- so lua script should use normal syntax ...
function doshellcommand(string)
  list = {}
  local i,j
  j=0
  repeat
    i, j = strfind(string, [[[^ ]+]], j+1)
    if i then
      local beg = strsub(string, i, i)
      if beg == [["]] or (beg == "[" and strsub(string, i+1, i+1) == "[") then
        -- this is a quoted argument (a string), reparse because we may have space into
        local n = 1
        local e = beg
        if beg == "[" then
          n = 2
          e = "]"
        end
	--print (i, j, beg, e)
        i, j = strfind(string, [[%b]]..beg..e, i)
        tinsert(list, strsub(string, i+n, j-n))
      else
        tinsert(list, strsub(string, i, j))
      end
      --print (getn(list), list[getn(list)])
    end
  until not i

  if list[1] then

    local sec
    if list[2] then sec=strsub(list[2], 1, 1) end
    if sec == [[(]] or sec == [[=]] or strfind(list[1], "[(=]") then
      -- plain LUA command
      return dostring(string)
    else
      -- shellized command
      local f = getglobal(list[1])
      tremove(list, 1)
      return call (f, list)
    end

  end

end
