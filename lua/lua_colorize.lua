--- @ingroup dclpaya_lua_gui
--- @file    lua_colorize.lua
--- @author  benjamin gerard <ben@sashipa.com>
--- @date    2003/03/20
--- @brief   LUA source colorizer.
---
--- $Id: lua_colorize.lua,v 1.3 2003-03-23 08:04:03 ben Exp $
--

--- @defgroup  dcplaya_lua_colorize  LUA source colorizer
--- @ingroup   dclpaya_lua_gui
--- @brief     creates preformatted colorized zml for lua source viewer
---
--- @author  benjamin gerard <ben@sashipa.com>
--- @{

--- LUA keywords.
--: boolean lua_color_key_words[keywodr];
lua_color_key_words = {
   ["and"] = 1,
   ["break"] = 1,
   ["do"] = 1,
   ["else"] = 1,
   ["elseif"] = 1,
   ["end"] = 1,
   ["for"] = 1,
   ["function"] = 1,
   ["if"] = 1,
   ["in"] = 1,
   ["local"] = 1,
   ["nil"] = 1,
   ["not"] = 1,
   ["or"] = 1,
   ["repeat"] = 1,
   ["return"] = 1,
   ["then"] = 1,
   ["until"] = 1,
   ["while"] = 1,
}

--: color lua_color_cod; ///< color used for code (default color).
lua_color_cod = "wheat" --"LemonChiffon2"
--: color lua_color_kwd; ///< color used for keyword.
lua_color_kwd = "cyan" --"LightSkyBlue2"
--: color lua_color_rem; ///< color used for comment.
lua_color_rem = "chocolate1" --"IndianRed1"
--: color lua_color_str; ///< color used for string.
lua_color_str = "LightSalmon1" --"MediumAquamarine"
--: color lua_color_fnm; ///< color used for function name.
lua_color_fnm = "LightSkyBlue"
--: color lua_color_fdf; ///< color used for function definition.
lua_color_fdf = "yellow" --"DarkSeaGreen1"
--: color lua_color_pct; ///< color used for punctuation.
lua_color_pct = "white"
--: color lua_color_digit; ///< color used for number.
lua_color_dgt = "gold1"

--- Line number format string (nil for none)
--: string lua_color_line_number;
lua_color_line_number = "%03d "

--- Maximum number of line.
--: number lua_color_max_line;
lua_color_max_line = 1000

function luacolor_codeflow_change(flow, newmode)
   local code = flow.code

   if code.mode ~= newmode then
      if code.text ~= "" then
	 code.format = code.format
	    .. code.mode .. code.text .. "</pre>"
	 code.fctdef = (newmode == "<kwd>" and code.kwd_fct) or
	    (code.fctdef and code.mode == "<kwd>" and newmode=="<cod>")

	 code.kwd_fct = nil
	 code.text = ""
      end
      code.mode = newmode
   end
end

function luacolor_dump_codeflow(flow)
   printf("Flow line %d-%d:",flow.mode_line,flow.line)
   printf("Current mode: [%s]",code.mode)
   printf("Current text: [%s]",strsub(code.text,-60))
   print("Current format:")
   print(strsub(code.format,-512))
end

function luacolor_codeflow (flow)
   local i,l = 1,strlen(flow.text)
   if l < i then
      -- Empty code node discarded
      return ""
   end
   local code = {}
   flow.code = code

   -- Setup
   code.mode = "<cod>" -- Current mode (opening tag)
   code.format = ""    -- Current formatted code output
   code.text = ""      -- Current mode text

   local oi = 0 -- Backstream detection
   while i <= l do
      local a,b,c,d,e,w

      if i <= oi then
	 printf("[luacolor_codeflow] : internal backward stream [%d %d]",
		i,oi)
	 luacolor_dump_codeflow(flow)
	 return -- Generate an concate error !
      end
      oi = i

      local newmode,best
      -- Find word/number starting position (and get char)
      a,b,c = strfind(flow.text, "([%w_])",i)

      -- Find punctuation string [considere space has punctuation here]
      d,e,w = strfind(flow.text, "([%p%s]+)", i)

      if d then
	 -- Get punctuation as default best.
	 best = d
	 -- e is the end, no nee to set it
	 newmode = "<pct>"

	 -- Check for function call here if we had an opening parenthesys.
	 if code.mode == "<cod>" and strfind(w,"[%s]*[(]") then
	    code.mode = code.fctdef and "<fdf>" or "<fnm>"
	 end
      end

      if a then
	 -- have some wordz too !
	 if not best or a < best then
	    -- Word was found before punctuation, it is our winner :P
	    best = a
	    -- Now we should determine what kind of word it is.

	    if strfind(c,"%d") then
	       -- It is a number 
	       newmode = "<dgt>"
	       a,e = strfind(flow.text, "%d+[.]?%d*[Ee]?%-?%d*", a)
	    else
	       -- It is a word, get it and trailing space too !
	       a,e,w,d = strfind(flow.text, "([%w_]*)(%s*)", a)

	       if lua_color_key_words[w] then
		  newmode = "<kwd>"
		  code.kwd_fct = w == "function"
	       else
		  newmode = "<cod>"
		  code.kwd_fct = code.kwd_fct and code.kwd_fct + 1
	       end

	    end
	 end
      end


      -- Well here we should have a new mode, indeed it is possible that
      -- we have none. This could happen if we make an error in the regexpr
      -- or more likely on unexpected characters. In that case we will
      -- simply advance in the current mode if there is none, we crates
      -- a default <cod> node.
      if not newmode then
	 break
      end
      
      -- Append skipped text if any to previous node
      if best > i then
	 print("APPEND TO " .. code.mode .. " BEFORE " .. newmode)
	 code.text = code.text .. strsub(flow.text, i, best-1)
      end
      
      -- Change to new mode
      luacolor_codeflow_change(flow, newmode)
      
      -- Append this mode text
      if not best or not e then
	 print("HAVE no best or no end",best,e)
	 luacolor_dump_codeflow(flow)
      end

      code.text = code.text .. strsub(flow.text,best,e)
      -- Finally advance text postion
      i = e + 1
   end
   
   --- Going out, needs to close current node, but we need to add
   if i <= l then
      code.text = code.text .. strsub(flow.text,i)
   end
   luacolor_codeflow_change(flow, "")

   if code.text ~= "" then
      print("Text remaining in code text buffer !!");
      luacolor_dump_codeflow(flow)
   end

   --- Mr. Clean ;)
   local r = code.format
   flow.code = nil
   code = nil

   return r
end

lua_color_modes = {
   { -- 1 : startup
      start = '<zml><lua><p>'
	 .. '<macro macro-name="cod" macro-cmd="pre" color="$lua_color_cod">'
	 .. '<macro macro-name="kwd" macro-cmd="pre" color="$lua_color_kwd">'
	 .. '<macro macro-name="rem" macro-cmd="pre" color="$lua_color_rem">'
	 .. '<macro macro-name="str" macro-cmd="pre" color="$lua_color_str">'
	 .. '<macro macro-name="fnm" macro-cmd="pre" color="$lua_color_fnm">'
	 .. '<macro macro-name="fdf" macro-cmd="pre" color="$lua_color_fdf">'
	 .. '<macro macro-name="pct" macro-cmd="pre" color="$lua_color_pct">'
	 .. '<macro macro-name="dgt" macro-cmd="pre" color="$lua_color_dgt">'
	 .. '<font id="1" size="16">'
	 .. '<pre id="1" tabsize="8" tabchar=" "></pre>',
      stop  = "",
   },
   { -- 2 : finish
      start = '<font id="0" size="20" color="text_color"><wrap>',
      stop  = ""
   },
   { -- 3 : code
      flow_ctrl = luacolor_codeflow,
   },
   { -- 4 : comment
      start = "<rem>--",
      stop = "</pre>",
      add = 2
   },
   { -- 5 : [ [
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

--
--- Change lua colorizer machine state.
--- @internal
--
function luacolor_mode(flow, mode)
   --- Did we change mode ?
   if mode ~= flow.mode then
      --- If we have an old mode ?
      if flow.mode then
	 -- There is an old mode !
	 -- Is there a special sub-parser for this type ?
	 if type(lua_color_modes[flow.mode].flow_ctrl) == "function" then
	    -- Yes, there is ! Append the result to formatted output.
	    flow.format = flow.format .. 
	       lua_color_modes[flow.mode].flow_ctrl(flow)
	 else
	    flow.format = flow.format
	       .. lua_color_modes[flow.mode].start
	       .. flow.text
	       .. lua_color_modes[flow.mode].stop
	 end
	 flow.text = ""
      end
      flow.mode_line = flow.line+1
      -- Discard text.
      flow.text = ""
      -- Set new mode
      flow.mode = mode
   end
end

--
--- Convert a lua source file into colored zml.
---
---  @param  fname  Path to lua file
---  @return zml string
---  @retval nil on error
--
function luacolor_file(fname)
   local file = openfile(fname,"rt")
   if not file then return end
   local flow = { format = "", text = "", code = nil, line = 0 }

   local brk
   local line

   -- startup
   luacolor_mode(flow,1)
   luacolor_mode(flow,3)

   -- $$$ Always have a maximum number of line ! 
   local linemax = lua_color_max_line or 5000

   line = read(file)
   while line do
      --- $$$$ mega hack since kos still bugged with printing '%' char !
      --- Strange becoz I have already removed this bug and it come again !!?
--      line = gsub(line,"[%]","%%")

      flow.line = flow.line + 1
      if linemax and flow.line > linemax then
	 printf("[luacolor_file] maximum line number exceeded (%d > %d)",
		flow.line,linemax)
	 return
      end

      line = (lua_color_line_number and
	      format(lua_color_line_number,flow.line) or "") .. line .. "\n"
      local start,len = 1, strlen(line)
      local os = 0

      
      while start <= len do

	 if os >= start then
	    printf("[luacolor_file] : internal error : backward stream %d %d",
		   os, start)
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
	    brk = nil
	 end
      end
      line = read(file)
   end
   -- finish
   luacolor_mode(flow,2)
   luacolor_mode(flow,nil)

   printf("[luacolor_file] [%s] process [%d lines]",fname, flow.line)
   return flow.format
end

return 1
