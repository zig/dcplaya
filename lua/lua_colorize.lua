--- @ingroup dclpaya_lua_gui
--- @author benjamin gerard <ben@sashipa.com>
--- @date 2003/03/20
--- @brief LUA source colorizer.
---
--- $Id: lua_colorize.lua,v 1.1 2003-03-20 21:59:19 ben Exp $
--

--- LUA keywords.
--: boolean lua_color_key_words[keywodr];
lua_color_key_words = {
   ["function"] = 1,
   ["if"] = 1,
   ["while"] = 1,
   ["do"] = 1,
   ["for"] = 1,
   ["repeat"] = 1,
   ["return"] = 1,
   ["local"] = 1,
   ["break"] = 1,
   ["if"] = 1,
   ["then"] = 1,
   ["else"] = 1,
   ["end"] = 1,
   ["elseif"] = 1,
   ["until"] = 1,
}

lua_color_cod = "LemonChiffon2"
lua_color_kwd = "LightSkyBlue2"
lua_color_rem = "IndianRed1"
lua_color_str = "MediumAquamarine"
lua_color_fnm = "orange"
lua_color_pct = "gold"
lua_color_dgt = "gold1"

lua_color_modes = {
   { -- 1 : startup
      start = '<zml><lua><p>'
	 .. '<macro macro-name="cod" macro-cmd="pre" color="$lua_color_cod">'
	 .. '<macro macro-name="kwd" macro-cmd="pre" color="$lua_color_kwd">'
	 .. '<macro macro-name="rem" macro-cmd="pre" color="$lua_color_rem">'
	 .. '<macro macro-name="str" macro-cmd="pre" color="$lua_color_str">'
	 .. '<macro macro-name="fnm" macro-cmd="pre" color="$lua_color_fnm">'
	 .. '<macro macro-name="pct" macro-cmd="pre" color="$lua_color_pct">'
	 .. '<macro macro-name="dgt" macro-cmd="pre" color="$lua_color_dgt">'
	 .. '<font id="1" size="8.5">'
	 .. '<pre id="1" tabsize="8" tabchar=" "></pre>'
   },
   { -- 2 : finish
      start = '<font id="0" size="20" color="text_color"><br>Start closing',
      stop = "<br>",
   },
   { -- 3 : code
      start = "<cod>",
      stop = '</pre>',
      flow_ctrl = function (s)
		     if not s then return "" end
 		     if strfind(s,"^<cod>.*") then
 			s = strsub(s,6)
 		     end
		     local r = "<cod>"
		     local i,l = 1,strlen(s)

		     local oi = 0

		     while i <= l do
			local a,b,c

			if i <= oi then
			   printf("flow %d %d",i,oi)
			   break
			end
			oi = i

			a,b,c = strfind(s,"([%w_])",i)
			if not a then
			   -- no more words nor number !
			   break
			end

			if c == "" then
			   print("NULL")
			   break
			end

--			r = r .. gsub(strsub(s,i,a-1),"(%p+)","<pct>%1")
			if a > i then
			   r = r .. "</pre><cod>" .. strsub(s,i,a-1)
			end

			if strfind(c,"%d") then
			   local number = "%-?%d+[.]?%d*[Ee]?%-?%d*"
			   a,b = strfind(s,number, a)
			   r = r .. gsub(strsub(s,a,b),"(.*)","</pre><dgt>%1")
			   i = b + 1
			else
			   a,b,c,d = strfind(s,"([%w_]*)(%s*)", a)
			   if not a then
			      print ("ERROR")
			      return ""
			   end
			   if lua_color_key_words[c] then
			      r = r .. '</pre><kwd>'.. c .. d
			   else
			      r = r .. '</pre><cod>' .. c .. d
			   end
			   
			   i = b + 1
			end
		     end

		     if i <= l then
			r = r .. "</pre><cod>" .. strsub(s,i)
		     end

		     return r
		  end,
      no_empty = 1
   },
   { -- 4 : comment
      start = "<rem>--",
      stop = "</pre>",
      add = 2
   },
   { -- 5 : [[
      start = "<str>[[",
      stop = "]]</pre>",
      add = 2
   },
   { -- 6 : ""
      start = '<str>"',
      stop = '"</pre>',
      add = 1
   },
   { -- 7 : ''
      start = "<str>'",
      stop = "'</pre>",
      add = 1
   },
}

function luacolor_mode(flow, mode)
   if mode ~= flow.mode then
      if flow.mode then
	 if lua_color_modes[flow.mode].no_empty and
	    flow.text == lua_color_modes[flow.mode].start then
	    -- nothing to do
	 else
	    if type(lua_color_modes[flow.mode].flow_ctrl) == "function" then
	       flow.format = flow.format .. 
		  lua_color_modes[flow.mode].flow_ctrl(flow.text)
	    else
	       flow.format = flow.format .. flow.text
	    end
	    if lua_color_modes[flow.mode].stop then
	       flow.format = flow.format .. lua_color_modes[flow.mode].stop
	    end
	 end
      end
      flow.text = (mode and lua_color_modes[mode].start) or ""
      flow.mode = mode
   end
end

function luacolor_file(fname)
   local file = openfile(fname,"rt")
   if not file then return end
   local flow = { format = "", text = "" }

   local brk
   local line

   -- startup
   luacolor_mode(flow,1)
   luacolor_mode(flow,3)

   line = read(file)
   while line do
      line = line .. "\n"
      local start,len = 1, strlen(line)
      local os = 0

      while start <= len do

	 if os >= start then
	    printf("UNDERFLOW %d %d", os,start)
	    return
	 end
	 os = start

	 if flow.mode == 3 then
	    -- CODE --
	    local cs,ce = strfind(line,"['\"[-]", start)
	    local newmode
	    if cs then
	       local c = strsub(line,cs,ce)
	       if c == '"' then
		  newmode = 6
		  brk = c
	       elseif c == "'" then
		  newmode = 7
		  brk = c
	       else
		  local c2 = strsub(line,cs+1,ce+1)
		  if c2 and c2 == c then
		     if c == '-' then
			newmode = 4
		     else
			newmode = 5
			brk = ']]'
		     end
		     c = c..c
		  end
	       end
	    else
	       flow.text = flow.text .. strsub(line, start)
	       break
	    end
	    if newmode then 
	       flow.text = flow.text .. strsub(line, start, cs-1)
	       start = cs + (lua_color_modes[newmode].add or 0)
	       luacolor_mode(flow, newmode)
	    else
	       flow.text = flow.text .. strsub(line, start, cs)
	       start = cs + 1
	    end
	 elseif flow.mode == 4 then
	    -- REM --
	    flow.text = flow.text .. strsub(line, start)
	    luacolor_mode(flow, 3)
	    break
	 else
	    local bs,be = strfind(line,brk,start)
	    if not bs then
	       flow.text = flow.text .. strsub(line, start)
	       break
	    end
	    flow.text = flow.text .. strsub(line, start, bs-1)
	    luacolor_mode(flow, 3)
	    start = be+1
	 end
      end
      line = read(file)
      
   end

   -- finish
   luacolor_mode(flow,2)
   luacolor_mode(flow,nil)
   return flow.format
end

return 1
