--- @ingroup dcplaya_lua_colorize
--- @file    lua_colorize.lua
--- @author  benjamin gerard
--- @date    2003/03/20
--- @brief   LUA source colorizer.
---
--- $Id: lua_colorize.lua,v 1.7 2003-03-27 05:48:05 ben Exp $
--

--- @defgroup  dcplaya_lua_colorize  LUA source colorizer
--- @ingroup   dcplaya_lua_gui
--- @brief     creates preformatted colorized zml for lua source viewer
---
---  This is a lua syntax high-lighter. Because of a lot of memory problems
---  caused by string concatenation and garbage collecting, these function
---  use a trick to allocate a big string. It is a special print() function
---  which override the standard print. This function is call by a special
---  dostring() called dostring_print(). Print function executed via the
---  dostring_print() will be concatenate in a buffer. Supplemental parameters
---  allow to control the initial size of the final string and the reallocation
---  policy. In an other hand, the syntax highlight have different level of
---  details. For more information have a look to the lua_color_frag_size
---  documentation.
---
--- @author  benjamin gerard
--- @{

--- LUA keywords.
--: boolean lua_color_key_words[keyword];
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

--- Fragment size in bytes for colorization auto-leveling. In order
--- to save memory, the syntax highlight of lua source have used
--- different level of details with a maximum of features for files which
--- size is less than the lua_color_frag_size value. Each level step is a
--- block twice larger, which means that the file less than
--- 2 times lua_color_frag_size have a medium level syntax highlight,
--- 4 times have a minimum syntax highlight, 8 times are just formatted and
--- largest files are discarded. The default value is 16Kb so you may see
--- all decent lua source file with a nice render and the maximum limit is
--- 128Kb which is more than usual size for source code !
--: number lua_color_frag_size;
lua_color_frag_size = 16 * 1024

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
lua_color_line_number = nil -- "%03d " -- deactivated to save little memory 

--- Maximum number of line.
--: number lua_color_max_line;
lua_color_max_line = nil --500

function luacolor_codeflow_change(flow, newmode)
   local code = flow.code

   if code.mode ~= newmode then
      if code.text ~= "" then
	 local isfdf = code.mode == "<d>"
	 if  isfdf then
	    print(format("<a name=%q>",code.text))
	 end
	 print(code.mode,code.text,"</pre>")
	 if isfdf then
	    print("</a>")
	 end
	 code.fctdef = (newmode == "<k>" and code.kwd_fct) or
	    (code.fctdef and code.mode == "<k>" and newmode=="<c>")
	 code.kwd_fct = nil
	 code.text = ""
      end
      code.mode = newmode
   end
end

function luacolor_dump_codeflow(flow)
end

function luacolor_codeflow_simple (flow)
   local i,l = 1,strlen(flow.text)
   if l < i then
      -- Empty code node discarded
      return ""
   end

   local func -- previous word was "function"
   local code -- code tag has been emit
   while i <= l do
      local a,b,w
      -- Find word start
      a,b = strfind(flow.text, "[%w_]", i)
      if not a then
	 -- No more, finish
	 if not code then print ("<c>") end
	 print(strsub(flow.text,i),"</pre>")
	 code = nil
	 break
      end
      -- Flush char before word
      if (a > i) then
	 if not code then print ("<c>") code = 1 end
	 print(strsub(flow.text,i,a-1))
	 i = a
      end

      a,b,w = strfind(flow.text, "([%w_]+)",a)
      -- Can't be nil, or I missed something !
      if func then
	 -- Had a function before, this could be a function def, just check
	 -- for an open parenthesis
	 if strfind(flow.text, "[:space:]*[(]", b+1) then
	    -- really have a function name here
	    if code then print("</pre>") code=nil end
	    print(format('<a name=%q>',w),
		  "<d>", w, "</pre></a>")
	    w = nil -- discard word
	 end
	 func = nil -- discard function
      elseif lua_color_key_words[w] then
	 -- Hi-light keywords
	 if code then print("</pre>") code=nil end
	 print("<k>",w,"</pre>")
	 -- notify function for next word found, even if it may not be a
	 -- function
	 func = w == "function"
	 w = nil -- discard word
      end

      -- word not discarded, print it now
      if w then
	 if not code then print("<c>") code = 1 end
	 print(w)
      end
      i = b+1
   end

   -- If code tag has been emit, close it here
   if code then print ("</pre>") end
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
   code.mode = "<c>" -- Current mode (opening tag)
--    code.format = ""    -- Current formatted code output
   code.text = ""      -- Current mode text

   while i <= l do
      local a,b,c,d,e,w

      local newmode,best
      -- Find word/number starting position (and get char)
      a,b,c = strfind(flow.text, "([%w_])",i)

      -- Find punctuation string [consider space has punctuation here]
      d,e,w = strfind(flow.text, "([%p%s]+)", i)

      if d then
	 -- Get punctuation as default best.
	 best = d
	 -- e is the end, no nee to set it
	 newmode = "<t>"

	 -- Check for function call here if we had an opening parenthesis.
	 if code.mode == "<c>" and strfind(w,"[%s]*[(]") then
	    code.mode = code.fctdef and "<d>" or "<f>"
	 end
      end

      if a then
	 -- have some words too !
	 if not best or a < best then
	    -- Word was found before punctuation, it is our winner :P
	    best = a
	    -- Now we should determine what kind of word it is.

	    if strfind(c,"%d") then
	       -- It is a number 
	       newmode = "<n>"
	       a,e = strfind(flow.text, "%d+[.]?%d*[Ee]?%-?%d*", a)
	    else
	       -- It is a word, get it and trailing space too !
	       a,e,w,d = strfind(flow.text, "([%w_]*)(%s*)", a)

	       if lua_color_key_words[w] then
		  newmode = "<k>"
		  code.kwd_fct = w == "function"
	       else
		  newmode = "<c>"
		  code.kwd_fct = code.kwd_fct and code.kwd_fct + 1
	       end

	    end
	 end
      end


      -- Well here we should have a new mode, indeed it is possible that
      -- we have none. This could happen if we make an error in the regexpr
      -- or more likely on unexpected characters. In that case we will
      -- simply advance in the current mode if there is none, we crates
      -- a default <c> node.
      if not newmode then
	 break
      end
      
      -- Append skipped text if any to previous node
      if best > i then
-- 	 print("APPEND TO " .. code.mode .. " BEFORE " .. newmode)
	 code.text = code.text .. strsub(flow.text, i, best-1)
      end
      
      -- Change to new mode
      luacolor_codeflow_change(flow, newmode)
      
      -- Append this mode text
      if not best or not e then
-- 	 print("HAVE no best or no end",best,e)
-- 	 luacolor_dump_codeflow(flow)
      end

      code.text = code.text .. strsub(flow.text,best,e)
      -- Finally advance text position
      i = e + 1
   end
   
   --- Going out, needs to close current node, but we need to add
   if i <= l then
      code.text = code.text .. strsub(flow.text,i)
   end
   luacolor_codeflow_change(flow, "")

   if code.text ~= "" then
--       print("Text remaining in code text buffer !!");
--       luacolor_dump_codeflow(flow)
   end

   --- Mr. Clean ;)
--   local r = code.format
   flow.code = nil
   code = nil

--   return r
end

lua_color_modes = {
   { -- 1 : startup
      start = '<zml><lua><p>\n'
	 .. '<macro macro-name="c" macro-cmd="pre" color="$lua_color_cod">\n'
	 .. '<macro macro-name="k" macro-cmd="pre" color="$lua_color_kwd">\n'
	 .. '<macro macro-name="r" macro-cmd="pre" color="$lua_color_rem">\n'
	 .. '<macro macro-name="s" macro-cmd="pre" color="$lua_color_str">\n'
	 .. '<macro macro-name="f" macro-cmd="pre" color="$lua_color_fnm">\n'
	 .. '<macro macro-name="d" macro-cmd="pre" color="$lua_color_fdf">\n'
	 .. '<macro macro-name="t" macro-cmd="pre" color="$lua_color_pct">\n'
	 .. '<macro macro-name="n" macro-cmd="pre" color="$lua_color_dgt">\n'
	 .. '<font id="1" size="16">'
	 .. '<pre id="1" tabsize="8" tabchar=" ">\n',
      stop  = "</pre>",
   },
   { -- 2 : finish
      start = '\n<font id="0" size="20" color="text_color"><wrap>\n',
      stop  = ""
   },
   { -- 3 : code
      flow_ctrl = luacolor_codeflow,
   },
   { -- 4 : comment
      start = "<r>--",
      stop = "</pre>",
      add = 2
   },
   { -- 5 : [ [
      start = "<s>[[",
      stop = "]]</pre>",
      add = 2
   },
   { -- 6 : ""
      start = '<s>"',
      stop = '"</pre>',
      add = 1
   },
   { -- 7 : ''
      start = "<s>'",
      stop = "'</pre>",
      add = 1
   },
}

--
--- Change lua colorizer machine state.
--- @internal
--
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
-- 	    flow.format = flow.format .. 
-- 	       lua_color_modes[flow.mode].flow_ctrl(flow)
	    lua_color_modes[flow.mode].flow_ctrl(flow)
	 else
-- 	    flow.format = flow.format
-- 	       .. lua_color_modes[flow.mode].start
-- 	       .. flow.text
-- 	       .. lua_color_modes[flow.mode].stop
	    print(lua_color_modes[flow.mode].start,
		  flow.text,
		  lua_color_modes[flow.mode].stop)
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
function luacolor_file_wrap(fname)
   local file = openfile(fname,"rt")
   if not file then return end
   local flow = { text = "", code = nil, line = 0 }

   local brk
   local line

   -- startup
   luacolor_mode(flow,1)
   luacolor_mode(flow,3)

   -- $$$ Always have a maximum number of line ! 
   local linemax = lua_color_max_line or 5000

   line = read(file)
   while line do
      flow.line = flow.line + 1
      if linemax then
	 if flow.line == linemax+1 then
	    line = format ("-- Truncated file : too many line (%d > %d)",
			   flow.line,linemax)
	 elseif flow.line > linemax then
	    break;
	 end
      end

      -- Make garbage collector from time to time
      if mod(flow.line,100) == 0 then 
	 collectgarbage()
      end

      line = (lua_color_line_number and
	      format(lua_color_line_number,flow.line) or "") .. line .. "\n"
      local start,len = 1, strlen(line)
      local os = 0
      
      while start <= len do

	 if os >= start then
-- 	    printf("[luacolor_file] : internal error : bckwrd stream %d %d\n",
-- 		   os, start)
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
	    while bs and bs > 1 do
	       -- Quote break found but it can be escaped, take care of that
	       -- here.
	       local ss = bs - 1 -- Position of the preceding char
	       local slashed     -- non slashed at start
	       while ss > 0 and strsub(line,ss,ss) == "\\" do
		  slashed = not slashed
		  ss = ss - 1
	       end
	       if slashed then
		  -- Finally it was slashed, try to find another break.
		  bs,be = strfind(line,brk,bs+1)
	       else
		  -- Find a break, do a break.
		  break
	       end
	    end

	    -- After while, bs,be must match the ending quote if any.
	    if not bs then
	       -- No ending, just append the line to current mode buffer.
	       flow.text = flow.text .. strsub(line, start)
	       break
	    end
	    -- Quote finish here : back to code after flushing string end.
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
   collectgarbage()
end

function luacolor_file(fname)
   if type(dostring_print) ~= "function" then return end
   if not test("-f",fname) then return end
   local file = openfile(fname,"rt")
   if not file then return end
   local len = seek(file,"end")
   closefile(file)
   if not len then return end
   printf("File size = %d",len)

   -- $$$ ben : Because of memory problems, lua color capabilities depends on 
   -- file size. Here is my little kitchen :P
   if len < 2*lua_color_frag_size then
      -- < 16Kb use full colorization
      -- < 32Kb use medium colorization (still has keywords, function def)
      local m = 2
      lua_color_modes[3].start = nil
      lua_color_modes[3].stop = nil
      lua_color_modes[3].flow_ctrl = luacolor_codeflow_simple
      if len < lua_color_frag_size then
	 m = 4
	 lua_color_modes[3].flow_ctrl = luacolor_codeflow
      end
      return dostring_print(format("luacolor_file_wrap(%q)\n",fname),
			    -1.25, 2048 + (len+16) * m)
   elseif len < 4*lua_color_frag_size then
      -- < 64Kb Use first level colorization (code, string and comment)
      lua_color_modes[3].flow_ctrl = nil
      lua_color_modes[3].start = "<c>"
      lua_color_modes[3].stop = "</pre>"
      return dostring_print(format("luacolor_file_wrap(%q)\n",fname),
			    -1.25, 2048 + (len+16) * 4)
   elseif len < 8*lua_color_frag_size then
      -- < 128Kb is our maximum allowed, but who makes 128kb of source code
      -- in a single file ??
      -- Here, we just set the preformat to suit a tab-size of 8
      file = openfile(fname,"rt")
      local s =
	 '<zml><font id="1" size="16"><pre id="1" tabsize="8" tabchar=" ">\n'
	 .. (read(file,"*a") or "")
	 .. ('</pre>' .. lua_color_modes[2].start)
      closefile(file)
      return s
   end
end

return 1
