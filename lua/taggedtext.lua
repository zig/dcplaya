
if not dolib("sprite") then return end

function tt_text_draw(block)
--   print(block.dl, block.x, block.y, block.z, block.color, block.text)
   
   dl_draw_text(block.dl, block.x, block.y, block.z, block.color, block.text)
end

function tt_img_draw(block)
   if block.spr.rotate then
      sprite_draw(block.spr, block.dl, block.x+block.spr.y, block.y+block.spr.x, block.z)
   else
      sprite_draw(block.spr, block.dl, block.x+block.spr.x, block.y+block.spr.y, block.z)
   end
end

function tt_img_cmd(mode, param)
   local src = param.src
   local name = param.name or src
   local spr = sprite_get(name)
   if not spr then
      if not src then
	 return
      end

      --local tex = tex_new()
      --print("'"..src.."'")
      local tex = tex_get(home.."lua/rsc/icons/"..src) or tex_new(home.."lua/rsc/icons/"..src)
      print(tex)
      if tex then
	 local info = tex_info(tex)
	 local w, h = info.w, info.h
	 w = param.w or w
	 h = param.h or h
	 rotate = param.rotate
	 spr = sprite(name, 0, 0, w, h, 0, 0, 1, 1, tex, rotate)
      end
   end

   if spr then
      local block = {
	 type = "img",
	 spr = spr,
	 w = spr.w,
	 h = spr.h,
	 dl = mode.dl,
	 z = mode.z,
	 draw = tt_img_draw
      }
      return block
   end
end


-- UGLY !!!
function tt_tocolor(s)
   if strsub(s, 1, 1) == "#" then
      local v = 0
      local i
      local l = strlen(s)
      local n = 0
      local j = 2
      local col = { 1, 1, 1, 1 }
      
      for i=2, l, 1 do
	 local c
	 c = strbyte(s, i)
	 if c >= 97 then
	    c = c - 97 + 10
	 elseif c >= 65 then
	    c = c - 65 + 10
	 else
	    c = c - 48
	 end
	 v = v * 16 + c

	 n = n+1
	 if n == 2 then
	    col[j] = v/255
	    v = 0
	    j = j+1
	    n = 0
	    if j == 5 then
	       j=1
	    end
	 end
      end

      return col
   else
      local a = tonumber(s) / 255
      return { 1, a, a, a }
   end
end


function tt_font_cmd(mode, param)

   mode.font_h = tonumber(param.size) or mode.font_h
   if param.color then
--      print("color", param.color)
      mode.color = tt_tocolor(param.color)
--      print(mode.color[1], mode.color[2], mode.color[3], mode.color[4])
   end
   mode.font_id = param.id or mode.font_id
--   print(param.size)

   local block = {
      type = "font_style",
      dl = mode.dl,
      id = mode.font_id,
      h = mode.font_h,
      w = 0,
      h = 0,
      draw = function(block)
--		print(block.dl, block.id, block.h)
		dl_text_prop(block.dl, block.id, block.h)
	     end
   }

   return block
end

-- all commands
tt_commands = {
   img = tt_img_cmd,
   br = function(mode)
	   tt_endline(mode)
	end,
   left = function(mode)
	     mode.align_line_h = tt_align_line_left
	  end,
   center = function(mode)
	       mode.align_line_h = tt_align_line_center
	    end,
   right = function(mode)
	      mode.align_line_h = tt_align_line_right
	   end,
   font = tt_font_cmd
}




function tt_getword(text, e1)
   if not e1 then
      return
   end

   local word

   e1 = strfind(text, "[^%s]", e1)
   
   if e1 then
      local e2 = strfind(text, "%s", e1)
      if e2 then
	 word = strsub(text, e1-1, e2-1)
      else
	 word = strsub(text, e1, -1)
      end
      e1 = e2
   end

   return word, e1
end

function tt_getparam(text, e1)
   if not e1 then
      return
   end

   local word, value

   e1 = strfind(text, "[^%s]", e1)
   
   if e1 then
      local _, e2 = strfind(text, "[^=%s]+", e1)
      if e2 then
	 word = strsub(text, e1, e2)
	 e1, e2 = strfind(text, '%b""', e2+1)
	 if e2 then
	    value = strsub(text, e1+1, e2-1)
	    e2 = e2+1
	 end
      else
	 word = strsub(text, e1, -1)
      end
      e1 = e2
   end

   return word, value, e1
end


function tt_copymode(mode)
   local copy, i, v
   for i, v in mode do
      copy[i] = v
   end
   return copy
end

function tt_align_line_down(mode, h)
   local line = mode.curline
   local i
   for i=1, line.n, 1 do
      line[i].y = line[i].y - line[i].h + h
   end
end

function tt_align_line_left(mode, h)
   local line = mode.curline
   local i
   local x = mode.box[1]
   for i=1, line.n, 1 do
      line[i].x = line[i].x + x
   end
end

function tt_align_line_right(mode, h)
   local line = mode.curline
   local i
   local x = mode.box[3] - mode.w
   for i=1, line.n, 1 do
      line[i].x = line[i].x + x
   end
end

function tt_align_line_center(mode, h)
   local line = mode.curline
   local i
   local x = (mode.box[1] + mode.box[3] - mode.w)/2
   for i=1, line.n, 1 do
      line[i].x = line[i].x + x
   end
end

function tt_endline(mode)
   local h = 0
   local line = mode.curline
   local i

   for i=1, line.n, 1 do
      h = max(h, line[i].h)
   end

   mode:align_line_h(h)
   mode:align_line_v(h)

   tinsert(mode.lines, mode.curline)
   mode.curline = { n = 0 }
   mode.h = mode.h + h
   mode.w = 0
end

function tt_insert_block(mode, block)
   local w = mode.w

   if w + block.w > mode.bw then
      tt_endline(mode)
      w = 0
   end

   block.x = w
   block.y = mode.h + mode.box[2]
   tinsert(mode.curline, block)
   mode.w = w + block.w
end


-- perform justification on a tagged text
function tt_build(text, mode)
   if not mode then
      mode = { }
   end
   mode.dl = mode.dl or dl_new_list(1024, 1)
   mode.box = mode.box or { 0, 0, 640, 480 }
   mode.outbox = mode.outbox or { 0, 0, 640, 480 }
   mode.color = mode.color or { 1, 1, 1, 1 }
   mode.z = mode.z or 0
   mode.font_id = mode.font_id or 0
   mode.font_h = mode.font_h or 16
   mode.font_aspect = mode.font_aspect or 1
   mode.curline = { n = 0 }
   mode.lines = { n = 0 }
   mode.bw = mode.box[3] - mode.box[1] -- box width
   mode.w = mode.w or 0
   mode.h = mode.h or 0
   mode.align_line_v = mode.align_line or tt_align_line_down
   mode.align_line_h = mode.align_line or tt_align_line_left

   local blocks = { n = 0 }
   
   local start = 1
   local len = strlen(text)
   
   while start <= len do
      local e1, e2 = strfind(text, "[%s%p]*%w+[%s%p]+", start)
      if not e2 then
	 e2 = len
      end

--      if e1 then
--	 print("sub", strsub(text, start, e2))
--      end

      local e3, e4
      if start <= e2 then
	 e3, e4 = strfind(text, "<", start, e2)
	 if e3 then 
	    e3, e4 = strfind(text, "%b<>", start)
	    if e3 then
	       e2 = e3-1
	    end
	 end
      end

      if start <= e2 then
	 local block = {
	    type = "text",
	    text = strsub(text, start, e2),
	    draw = tt_text_draw,
	    dl = mode.dl,
	    font_id = mode.font_id,
	    font_h = mode.font_h,
	    font_aspect = mode.font_aspect,
	    z = mode.z,
	    color = mode.color,
	    
	 }
--	 print("text", block.text)
	 block.w, block.h = dl_measure_text(block.dl, block.text, block.font_id, block.font_h, block.font_aspect)

	 tt_insert_block(mode, block)

	 --print(block.text, block.w, block.h)
      end
     
      if e3 then
	 local tag = strsub(text, e3+1, e4-1)
	 --print("tag", tag)
	 e2 = e4

	 local i = 1
	 local command
	 command, i = tt_getword(tag, i)
	 if command then
	    local param = { }
	    local key, value
--	    print("command", "'"..command.."'")
	    key, value, i = tt_getparam(tag, i)
	    while key do
--	       if value then
--		  print("'"..key.."'", "'"..value.."'")
--	       end
	       param[key] = value
	       key, value, i = tt_getparam(tag, i)
	    end

	    local f = tt_commands[command]

	    if f then
	       block = f(mode, param)
	       if block then
		  tt_insert_block(mode, block)
	       end
	    end
	 end
      end

      start = e2 + 1
   end

   if mode.curline.n > 0 then
      tt_endline(mode)
   end

   return mode
end

function tt_draw(tt)
   local i, j
   for i=1, tt.lines.n, 1 do
      local line = tt.lines[i]
      for j=1, line.n, 1 do
	 line[j]:draw()
      end
   end
end

tt = tt_build(
	      [[
Hello <img name="vmu"> World ! <br>
<center> <font size="24" color="#00ffff"> Centered !! <br>
<right> <font size="48" color="#ffff00"> <img name="dcplaya" src="dcplaya.tga"> TITI <img name="colorpicker" src="colorpicker.tga"> TOTO
]], 
	      { 
		 box = { 100, 100, 600, 200 },
		 z = 200
	      })

tt_draw(tt)

--print(tt.w, tt.h)

