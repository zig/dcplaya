--- @ingroup dcplaya_lua_gui
--- @file    textviewer.lua
--- @date    2002/12/06
--- @author  benjamin gerard
--- @brief   hyper text viewer gui.
---
--- $Id: textviewer.lua,v 1.23 2004-07-31 22:55:18 vincentp Exp $
---

if not dolib("taggedtext") then return end
if not dolib("gui") then return end
if not dolib("menu") then return end

--- @defgroup  dcplaya_lua_tv_gui  Text Viewer
--- @ingroup   dcplaya_lua_gui
--- @brief     text viewer
--- @author    benjamin gerard
--- @{
--

--- Create a taggeg text from a string that may be either a file or a text.
--- @internal
function text_viever_tt_build(text, mode)
   local dl_len_scale = 1.5
   mode = mode or {}
   if type(text) ~= "string" then
      mode.dl = dl_new_list(256,0,1)
      return tt_build(format("Invalid tagged text %q.<br>",type(text)), mode)
   elseif strsub(text,1,1) == "/" then
      local buffer
      local file = openfile(text,"r")
      if file then
	 printf("loading tagged-text %q\n",text)
	 buffer = read(file,"*a")
	 closefile(file)
      end
      if type(buffer) ~= "string" then
	 buffer = format("file %q read error.<br>", text)
      end
      mode.dl = dl_new_list(strlen(buffer) * dl_len_scale, 0, 1, "tv_tt")
      return tt_build(buffer,mode)
   else
      mode.dl = dl_new_list(strlen(text) * dl_len_scale, 0,1)
      return tt_build(text,mode)
   end
end

--- Create a text view gui application.
--
--- @param  owner  owner applciation (default is desktop)
--- @param  texts  string or table of string.
--- @param  box    application box
--- @param  label  dialog label (optionnal)
--- @param  mode   label mode.
--- @param  help   optionnal tagged text for help
--- @return dialog application
---
function gui_text_viewer(owner, texts, box, label, mode, help)

   if type(box) == "number" then
      box = (640-box) / 2
      box = { box,50, 640-box, 400 }
   end

   --- Desactive a tagged text.
   --- @internal
   function gui_text_viewer_desactive_tt(tt)
      if tag(tt) ~= tt_tag then return end
      local a,r,g,b = dl_get_color(tt.dl)
      dl_set_color(tt.dl,0,r,g,b)
      dl_set_active(tt.dl,0)
   end

   --- Set current displayed tagged text.
   --- @param dial textviewer dialog
   --- @param tt   tagged-text or anchor string. Anchor string format is
   ---             [text-name][#anchor-name]. If an anchor is specified, it
   ---             override x and y parameters.
   --- @param x    x scrolling position
   --- @param y    y scrolling position
   --
   function gui_text_viewer_set_tt(dial, tt, x, y)
      if type(tt) == "string" then
	 local mf = "[%w%s%_%-%,%/%.]"
	 local start, stop, tt_name, hash, tt_anchor =
	    strfind(tt,"("..mf.."*)(#?)("..mf.."*)")
	 tt = (type(tt_name) == "string" and
	       type(dial.tts) == "table" and
		  dial.tts[tt_name]
	    ) or dial.cur_tt
	 if tag(tt) == tt_tag and type(tt_anchor) == "string" and
	    tt.anchors[tt_anchor] then
	    x,y = -tt.anchors[tt_anchor].x,-tt.anchors[tt_anchor].y
	 end
      end
      tt = (tag(tt) == tt_tag) and tt

      if tt ~= dial.cur_tt then
	 if dial.old_tt ~= tt then
	    gui_text_viewer_desactive_tt(dial.old_tt)
	 end
	 dial.old_tt = dial.cur_tt
	 dial.cur_tt = tt
      end
      gui_text_viewer_set_scroll(dial, x, y)
      dial.tt_a = 1
   end

   --- Set current tagged text-scrolling position.
   --- This function ensure that scrolling position is inside taged-text box.
   --- @param dial text-viewer dialog
   --- @param x    x scrolling position
   --- @param y    y scrolling position
   --
   function gui_text_viewer_set_scroll(dial, x, y)
      local mat
      local tt = dial.cur_tt
      if tag(tt) ~= tt_tag or not dial.tbox then return end
      local tw,th = dial.tbox[3], dial.tbox[4]
      local ttw,tth = tt.total_w, tt.total_h

      local xmin,xmax,ymin,ymax

      if tw > ttw then
	 xmin, xmax = 0, 0
      else
	 xmin, xmax = tw-ttw, 0
      end

      if th > tth then
	 ymin, ymax = 0,0
      else
	 ymin, ymax = th-tth, 0
      end

      x = x or 0
      y = y or 0
      x = (x < xmin and xmin) or (x > xmax and xmax) or x
      y = (y < ymin and ymin) or (y > ymax and ymax) or y

      dial.tt_x = x
      dial.tt_y = y

   end

   local border = 8
   local dial
   local buttext = "close"

   -- Button size
   local butw,buth = dl_measure_text(nil,buttext)
   butw = butw + 12
   buth = buth + 8

   if not box then
      box = { 0,0,320,240 }
      local x,y = (640 - box[3]) * 0.5, (480 - box[4]) * 0.5
      box = box + { x, y, x, y }
   end

   local tbox
   tbox = box + { border, border,-border, -(2*border+buth) }

   local minx,miny = 32,32
   local tx,ty,tw,th = tbox[1], tbox[2], tbox[3]-tbox[1], tbox[4]-tbox[2]
   local x,y
   x = (tw < minx and (minx-tw)/2) or 0
   y = (th < miny and (miny-th)/2) or 0

   box = box + { -x, -y, x, y }
   tbox = tbox + { -x, -y, x, y }

   -- Get final text box dim
   tw = tbox[3]-tbox[1]-12
   th = tbox[4]-tbox[2]-16
   tx = tx + 6 
   ty = ty + 8 

   -- Creates tagged texts
   local tts = {}

   local ttbox0 = { 0,0,tw,th }
   
   text_viever_tt_build(texts, { box = ttbox0 })

   if type(texts) == "string" then
      tts[1] = text_viever_tt_build(texts, { box = ttbox0 })
   elseif type(texts) == "table" then
      local i,v
      for i,v in texts do
	 if type(v) == "string" then
	    tts[i] = text_viever_tt_build(v, { box = ttbox0 })
	 elseif tag(v) == tt_tag then
	    tts[i] = v
	 end
      end
   end

   -- Create help
   if type(help) ~= "string" then
      help = '<center><font color="text_color">'
	 .. '\017 .. Toggle focus<br>'
	 .. '\018 .. Context menu (change text or go to mark)<br>'
   end
   local help_w,help_h,help_y = 0,0,0
   local help_tt = tt_build(help, { box = ttbox0 } )
   if help_tt then
      help_w, help_h = help_tt.total_w, help_tt.total_h + 6
      help_y = tbox[4] + 3
      box[4] = box[4] + help_h
   end
   -- Create main-dialog
   dial = gui_new_dialog(owner, box, nil, nil, label, mode, "text viewer")
   if not dial then return end
   dial.icon_name = "view"

   -- Create view-dialog
   dial.viewer = gui_new_dialog(dial, tbox, nil, nil, nil, nil,
				"viewer_area", {transparent=1})
   if not dial.viewer then return end
   dial.tbox = ttbox0

   -- Display help 
   if help_tt then
      local b = dial.viewer.box
      if b then
	 tt_draw(help_tt)
 	 dl_set_trans(help_tt.dl,
 		      mat_trans(tbox[1], help_y, 10))
	 dl_sublist(dial.dl,help_tt.dl)
      end
   end

   -- Create button
   local bbox = {( box[1] + box[3] - butw) * 0.5, (box[4] - buth - border) }
   bbox[3] = bbox[1] + butw
   bbox[4] = bbox[2] + buth
   dial.button = gui_new_button(dial, bbox, buttext , mode, nil, "close")
   if not dial.button then return end
   dial.button.event_table[gui_press_event] =
      function(app, evt)
	 evt_shutdown_app(app.owner)
	 app.owner.answer = "close"
      end
   dial.button.event_table[gui_cancel_event] =
      function(app, evt)
	 gui_new_focus(app.owner, app.owner.viewer)
      end
   
   
   -- Display tagged-texts
   local tt_dl = dl_new_list(256,1,1) -- For clipping
   dl_set_clipping(tt_dl, 0, 0, tw, th)
   dl_set_trans(tt_dl, mat_trans(tx,ty,90))
   local tt_dl2 = dl_new_list(256,1,1) -- For scrolling
   dl_sublist(tt_dl, tt_dl2)
   for i,v in tts do
      if tag(v) == tt_tag then
	 gui_text_viewer_desactive_tt(v)
	 if not dial.cur_tt then
	    gui_text_viewer_set_tt(dial, v, 0, 0)
	 end
	 dl_sublist(tt_dl2, v.dl)
	 tt_draw(v)
      end
   end
   dl_sublist(dial.viewer.dl,tt_dl)
   dial.tt_dl = tt_dl2
   dial.tts = tts

   --- Process tagged text fading.
   --- @internal
   function gui_text_viewer_fadeto(tt, alpha, spd)
      if not alpha or not tt or not spd then return end
      local a,r,g,b = dl_get_color(tt.dl)
      if a < alpha then
	 a = a + spd
	 if a >= alpha then
	    a = alpha
	    alpha = nil
	 end
      else
	 a = a - spd
	 if a <= alpha then
	    a = alpha
	    alpha = nil
	 end
      end
      dl_set_color(tt.dl,a,r,g,b)
      dl_set_active(tt.dl, a > 0)
      return alpha
   end

   --- Process tagged text scrolling.
   --- @internal
   function gui_text_viewer_scrollto(x, xgoal)
      if xgoal and x then
	 local scrollmax = 64
	 local newx = xgoal * 0.2 + x * 0.8
	 local movx = newx - x
	 local amovx = abs(movx)
	 if amovx > scrollmax then movx = movx * scrollmax / amovx end
	 x = x + movx
	 if abs(x-xgoal) < 0.01 then
	    x = xgoal
	    xgoal = nil
	 end
      end
      return x, xgoal
   end

   --- Text viewer gui application update. 
   --- @internal
   function gui_text_viewer_update(dial, frametime)
      local fadespd = 1 * frametime

      -- Fade in current TT 
      dial.tt_a = gui_text_viewer_fadeto(dial.cur_tt, dial.tt_a, fadespd)

      -- Fade out old TT
      if not gui_text_viewer_fadeto(dial.old_tt, 0, fadespd) then
	 dial.old_tt = nil
      end

      -- Update scrolling
      if dial.tt_dl and (dial.tt_x or dial.tt_y) then
	 local mat = dl_get_trans(dial.tt_dl)
	 local x,y = mat[4][1], mat[4][2]
	 x, dial.tt_x = gui_text_viewer_scrollto(x, dial.tt_x)
	 y, dial.tt_y = gui_text_viewer_scrollto(y, dial.tt_y)
	 mat[4][1], mat[4][2] = x,y
	 dl_set_trans(dial.tt_dl, mat)
      end

      -- Run "original" dialog update
      if type (dial.tv_old_update) == "function" then
	 dial.tv_old_update(dial, frametime)
      end
   end

   -- Patch dialog up/down/left/right events

   function gui_text_viewer_move_handle(dial, x, y)
      local mat = dl_get_trans(dial.tt_dl)
      if mat then
	 gui_text_viewer_set_scroll(dial, mat[4][1]+x, mat[4][2] + y)
      end

   end

   function gui_text_viewer_up(dial, evt)
      gui_text_viewer_move_handle(dial, 0, 16)
   end

   function gui_text_viewer_dw(dial, evt)
      gui_text_viewer_move_handle(dial, 0, -16)
   end

   function gui_text_viewer_lt(dial, evt)
      gui_text_viewer_move_handle(dial, 16, 0)
   end

   function gui_text_viewer_rt(dial, evt)
      gui_text_viewer_move_handle(dial, -16, 0)
   end

   local j,w 
   for j,w in {
      { gui_text_viewer_up, gui_keyup },
      { gui_text_viewer_dw, gui_keydown },
      { gui_text_viewer_lt, gui_keyleft },
      { gui_text_viewer_rt, gui_keyright } } do

      for i,v in w[2] do
	 dial.event_table[i] = w[1]
      end
   end

   dial.viewer.event_table[gui_cancel_event] = 
      function(app, evt)
	 gui_new_focus(app.owner, app.owner.button)
      end

   --   dial.viewer.event_table[gui_select_event] = 
   --      function(app, evt)
   --	 local dial = app.owner
   --	 if not dial then return end
   --	 local name = (dial.name or "textviewer") .. "-menu"
   --	 local def = menu_create_defs(gui_text_viewer_menucreator, dial)
   --	 dial.menu = gui_menu(dial, name, def)
   --	 if tag(dial.menu) == menu_tag then
   --	    dial.menu.target = dial
   --	 end
   --      end

   for j, _ in gui_keyselect do
      dial.event_table[j] = 
	 function(app, evt)
	    local dial = app
	    app = app.viewer
	    if not dial then return end
	    local name = (dial.name or "textviewer") .. "-menu"
	    local def = menu_create_defs(gui_text_viewer_menucreator, dial)
	    dial.menu = gui_menu(dial, name, def)
	    if tag(dial.menu) == menu_tag then
	       dial.menu.target = dial
	    end
	 end
   end

   function gui_text_viewer_menucreator(target)
      local dial = target;
      if not dial then return end
      
      local cb = {
	 settext = function(menu)
		      local dial, fl = menu.root_menu.target, menu.fl
		      if not fl or not dial then return end
		      local pos = fl:get_pos()
		      if not pos then return end
		      local entry = fl.dir[pos]
		      if entry then
			 local tt_name = ((menu.name == "anchor" and "#") or
					  "") .. entry.name
			 gui_text_viewer_set_tt(dial, tt_name)
		      end
		   end,
      }
      local root = ":" .. target.name .. ":" .. 
	 "view >view,anchor >anchor"
      local view = ":view:"
      if type(dial.tts) == "table" then
	 local i,v
	 for i,v in dial.tts do
	    if tag(v) == tt_tag then
	       view = view .. tostring(i) .. "{settext},"
	    end
	 end
      end
      local anchor = ":anchor:"
      if tag(dial.cur_tt) == tt_tag and 
	 type(dial.cur_tt.anchors) == "table" then
	 local i,v
	 local tmp = {}
	 for i,v in dial.cur_tt.anchors do
	    tinsert(tmp,
		    { pos = v.y * 4000 + v.x , name = tostring(i) }
		 )
	 end
	 tmp.n = nil
	 sort(tmp,
	      function (a,b) return a.pos < b.pos end )
	 for i,v in tmp do
	    anchor = anchor .. tostring(v.name) .. "{settext},"
	 end
      end

      local def = {
	 root=root,
	 cb = cb,
	 sub = { view = view, anchor = anchor }
      }
      return menu_create_defs(def , target)
   end

   -- Install new update 
   dial.tv_old_update = dial.update
   dial.update = gui_text_viewer_update

   -- Install menu
   dial.mainmenu_def = gui_text_viewer_menucreator

   -- make sure the button is focused
   gui_new_focus(dial, dial.button)

   return dial
end

--- Create a text-viewer application from a file.
--- @param  owner  owner applciation (default is desktop)
--- @param  fname  Filename to view
--- @param  box    application box
--- @param  label  dialog label (optionnal)
--- @param  mode   label mode.
--- @param  preformatted  if defined text is load as preformatted and
---                       preformatted is the tab size.
--- @see gui_text_viewer()
---
function gui_file_viewer(owner, fname, box, label, mode, preformatted)
   fname = canonical_path(fname)
   if test("-f",fname) then
      local file = openfile(fname,"r")
      local name = get_nude_name(fname) or "text"
      if not file then return end
      local header,footer
      if preformatted then
	 header = format('<font id="1" size="16" color="text_color"><pre tabsize="%d" tabchar=" ">', preformatted)
	 footer = '</pre><font id="0" size="16">'
      else
	 header,footer = "",""
      end
      local tmp = read(file,"*a")
      closefile(file)
      if tmp then
	 tmp = { top = header .. tmp  .. footer }
      end
      return gui_text_viewer(owner,tmp,box,label,mode)
   end
end

--
--- @}
---

-- Create application icon sprite
sprite_simple(nil,"view.tga")

return 1
