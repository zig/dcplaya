--- @file   taggedtext.lua
--- @author Vincent Penne <ziggy@sashipa.com>
--- @brief  sgml text and gui element formater.
---
--- $Id: taggedtext.lua,v 1.27 2003-03-20 21:59:19 ben Exp $
---

if not dolib("dirfunc") then return end
if not dolib("sprite") then return end
if not dolib("color") then return end

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
--    if block.spr.rotate then
--       sprite_draw(block.spr, block.dl, block.x+ly, block.y+lx, block.z, sx, sy)
--    else
   sprite_draw(block.spr, block.dl, block.x+lx, block.y+ly, block.z, sx, sy)
--    end
end

function tt_img_name(src)
   return get_nude_name(src) or src
end

function tt_img_cmd(mode, param)
   local src = param.src
   local name = param.name or tt_img_name(src)
   if not name then return end
   local spr = sprite_get(name)
   if not spr then
      if not src then return end
      --local tex = tex_new()
      --print("'"..src.."'")
      local tex = tex_exist(src) or tex_new(home.."lua/rsc/icons/"..src)
      --print(tex)
      if tex then
	 local info = tex_info(tex)
	 local w, h = info.w, info.h
	 local orig_w, orig_h = info.orig_w, info.orig_h
	 rotate = param.rotate
	 spr = sprite(name, 0, 0, orig_w, orig_h,
		      0, 0, orig_w/w, orig_h/h, tex, rotate)
      end
   end

   if spr then
      local sx, sy
      sx = param.sx or param.scale
      sy = param.sy or sx
      local w, h
      w = param.w
      h = param.h
      if w then
	 sx = w/spr.w
	 if h then
	    sy = h/spr.h
	 else
	    sy = sx
	 end
      elseif h then
	 sy = h/spr.h
	 sx = param.sx or sy
      end

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

--- Tagged text color table.
--- @TODO Add some more colors
tt_color_table = {
   -- Get gui current colors
   box_bcolor = function() return gui_box_bcolor end,
   box_color1 = function() return gui_box_color1 end,
   box_color2 = function() return gui_box_color2 end,
   button_bcolor = function() return gui_button_bcolor end,
   button_color1 = function() return gui_button_color1 end,
   button_color2 = function() return gui_button_color2 end,
   text_color = function() return gui_text_color end,
   text_shiftbox = function() return gui_text_shiftbox end,
   input_color1 = function() return gui_input_color1 end,
   input_color2 = function() return gui_input_color2 end,
   input_cursor_color1 = function() return gui_input_cursor_color1 end,
   input_cursor_color2 = function() return gui_input_cursor_color2 end,
}

-- UGLY !!!  - $$$ ben : not anymore :)
function tt_tocolor(s)
   if type(s) == "string" then
      local first = strsub(s,1,1)
      if first then
	 if first == "$" then
	    s = getglobal(strsub(s,2))
	 elseif first ~= "#" then
	    s = tt_color_table[s] or s
	 end
      end
   end

   -- A function : run it !
   if type(s) == "function" then
      s = s()
   end

   -- Just reference colors
   if type(s) == "table" then
      return s
   end
   -- Other type : create a new color 
   return color_new(s)
end


function tt_font_cmd(mode, param)

   mode.font_h = tonumber(param.size) or mode.font_h
   if param.color then
      mode.color = tt_tocolor(param.color)
   end
   mode.font_id = tonumber(param.id) or mode.font_id

   local block = {
      type = "font_style",
      dl = mode.dl,
      id = mode.font_id,
      h = mode.font_h,
      w = 0,
      h = 0,
      draw = function(block)
		-- $$$ ben hack : Remove filtering for font 1 (fixed)
		dl_text_prop(block.dl, block.id, block.h, nil,
			     block.id and (block.id ~= 1 or 0))
	     end
   }
   return block
end

function tt_dialog_draw(block)
   local papp
   local mode = block.tt
   papp = mode.parent.app
   local box = { block.x, block.y, block.x + block.w, block.y + block.h }
   mode.box = box + { 8, 8, -8, -8 }
   dl_set_trans(mode.dl, mat_trans(0, 0, block.z))
   local app = gui_new_dialog(papp, box, nil, nil, mode.label, nil, mode.name)
   -- $$$ added by ben for application icon support.
   app.icon_name = mode.icon
   app.icon_file = mode.src

   app.guis = mode.guis
   mode.app = app
   gui_label(app, mode)
   --print(mode.label, mode.guiref)
   if mode.guiref then
      --print("dialog", mode.guiref)
      mode.parent.guis[mode.guiref] = app
   end
end

function tt_dialog_cmd(mode, param)
   param.x = param.x or "left"
   param.y = param.y or "up"
   param.border = { 0, 0 }
   param.box = {
      0, 0, 
      param.hint_w or param.total_w or mode.box[3] - mode.box[1],
      param.hint_h or param.total_h or mode.box[4] - mode.box[2]
   }
   if param.color then
      param.color = tt_tocolor(param.color)
   else
      param.color = gui_text_color
   end
   local newmode = tt_build("", param)
   
   newmode.parent = mode
   newmode.parent_cmd = tt_end_dialog_cmd
   newmode.z = 10
   
   return nil, newmode
end

function tt_end_dialog_cmd(mode)
   mode = tt_endgroup(mode, tt_end_dialog_cmd)

   local block = {
      type = "dialog",
      w = mode.total_w + 20,
      h = mode.total_h + 20,
      fill_w = mode.fill_x,
      fill_h = mode.fill_y,
      z = mode.z,
      tt = mode,
      draw = tt_dialog_draw,
   }

   return block, mode.parent
end


function tt_button_draw(block)
   local papp
   local mode = block.tt
   papp = mode.parent.app
   if not papp then
      return
   end
   local box = { block.x, block.y, block.x + block.w, block.y + block.h }
   mode.box = box + { 4, 4, -4, -4 }
   dl_set_trans(mode.dl, mat_trans(0, 0, block.z))
   local name = mode.name or (papp.name and (papp.name .. "-button"))
   local app = gui_new_button(papp, box, nil, nil, nil, name)
   app.guis = mode.guis
   mode.app = app
   gui_label(app, mode)
   if mode.guiref then
      --print("button", mode.guiref)
      mode.parent.guis[mode.guiref] = app
   end
end

function tt_button_cmd(mode, param)
   param.x = param.x or "center"
   param.y = param.y or "center"
   param.border = { 0, 0 }
   param.box = {
      0, 0, 
      param.hint_w or param.total_w or mode.box[3] - mode.box[1],
      param.hint_h or param.total_h or mode.box[4] - mode.box[2]
   }
   if param.color then
      param.color = tt_tocolor(param.color)
   else
      param.color = gui_text_color
   end
   local newmode = tt_build("", param)
   
   newmode.parent = mode
   newmode.parent_cmd = tt_end_button_cmd
   newmode.z = 20
   newmode.box = {
      0, 0, 
      param.w or 640,
      param.h or 480
   }
   
   return nil, newmode
end

function tt_end_button_cmd(mode)
   mode = tt_endgroup(mode, tt_end_button_cmd)

   local block = {
      type = "button",
      w = mode.total_w + 12,
      h = mode.total_h + 12,
      fill_w = mode.fill_x,
      fill_h = mode.fill_y,
      z = mode.z,
      tt = mode,
      draw = tt_button_draw,
   }

   return block, mode.parent
end

function tt_a_name_draw(block)
   local tt = block.tt
--   print("New anchor", block.name)
   tt.anchors[block.name] = {
      x=block.x,
      y=block.y,
   }
end

function tt_a_cmd(mode, param)
   if param.name then
      local block = {
	 type = "anchor",
	 w = 0,
	 h = 0,
	 z = mode.z,
	 name = param.name,
	 tt = mode,
	 draw = tt_a_name_draw,
      }
      return block

   elseif param.href then
   end
end

function tt_end_a_cmd(mode, param)
end

-- all commands
tt_commands = {

   img = tt_img_cmd,

   br = function(mode)
	   tt_endline(mode)
	end,

   p = function(mode, param)
	  tt_endline(mode)
	  mode.vspace = param.vspace
       end,

   pre = function(mode,param)
	    if param.color then
	       mode.color = tt_tocolor(param.color)
	    end
	    if mode.preformatted then
	       print ("ERROR NESTED <PRE>")
	    end
	    mode.preformatted = 1
	    -- Preformatted default don't wrap
	    mode.wrap = param.wrap or mode.wrap or 0
	    mode.tabsize = tonumber(param.tabsize) or mode.tabsize or 3
	    if mode.tabsize <= 0 then mode.tabsize = 3 end
	    mode.tabchar = param.tabchar or mode.tabchar or " "
	    -- 	    printf ("<PRE tab=%d char=%q>",mode.tabsize,mode.tabchar)
	 end,

   ["/pre"] = function(mode,param)
		 if not mode.preformatted then
		    print ("WARNING TT </PRE> without <PRE>")
		 end
		 mode.preformatted = nil
	      end,

   ["nowrap"] = function(mode,param)
		   mode.wrap = 0
		end,
   
   ["wrap"] = function(mode,param)
		    mode.wrap = 1
		 end,

   lineup = function(mode)
	     mode.align_line_v = tt_align_line_up
	  end,

   linedown = function(mode)
	       mode.align_line_v = tt_align_line_down
	    end,

   linecenter = function(mode)
	       mode.align_line_v = tt_align_line_vcenter
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

   font = tt_font_cmd,

   vspace = function(mode, param)
	       local h = param.h or 16
--	       mode.h = mode.h + h
	       tt_endline(mode)
	       tt_insert_block(mode, { w = 0, h = h, draw = function() end })
	       tt_endline(mode)
	    end,

   hspace = function(mode, param)
	       local w = param.w or 16
--	       mode.w = mode.w + w
	       return { w = w, h = 0, draw = function() end }
	    end,


   macro = function(mode, param)
	      local name = param["macro-name"]
	      local cmd = param["macro-cmd"]
	      if name and cmd and tt_commands[cmd] then
--		 print("macro", name, cmd)
		 local fcmd = tt_commands[cmd]
		 local mp = tt_copymode(param)
		 tt_commands[name] = function(mode, param)
					local i, v
					for i,v in %mp do
--					   print("macro param", i, v, param[i])
					   param[i] = param[i] or v
					end
					return %fcmd(mode, param)
				     end
	      end
		 
	   end,

   ["dialog"] = tt_dialog_cmd,
   ["/dialog"] = tt_end_dialog_cmd,

   ["button"] = tt_button_cmd,
   ["/button"] = tt_end_button_cmd,

   ["a"] = tt_a_cmd,
   ["/a"] = tt_end_a_cmd,

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
	 word = strsub(text, e1, e2-1)
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

-- $$$ Added by ben. Don't really know what I do here, but it seems ok.
function tt_align_line_up(mode, line, h)
end

function tt_align_line_down(mode, line, h)
   local i
   for i=1, line.n, 1 do
      line[i].y = line[i].y - line[i].h + h
   end
end

function tt_align_line_vcenter(mode, line, h)
   local i
   for i=1, line.n, 1 do
      line[i].y = line[i].y + ( h - line[i].h ) / 2
   end
end

function tt_align_line_left(mode, line, h)
   local i
   local x = mode.box[1]
   for i=1, line.n, 1 do
      line[i].x = line[i].x + x
   end
end

function tt_align_line_right(mode, line, h)
   local i
   local x = mode.box[3] - line.w
   for i=1, line.n, 1 do
      line[i].x = line[i].x + x
   end
end

function tt_align_line_center(mode, line, h)
   local i
   local x = (mode.box[1] + mode.box[3] - line.w)/2
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
   if mode.curline.n < 1 then
      return
   end

   local h = 0
   local line = mode.curline
   local i

   for i=1, line.n, 1 do 
      local block = line[i]
      h = max(h, block.h)
   end
   -- $$$ ben : fix multiple line break.
   if h == 0 and line.n > 0 and line[1].type and line[1].type == "text" then
      h = line[1].font_h or 16
   end

   line.fill_totalw = mode.fill_totalw
   line.h = h
   line.w = mode.w
   line.align_line_h = mode.align_line_h
   line.align_line_v = mode.align_line_v

   tinsert(mode.lines, line)
   mode.curline = { n = 0 }
   mode.h = mode.h + h + (mode.vspace or 0)

   mode.total_w = max(mode.total_w, mode.w)
   mode.total_h = max(mode.total_h, mode.h)

   mode.w = 0
   mode.fill_totalw = 0
end

function tt_insert_block(mode, block)
   local w = mode.w
   local wrap = mode.wrap or 1 -- Default mode is wrapping

   -- $$$ add wrapping test here !
   if wrap == 1 and w + block.w > mode.bw then
      tt_endline(mode)
      w = 0
      mode.fill_totalw = 0
   end

   block.y = mode.h

   tinsert(mode.curline, block)
   mode.w = w + block.w
   if block.fill_w then
      mode.fill_totalw = mode.fill_totalw + block.fill_w
   end
end


function tt_endgroup(mode, cur)
   tt_endline(mode)
   
   while mode.parent and mode.parent_cmd ~= cur do
      local block = mode:parent_cmd()
      mode = mode.parent
      if block then
	 tt_insert_block(mode, block)
      end
   end
   return mode
end

--- process a tagged text.
function tt_build(text, mode)
   if not tt_tag then
      tt_tag = newtag()
   end

   if tag(text) == tt_tag then
      return text
   end

   if type(text) ~= "string" then
      return
   end

   if not mode then
      mode = { }
   end

   settag(mode, tt_tag)
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
   mode.w = 0
   mode.h = 0
   mode.border = mode.border or { 8, 8 }
   mode.total_w = mode.total_w or 0
   mode.total_h = mode.total_h or 0
   mode.fill_totalw = 0
   mode.guis = mode.guis or { }
   mode.anchors = mode.anchors or { }
   
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
 
   local start,stop = 1,0
   local len = strlen(text)

   -- $$$ added by ben : text without tags
   local text_nude = ""

   if mode.font_h ~= 16 then
      tt_insert_block(mode, tt_font_cmd(mode, { size = mode.font_h }))
   end

   local countermax = 80
  
   -- Skip starting sapces.
   start,stop = strfind(text, "%s*", start)
   start = stop + 1
   local prevstart = start-1

   while start <= len do

      if prevstart >= start then
	 printf("tagged text internal error : rollback [%d %d]",
		prevstart, start)
	 return
      end
      prevstart = start


--      local e1, e2 = strfind(text, "[%s%p]*%w+[%s%p]+", start)
      local e1, e2
--      printf("READ @ %d",start)
      if not mode.preformatted then
	 e1,e2 = strfind(text, "[^%s]+%s+", start)
      else
	 e1,e2 = strfind(text, ".-\n", start)
      end
      if not e2 then
--	 printf("last token [%s],%d %d",strsub(text,start,len),start,len)
	 e2 = len
      end


      -- Get next word
      local t = strsub(text, start, e2)

      local tag
      local e3, e4
      if start < e2 then
	 if mode.preformatted then
	    e3, e4 = strfind(t, "</pre>")
	 else
	    e3, e4 = strfind(t, "<")
	 end
	 if e3 then
	    e3, e4 = strfind(text, "%b<>", start)
	    
	 end
	 if e3 then
	    -- Get text
	    e2 = e3-1
	    t = strsub(text, start, e2)
	    -- Get the tag if any
	    tag = strsub(text, e3+1, e4-1)
	 end
      end

      -- Create text block
      if start <= e2 then
	 local newline
	 local block = {
	    type = "text",
	    draw = tt_text_draw,
	    dl = mode.dl,
	    font_id = mode.font_id,
	    font_h = mode.font_h,
	    font_aspect = mode.font_aspect,
	    z = mode.z,
	    color = mode.color,
	 }

	 if not mode.preformatted then
	    -- not preformatted : remove any space and newline
	    block.text = (gsub(t, "%s", "").." ")
	 else
	    -- preformatted, Keep the text as is but insert EOL when '\n'

	    newline = strsub(t,-1) == "\n"
	    if newline then
	       t = strsub(t, 1, -2)
	    end
	    if t ~= "" then
	       -- Do tabstop
	       local s,l = 1,strlen(t)
	       local a,b
	       local pos = 0

	       block.text = ""
	       local tab = strchar(9)

	       while s <= l do
		  a,b = strfind(t,tab,s)
		  if not a then
		     break
		  end
		  -- Copy before tab
		  block.text = block.text .. strsub(t,s,a-1)
		  pos = pos + (a - s)
		  local tabstop = pos+mode.tabsize-intop(pos,'%',mode.tabsize)
		  tabstop = tabstop - pos
		  block.text = block.text .. strrep(mode.tabchar,tabstop)
		  pos = pos + tabstop

-- 		  if (pos ~= strlen(block.text)) then
-- 		     printf("tab-error : %d ~= %d", pos, strlen(block.text))
-- 		  end
-- 		  if intop(pos,'%',mode.tabsize) ~= 0 then
-- 		     print("tab error : pos mod = "..intop(pos,'%',mode.tabsize))
-- 		  end
		  s = a + 1
	       end
	       block.text = block.text .. strsub(t, s)
	    else
	       block = nil
	    end
	 end
	 
	 if block then
	    block.w, block.h =
	       dl_measure_text(block.dl, block.text,
			       block.font_id, block.font_h,
			       block.font_aspect)
	    text_nude = text_nude .. block.text
	    tt_insert_block(mode, block)
	 end
	 if newline then
	    tt_endline(mode)
	 end

	 start = e2 + 1
      end
      
      if tag then

	 e2 = e4

	 local i = 1
	 local command
	 command, i = tt_getword(tag, i)
	 if command then
	    local param = { }
	    local key, value
	    key, value, i = tt_getparam(tag, i)
	    while key do
	       param[key] = value
	       key, value, i = tt_getparam(tag, i)
	    end

	    local f = tt_commands[command]

	    if f then
	       local newmode
	       block, newmode = f(mode, param)
	       if newmode then
		  mode = newmode
	       end
	       if block then
		  tt_insert_block(mode, block)
	       end
	    end
	 end
	 start = e2 + 1
      end


   end

   -- end all unclosed groups
   mode = tt_endgroup(mode)

   tt_endline(mode)

   mode.text_nude = text_nude

--   print("tt_text_nude:".. mode.text_nude)

   return mode
end


--- draw a tagged text (must be previously build with tt_build)
function tt_draw(tt)
   local i, j

   tt:align_v()

   for i=1, tt.lines.n, 1 do
      local line = tt.lines[i]

      local h = line.h

      local w = 0

      local fill = tt.total_w - line.w
      local totalw = line.fill_totalw

      for j=1, line.n, 1 do
	 local block = line[j]
	 block.x = w

	 if block.fill_w then
	    block.w = block.w + fill * block.fill_w / totalw
	    line.align_line_h = tt_align_line_left
	 end
	 w = w + block.w
      end

      line.align_line_h(tt, line, h)
      line.align_line_v(tt, line, h)

      for j=1, line.n, 1 do
	 local block = line[j]
	 block:draw()
      end
   end
end

-- Add filetype for text and zml (Ziggy Markup Language)
if type(filetype_add) == "function" then
   filetype_add("text", nil, ".zml\0")
end

return 1
