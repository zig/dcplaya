
if not dolib("sprite") then return end

function tt_text_draw(block)
--   print(block.dl, block.x, block.y, block.z, block.color, block.text)
   
   dl_draw_text(block.dl, block.x, block.y, block.z, block.color, block.text)
end

function tt_img_draw(block)
   local lx, ly = block.spr.x, block.spr.y
   local sx, sy = block.sx, block.sy
   if sx then
      lx = lx * sx
      ly = ly * sy
   end
   if block.spr.rotate then
      sprite_draw(block.spr, block.dl, block.x+ly, block.y+lx, block.z, sx, sy)
   else
      sprite_draw(block.spr, block.dl, block.x+lx, block.y+ly, block.z, sx, sy)
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
	 rotate = param.rotate
	 spr = sprite(name, 0, 0, w, h, 0, 0, 1, 1, tex, rotate)
      end
   end

   if spr then
      local sx, sy
      sx = param.sx or param.scale
      sy = param.sy or sx
      local block = {
	 type = "img",
	 spr = spr,
	 w = spr.w,
	 h = spr.h,
	 sx = sx,
	 sy = sy,
	 dl = mode.dl,
	 z = mode.z,
	 draw = tt_img_draw
      }

      if sx then
	 block.w = block.w * sx
	 block.h = block.h * sy
      end
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
   up = function(mode)
	     mode.align_v = tt_align_up
	  end,
   vcenter = function(mode)
	       mode.align_v = tt_align_vcenter
	    end,
   down = function(mode)
	      mode.align_v = tt_align_down
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
   copy = { }
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

function tt_align_line_vcenter(mode, h)
   local line = mode.curline
   local i
   for i=1, line.n, 1 do
      line[i].y = line[i].y + ( h - line[i].h ) / 2
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

function tt_align_up(mode)
   local j
   local lines = mode.lines

   local y = mode.box[2]

   for j=1, lines.n, 1 do

      local line = lines[j]
      local i
      for i=1, line.n, 1 do
	 line[i].y = line[i].y + y
      end

   end
end

function tt_align_vcenter(mode)
   local j
   local lines = mode.lines

   local y = (mode.box[2] + mode.box[4] - mode.h)/2

   for j=1, lines.n, 1 do

      local line = lines[j]
      local i
      for i=1, line.n, 1 do
	 line[i].y = line[i].y + y
      end

   end
end

function tt_align_down(mode)
   local j
   local lines = mode.lines

   local y = mode.box[4] - mode.h

   for j=1, lines.n, 1 do

      local line = lines[j]
      local i
      for i=1, line.n, 1 do
	 line[i].y = line[i].y + y
      end

   end
end


function tt_endline(mode)
   local h = 0
   local line = mode.curline
   local i

   for i=1, line.n, 1 do
      h = max(h, line[i].h)
   end

   line.h = h
   mode:align_line_h(h)
   mode:align_line_v(h)

   tinsert(mode.lines, line)
   mode.curline = { n = 0 }
   mode.h = mode.h + h

   mode.total_w = max(mode.total_w, mode.w)
   mode.total_h = mode.h

   mode.w = 0
end

function tt_insert_block(mode, block)
   local w = mode.w

   if w + block.w > mode.bw then
      tt_endline(mode)
      w = 0
   end

   block.x = w
   block.y = mode.h
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
   mode.border = mode.border or { 6, 6 }
   mode.total_w = 0
   mode.total_h = 0
   
   mode.box = tt_copymode(mode.box)

   if mode.x == "left" then
      mode.align_line_h = tt_align_line_left
      mode.box[1] = mode.box[1] + mode.border[1]
      mode.box[3] = mode.box[3] - mode.border[1]
   elseif mode.x == "leftout" then
      mode.align_line_h = tt_align_line_right
      mode.box[3] = mode.box[1]
      mode.box[1] = mode.outbox[1]
   elseif mode.x == "right" then
      mode.align_line_h = tt_align_line_right
      mode.box[1] = mode.box[1] + mode.border[1]
      mode.box[3] = mode.box[3] - mode.border[1]
   elseif mode.x == "rightout" then
      mode.align_line_h = tt_align_line_left
      mode.box[1] = mode.box[3]
      mode.box[3] = mode.outbox[3]
   elseif mode.x == "center" then
      mode.align_line_h = tt_align_line_center
   end

   if mode.y == "up" then
      mode.align_v = tt_align_up
      mode.box[2] = mode.box[2] + mode.border[2]
      mode.box[4] = mode.box[4] - mode.border[2]
   elseif mode.y == "upout" then
      mode.align_v = tt_align_down
      mode.box[4] = mode.box[2]
      mode.box[2] = mode.outbox[2]
   elseif mode.y == "down" then
      mode.align_v = tt_align_down
      mode.box[2] = mode.box[2] + mode.border[2]
      mode.box[4] = mode.box[4] - mode.border[2]
   elseif mode.y == "downout" then
      mode.align_v = tt_align_up
      mode.box[2] = mode.box[4]
      mode.box[4] = mode.outbox[4]
   elseif mode.y == "center" then
      mode.align_v = tt_align_vcenter
   end

   mode.align_line_v = mode.align_line_v or tt_align_line_vcenter
   mode.align_line_h = mode.align_line_h or tt_align_line_left
   mode.align_v = mode.align_v or tt_align_up

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

   mode:align_v()

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


if not nil then

   box = { 100, 100, 600, 400 }
   tt = tt_build(
		 [[
		       Hello <img name="vmu" scale="0.5"> World ! <br>
			  <center> <font size="24" color="#00ffff"> Centered !! <br>
			  <right> <font size="14" color="#ffffff"> right aligned <font size="48" color="#ffff00"> <img name="dcplaya" src="dcplaya.tga"> TITI <img name="colorpicker" src="colorpicker.tga"> TOTO TUTU
		 ]], 
		 { 
		    --		 x = "leftout",
		    y = "down",
		    box = box,
		    z = 300
		 })

   gui_dialog_box_draw(tt.dl, box, tt.z-10, gui_box_color1, gui_box_color1)

   tt_draw(tt)

   --print(tt.total_w, tt.total_h)

end

taggedtext_loaded = 1
