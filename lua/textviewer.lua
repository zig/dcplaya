--- @ingroup dcplaya_lua_gui
--- @file   textviewer.lua
--- @date   2002/12/06
--- @author benjamin gerard <ben@sashipa.com>
--- @brief  hyper text viwer gui
--- $Id: textviewer.lua,v 1.2 2003-01-13 04:09:52 ben Exp $
---

if not dolib("taggedtext") then return end
if not dolib("gui") then return end
if not dolib("menu") then return end

--- @name text viewer
--- @ingroup dcplaya_lua_gui
--- @{
--

--- Create a text view gui application.
--
--- @param  owner  owner applciation (default is desktop)
--- @param  texts  string or table of string.
--- @param  application box
--- @param  dialog label (optionnal)
--- @param  mode label mode.
--- @return dialog application
---
function gui_text_viewer(owner, texts, box, label, mode)

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
	 local start, stop, tt_name, hash, tt_anchor =
	    strfind(tt,"(%w*)(#?)(%w*)")
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
      box = { 0,0,320,200 }
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

   local ttbox0 = { 0,0,tw,th}
   
   if type(texts) == "string" then
      tts[1] = tt_build(texts, { dl = dl_new_list(strlen(texts),0,1) } )
   elseif type(texts) == "table" then
      local i,v
      for i,v in texts do
	 if type(v) == "string" then
	    tts[i] = tt_build(v,
			      {
				 box = ttbox0,
				 dl = dl_new_list(strlen(v),0,1)
			      } )
	 elseif tag(v) == tt_tag then
	    tts[i] = v
	 end
      end
   end

   -- Create main-dialog
   dial = gui_new_dialog(owner, box, nil, nil, label, mode, "gui_viewer")
   if not dial then return end

   -- Create view-dialog
   dial.viewer = gui_new_dialog(dial, tbox, nil, nil, nil, nil,
				  "viewer_area")
   if not dial.viewer then return end

   dial.tbox = ttbox0

   -- Create button
   local bbox = {( box[1] + box[3] - butw) * 0.5, (box[4]-buth-border) }
   bbox[3] = bbox[1] + butw
   bbox[4] = bbox[2] + buth
   dial.button = gui_new_button(dial, bbox, buttext , mode)
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
	 for i,v in dial.cur_tt.anchors do
	    anchor = anchor .. tostring(i) .. "{settext},"
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

   if nil then
      local header,footer

      header = '<dialog guiref="dialog"'
      header = header .. ' x="center"'
      header = header .. ' name="gui_viewer"'
      header = header .. ' hint_w="500"'
      header = header .. '>'

      footer = '<p><center><button guiref="close">close</button></dialog>'

      text = '<dialog guiref="text" x="center" name="gui_viewer_text">' ..
	 text .. '</dialog>'

      text = header..text..footer

      local tt = tt_build(text, {
			     x = "center",
			     y = "center",
			     box = { 0, 0, 640, 400 },
			  }
		       )
      tt_draw(tt)
      tt.guis.dialog.event_table[evt_shutdown_event] =
	 function(app)
	    app.answer = "shutdown"
	 end

      local i,v 
      for i,v in tt.guis.dialog.guis do
	 print (i,tostring(v))
      end

      tt.guis.dialog.guis["close"].event_table[gui_press_event] =
	 function(app, evt)
	    evt_shutdown_app(app.owner)
	    app.owner.answer = "close"
	    return evt
	 end

      tt.guis.dialog.guis["text"].event_table[gui_press_event] =
	 function(app, evt)
	    print("press")
	    return
	 end


      function gui_text_viewer_up_evt(app, evt)

	 print(app.owner.name, app.name, "up")
      end

      function gui_text_viewer_down_evt(app, evt)
	 print("down")
      end

      local i,v
      for i,v in gui_keyup do
	 tt.guis.dialog.guis["text"].event_table[i] = gui_text_viewer_up_evt
      end
      for i,v in gui_keydown do
	 tt.guis.dialog.guis["text"].event_table[i] = gui_text_viewer_down_evt
      end

      while not tt.guis.dialog.answer do
	 evt_peek()
      end

      print("answer="..tt.guis.dialog.answer)

      return tt.guis.dialog.answer
      
   end
end

--- Create a text-viewer application from a file.
--- @param  fname  Filename to view
--- @see gui_text_viewer
---
function gui_file_viewer(owner, fname, box, label, mode)
   if test("-f",fname) then
      local file = openfile(fname,"rt")
      if not file then return end
      local buffer = read(file,"*a")
      closefile(file)
      return gui_text_viewer(owner,buffer,box,label,mode)
   end
end

--
--- @}
--

-- <left> <img name="dcplaya" src="dcplaya.tga">
-- <center> <font size="18" color="#FFFF90"> Welcome to dcplaya<br>
-- <br>

local newbie_text = 
[[
<font size="14">
<center><font color="#9090FF">Bienvenue
<vspace h="4"><p><left><font color="#909090">
<hspace w="8">

If is this the first time you launch dcplaya, you should read this lines.
It contains a <button guiref="toto">TOTO</button>titi<br>Button
]]

-- Introduction
local introduction_text = 
[[
dcplaya est un programme pour ecouter de la musique avec votre DreamCast.
dcplaya permet de jouer la plupard des formats de musique populaire.
Chacun de ces formats fait l'objet d'un plugin, c'est a dire un programme
exterieur qui peut etre charger a tout moment. Cela permet d'ajouter de
nouveau format sans avoir a changer de version de dcplaya.
]]

-- Plugins
local plugins_text = 
[[
<center><font size="18" color="#FFFFFF">plugins<br>

<p><left><vspace h="10">
<font color="#909090" size="14">
Les plugins sont des programmes exterieur qui permettent d'ajouter des
fonctionnalites a dcplaya sans changer de version. Ils sont disponibles
sous forme de fichier qui portent generalement l'extension ".lef" ou ".lez".
Les fichiers ".lez" sont des fichiers ".lef" compresses en format gzip (.gz).

<p>Les plugins sont classes par categorie.<br>
<center>
<a href="#input">input</a> - 
<a href="#visual">visual</a> - 
<a href="#object">object</a> - 
<a href="#image">image</a> - 
<a href="#executable">executable</a>

<a name="input">
<p><center><font color="#FFFFFF" size="16">
input
</a>

<p><left>
<font color="#909090" size="14">
Les plugins input permettent d'ajouter de nouveau format de musiques
dans dcplaya. Un plugin cree un nouveau type de fichier musique, et
fournit et a la charge de charger et de decoder ces fichiers ainsi
que de fournir un certain nombre d'information sur la musique comme
par exemple l'artiste, le titre ou la duree.

<a name="visual">
<p><center><font color="#FFFFFF" size="16">
visual
<vspace h="8"></a>

<p><left><vspace h="6"><br>
<font color="#909090" size="14">
Les plugins visual permettent d'ajouter des effets visuels a dcplaya.

<vspace h="8"><br>
<a name="image">
<p><center><font color="#FFFFFF" size="16">
image
</a><br>


<font color="#909090" size="14">
Les plugins image sont utilises par dcplaya pour charger des images. Ces
images pourront ensuite etre utiliser de maniere transparente par dcplaya
pour charger des images en fond d'ecran ou pour comme texture pour les
fonctions graphiques. Les plugins images creent un nouveau type de fichier
image associe a des extensions (per exemple .jpg) et fournissent les
methodes pour charger une image ainsi qu'une methode pour recuperer des
infomations sur une image comme sa resolution ou le nombre de couleur.

<a name="object">
<p><center><font color="#FFFFFF" size="16">
object
<vspace h="8"></a>

<p><left><vspace h="6"><br>
<font color="#909090" size="14">
les plugins object permettent d'ajouter des objets 3D qui pourront
etre utiliser par la suite par d'autre plugin (notament les visual)

<a name="executable">
<p><center><font color="#FFFFFF" size="16">
executable
<vspace h="8"></a>

<p><left><vspace h="6"><br>
<font color="#909090" size="14">
Les plugins executables ont pour principale utilite d'ajouter des
fonctinnalites a l'interpreteur LUA de dcplaya. Mais ils peuvent servir
en fait a un peu pres n'importe quoi dans la mesure ou il s'agit d'un
programme. Ils peuvent par exemple etre utiliser pour patcher dcplaya.
]]

-- Historic


local welcome_text =
[[
<font size="14">
<center><font color="#90FF00">Greetings
<vspace h="4"><p><left><font color="#909090">
<hspace w="4">The dcplaya team wants to greet people that make this project
possible.
<vspace h="6"><p><center>
GNU [http://www.gnu.org]<br>
The Free Software Foundation<br>
KOS developpers. Particulary Dan Potter & Jordan DeLong of Cryptic Allusion<br>
andrewk for its dcload program.<br>
<left>
]]

local warning_text = 
[[
<font size="14">
<center><font color="#FF9000">Warning
<vspace h="4"><p><left><font color="#909090">
<hspace w="8">
This program is NOT official SEGA production. It is distributed in the
hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.<br>
<vspace h="4"><p>
<a name="clear">
<hspace w="8">
In other words, dcplaya developpers have worked hard to make it possible.
They have make the best to do a nice program and test it for you during hours.
This is a lot of work and they made it for FREE. They do not want to be implied
with any purchasse for anything that happen to you or to anything while using
this program. Problems may be submit to them but this is without any
warranty !
</a>
]]

-- [[

-- <left> <img name="dcplaya" src="dcplaya.tga">
-- <center> <font size="18" color="#FFFF90"> Welcome to dcplaya<br>
-- <br>
-- <font size="14">

-- <vspace h="8"><p>
-- <center><font color="#9090FF">Newbie
-- <vspace h="4"><p><left><font color="#909090">
-- If is this the first time you launch dcplaya, you should read this lines.
-- It contains a 

-- <vspace h="8"><p>
-- <center><font color="#9090FF">Presentation
-- <vspace h="4"><p><left><font color="#909090">
-- dcplaya is a music player for the dreamcast.<br> toto

-- <vspace h="8"><p>
-- <center><font color="#90FF00">Greetings
-- <vspace h="4"><p><left><font color="#909090">
-- The dcplaya team wants to greet people that make this project possible.<br>
-- o - GNU <http://www.gnu.org><br>
-- o - The Free Software Foundation<br>
-- o - KallistiOS (KOS) developpers. Particulary Dan Potter & Jordan DeLong of Cryptic Allusion<br>
-- o - andrewk for its dcload program.

-- <p><center>
-- ]]

tv = gui_text_viewer(nil,
		     {
			newbie  = newbie_text,
			welcome = welcome_text,
			warning = warning_text,
			plugins = plugins_text,
		     } , nil, "Welcome", nil)

return 1
