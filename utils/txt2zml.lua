--- @author  benjamin gerard
--- @date    2003/03/31
--- @brief   convert a lynks generated text file to zml.
---
--- $Id: txt2zml.lua,v 1.2 2003-04-01 13:18:56 ben Exp $
--

--- Mesure a block of line.
function txt2zml_measure_block(block)
   local n,i = getn(block)
   local xm,xM
   for i=1, n do
      local x1,x2,l
      l = strlen(block[i].line)
      x1,x2 = strfind(block[i].line, "^%s+")
      x2 = x2 or 0
      xm = xm and min(xm,x2) or x2
      xM = xM and max(xM,l) or l
      block[i].x = x2
      block[i].w = l
--      print(format("%02d[%s]",block[i].w,block[i].line))
   end
   block.x = xm
   block.w = xM
end

--- Mesure all blocks.
function txt2zml_measure_blocks(blocks)
   local n,i = getn(blocks)

   for i=1, n do
      local block = blocks[i]
      txt2zml_measure_block(block)
      blocks.x = (blocks.x and  min(block.x, blocks.x)) or block.x
      blocks.w = (blocks.w and  max(block.w, blocks.w)) or block.w
   end
end

--- Guess text width.
function txt2zml_guess_width(blocks)
   local n,i = getn(blocks)
   if not blocks.w then
      txt2zml_measure_blocks(blocks)
   end
   for i=1, n do
      local block = blocks[i]
      local m,j = getn(block)
      for j=1, m do
	 local guess
	 
	 if i == 1 then
	    -- $$$ Special case, first block should be a centrered title,
	    -- we use it to make the measure.
	    guess = block[j].w + block[j].x
	 end

	 if strfind(block[j].line,"^%s+%p%p%p%p+$") then
	    -- I don't understand why searching '-' does not work here,
	    -- since it works when I used this regexpr directly into lua
	    -- interpretor
	    guess = guess or block[j].w + block[j].x
	    block.guessed_w = block.guessed_w
	       and max(block.guessed_w,guess) or guess
 	    tremove(block,j)
	    break
	 else
	    guess = guess or block[j].w
	 end
	 block.guessed_w = block.guessed_w
	    and max(block.guessed_w,guess) or guess
      end
      blocks.guessed_w = blocks.guessed_w and block.guessed_w
	 and max(blocks.guessed_w,block.guessed_w) or block.guessed_w
   end
end

--- Build refrence table, and removes reference from bottom of file.
--
function txt2zml_make_references(blocks)
   local references = {}
   local n,i,ref = getn(blocks)
   for i=1, n do
      local block = blocks[i]
      if getn(block) == 1 and block[1].line == "References" then
	 ref = i
	 break
      end
   end
   if not ref then return end
   for i=ref+1, n do
      local block = blocks[i]
      local m,j = getn(block)
      for j=1,m do
	 local a,b,refnum,reffile =
	    strfind(block[j].line,"^[%s]*(%d+)%.[%s]+([%w%p]+)$")
	 if a then
	    reffile = gsub(reffile,".*/","")
	    reffile = gsub(reffile,"%.html(#*[%w%p]*)$",".zml%1")
	    tinsert(references, tonumber(refnum), reffile)
	 end
      end
   end

   while getn(blocks) >= ref do
      tremove(blocks)
   end
   return references
end

--- Transform tab into space and trim trailing soaces.
--
function txt2zml_tab_to_space(line, tabstop)
   local tab = strchar(9)
   local a,b = strfind(line,tab,1)
   tabstop = tabstop or 8


   if a then
      -- Found a tab, start conversion
      local pos,s,l = 0,1,strlen(line)
      local line2 = ""
      while a do
	 line2 = line2 .. strsub(line,s,a-1)
	 pos = pos + (a - s)
	 local stop = pos + tabstop - floor(mod(pos,tabstop))
	 stop = stop - pos
	 line2 = line2 .. strrep(' ',stop)
	 pos = pos + stop
	 s = a + 1
	 a,b = strfind(line,tab,s)
      end
      -- No more tab, finish line
      if s <= len then
	 line2 = line2 .. strsub(line,s)
      end
      line = line2
   end
   return gsub(line,"%s+$","")
end

--- Create blocks from lines buffer.
--
function txt2zml_create_blocks(lines)
   local blocks = {}
   local i,n = 1, getn(lines)
   while i <= n do
      -- Skip blank lines
      while i <= n and lines[i] == "" do
	 i = i + 1
      end
      if i > n then break end

      -- Start a new block
      local block = {}
      while i <= n and lines[i] ~= "" do
	 tinsert(block, { line=lines[i] } )
-- 	 print(getn(blocks),getn(block),lines[i])
	 i = i + 1;
      end
      tinsert(blocks,block)
   end
   return blocks
end

function txt2zml_guess_alignment(block, width)
   local n,i = getn(block)

   for i=1, n do
      local line = block[i]
      line.hint_center = line.x > 0 and abs(line.w+line.x-width) < 2
      block.hint_center = (block.hint_center or i == 1) and line.hint_center
   end
   block.alignment = (block.hint_center and n > 1) and "center" or "left"
end

function txt2zml_guess_type(block)
   local n,i = getn(block)
   if n == 1 and block.w < 60 then
      if block.x == 0 then
	 block.type = "chapter"
      elseif block.x == 3 then
	 block.type = "section"
      end
   end
   txt2zml_guess_function(block)
   block.type = block.type or "body"
end

-- guess for complete or not function line
function txt2zml_guess_function_line(line)
   local expr1 = "^%s*[%w_]*[%s]*[%w_]+[%s]+%([%s%w_,)]*$"
   local expr2 = "^%s*[%w_]*[%s]*[%w_]+[%s]+%([%s%w_,]*%)$"

   -- Speed up !
   if not strfind(line,"(",1,1) then
      return
   end

   if strfind(line,expr2) then
      return 1
   end
   if strfind(line,expr1) then
      return 2
   end
end

function txt2zml_guess_function(block)
   local n = getn(block)
   if n == 0 then
      return
   end

   if n == 1 then
      if (txt2zml_guess_function_line(block[1].line) or 0) == 1 then
--	 print(format("FUNC1:[%s]",block[1].line))
	 block.type = "function"
      end
      return
   end

   if (txt2zml_guess_function_line(block[1].line) or 0) ~= 2 then
      return
   end

   if not strfind(block[n].line,"%)$") then
      return
   end
   local i
   for i=2,n-1 do
      if not strfind(block[i].line,"^%s*[%w_]*%s*[%w_]+%s*,*$") then
	 return
      end
   end

   local line = block[1].line
   while getn(block) > 1 do
      block[1].line = block[1].line .. gsub(block[2].line,"%s+"," ")
      tremove(block,2)
   end
   txt2zml_measure_block(block)
   block.type = "function"
--   print(format("FUNC2:]%d][%s]",getn(block),block[1].line))
end


function txt2zml_group_list(block, i, x, n)
   local li_expr = "^%s+[*+o#@-][%s]+"
   local ol_expr = "^%s+%d+%.[%s]+"
   n = n or 0

   while i <= getn(block) and block[i].x >= x do
      if strfind(block[i].line, ol_expr) then
	 if block[i].x > x then
	    i = txt2zml_group_list(block, i, block[i].x, 0)
	 else
	    n = n + 1
	    block[i].line = gsub(block[i].line,"^(%s*)%d+%.","%1#"..n)
	    block[i].guessed_list = x
	    block[i].numbered_list = n
	    i = i + 1
	 end
      elseif strfind(block[i].line, li_expr) then
	 n = 0
	 if block[i].x > x then
	    i = txt2zml_group_list(block, i, block[i].x)
	 else
	    block[i].line = gsub(block[i].line,"^(%s*).","%1~")
	    block[i].guessed_list = x
	    i = i + 1
	 end
      else
	 block[i-1].line = block[i-1].line .. ' '
	    .. gsub(block[i].line,"^%s*","")
	 tremove(block,i)
      end
   end	 
   return i
end

function txt2zml_guest_list(block)
   local li_expr = "^%s+[%*+o#@-][%s]+"
   local ol_expr = "^%s+%d+%.[%s]+"
   local i = 1
   while i <= getn(block) do
      if strfind(block[i].line, li_expr)
	 or strfind(block[i].line, ol_expr) then
	 i = txt2zml_group_list(block, i, block[i].x)
      else
	 i = i + 1
      end
   end
end

function txt2zml_format_line(line,trim,br)
   if trim then
      line = gsub(line,"^%s*","")
   end
   -- remove reference
   line = gsub(line,"%[%d+%](%w)","%1")

   br = br and "<br>" or ""
   print(line .. br)
end

function txt2zml_format_li(line)
   line = gsub(line,"^%s+~%s+","")
   txt2zml_format_line('<li>'..line..'</li>',nil,nil)
end

function txt2zml_format_ol(line)
   line = gsub(line,"^%s+","")
   txt2zml_format_line('<ol>'..line..'</ol>',nil,nil)
end


function txt2zml_format_title(block)
   local m,j = getn(block)
   print('<title><title-color><center><a name="Title">')
   for j=1,m do
      txt2zml_format_line(block[j].line,1, j ~= m)
   end
   print("</a></title>")
end

function txt2zml_format_chapter(block)
   local m,j = getn(block)
   print('<p hspace="0"><chapter><chapter-color><left>')
   print('<a name="'..block[1].line..'">')
   for j=1,m do
      txt2zml_format_line(block[j].line,1,nil)
   end
   print("</a></chapter>")
end

function txt2zml_format_section(block)
   local m,j = getn(block)
   print('<p hspace="16"><section><section-color><left>')
   for j=1,m do
      txt2zml_format_line(block[j].line,1,nil)
   end
   print("</section>")
end

function txt2zml_format_function(block)
   local m,j = getn(block)
   print('<p hspace="16"><function><function-color><left>')
   for j=1,m do
      txt2zml_format_line(block[j].line,1,nil)
   end
   print("</function>")
end


function txt2zml_format_subsection(line)
   print('<p hspace="24"><ssection><ssection-color><left>')
   txt2zml_format_line(line,1,nil)
   print("</ssection>")
end

function txt2zml_format_footer(block)
   local m,j = getn(block)
   line = ""
   for j=1,m do
      line = line .. gsub(block[j].line,"%s+"," ")
   end
   if line ~= "" then
      print('<p hspace="0"><footer><footer-color><center><a name="Footer">')
      txt2zml_format_line(line,1,nil)
      print("</a></footer>")
   end
end

function txt2zml_groups(block)
   local m,j = getn(block),1
   local groups = {}

   local b = j
   while j <= m do
      local x = block[j].x
      if block[j].guessed_list then
	 while b <= m and block[b].guessed_list do
	    b = b + 1
	 end
      else
	 while b <= m and block[b].x == x and not block[b].guessed_list do
	    b = b + 1
	 end
      end
      tinsert(groups, j)
      j = b
   end
   tinsert(groups, m+1)
   return groups
end

function txt2zml_format_body(block)
   local groups = txt2zml_groups(block)
   local inbody = nil
   local body_start = '<p hspace="%d"><body><body-color><'
      ..block.alignment..'>'
   local li_start = '<p hspace="%d"><body><body-color><left>'

   local n,g = getn(groups)-1 
   for g=1, n do
      local a,b = groups[g], groups[g+1]

      if b-a == 1
	 and block[a].x == block.x
	 and block[a].w < 60
	 and not block[a].guessed_list
	 and strfind(block[a].line,":$")
      then
	 txt2zml_format_subsection(block[a].line)
	 inbody = nil
      else
	 if block[a].guessed_list then
	    local f = block[a].numbered_list and txt2zml_format_ol
	       or txt2zml_format_li
	    local prevx = -1000
	    while a < b do
	       print(format(li_start,32+block[a].guessed_list*4))
	       f(block[a].line)
	       a = a + 1
	    end
	    inbody = nil
	 else
	    if not inbody then 
	       print(format(body_start,32))
	       inbody = 1
	    end
	    while a < b do
	       txt2zml_format_line(block[a].line,1, a+1 ~= b)
	       a = a + 1
	    end
	 end
      end
   end
   print('</body>')
end

txt2zml_format_type = {
   title = txt2zml_format_title,
   chapter = txt2zml_format_chapter,
   section = txt2zml_format_section,
   body = txt2zml_format_body,
   ["function"] = txt2zml_format_function,
   footer = txt2zml_format_footer,
}

function txt2zml_format(blocks)
   txt2zml_guess_width(blocks)
   local prevtype = ""
   local n,i = getn(blocks)
   local inchapter

   for i=1, n do
      local block = blocks[i]
      if i == 1 then
	 block.type = "title"
      elseif i == n then
	 block.type = "footer"
      else
	 txt2zml_guess_type(block)
	 if block.type == "chapter" then
	    inchapter = 1
	 elseif block.type == "section" then
	    -- $$$ hack : can't have a section in a function subsection
	    -- $$$ hack : can't have 2 following sections
	    if not inchapter or prevtype == "section" then
	       block.type = "body"
	    end

	 elseif block.type == "function" then
	    inchapter = nil
	 end
      end
      prevtype = block.type

      txt2zml_guess_alignment(block, blocks.guessed_w)
      txt2zml_guest_list(block)
      if txt2zml_format_type[block.type] then
	 txt2zml_format_type[block.type](block)
	 print()
      end
   end
end

function txt2zml(fname, tabstop)
   -- Check parameters
   if type(fname) ~= "string" then
      print("[txt2zml] : bad argument")
      return
   end
   tabstop = type(tabstop) == "number" and tabstop or 8

   -- Read the file
   local file = openfile(fname, "rt")
   if not file then
      print ("[txt2zml] : can't open [" .. fname .. "]")
      return
   end

   local lines,line = {}, read(file)
   while line do
      line = txt2zml_tab_to_space(line, tabstop)
      tinsert(lines, line)
      line = read(file)
   end
   line = nil
   closefile(file)
   -- Create Blocks
   blocks = txt2zml_create_blocks(lines)
   txt2zml_make_references(blocks)
   -- $$$ hack : remove first block (table of conntent)
   tremove(blocks,1)
   lines = nil
   txt2zml_measure_blocks(blocks)
   txt2zml_format(blocks)
end


-- txt2zml("/home/ben/dcplaya/doc/doc/text/modules.txt")
if type(arg) ~= "table" then
--   arg = { "/home/ben/dcplaya/doc/doc/text/group__dcplaya__display__list.txt" }
   arg = { "/home/ben/dcplaya/doc/doc/text4zml/group__dcplaya__lua__textlist.txt" }
end

txt2zml(arg[1])
arg=nil
