--- @ingroup dcplaya_lua_gui
--- @file    plugin_info.lua
--- @date    2002/12/06
--- @author  benjamin gerard
---
--- @brief   hyper text viwer gui
--- $Id: plugin_info.lua,v 1.3 2003-03-25 09:26:46 ben Exp $
---

if not dolib("textviewer") then return end

--- @name plugin viewer
--- @ingroup dcplaya_lua_gui
--- @{
--

--- Create a plugin info tagged text.
--
--- @param  texts      table where text is stored (created if nil)
--- @param  plugin     either a plugin
---                        or a table of [table of ...] plugin
--- @param  text_name  prefix of index in texts table.
---
--- @return texts table
--- @internal
---
function plugin_viewer_build(texts, plugin, text_name)
   if type(plugin) == "table" then
      local i,v
      local prefix = (text_name and (text_name .. "/")) or ""
      for i,v in plugin do
	 local n
	 if type(i) == "string" then
	    n = prefix .. i
	 else
	    n = text_name
	 end
	 texts = plugin_viewer_build(texts, v, n)
      end
      return texts
   elseif tag(plugin) ~= driver_tag then
      return texts
   end

   local text, name, authors, version, desc

   name = plugin.name
   version = plugin.version
   authors = plugin.authors
   desc = plugin.description
   plugin = nil

   if type(text_name) ~= "string" then
      text_name = name
   else
      text_name = text_name .. "/" .. name
   end

   local chap = '<p><vspace h="14"><center><cfont>'
   local body = '<p><vspace h="6"><left><bfont>'

   text = '<macro macro-name="tfont" macro-cmd="font" color="#c0c0f0" size="24">'
      .. '<macro macro-name="cfont" macro-cmd="font" color="#F0E080" size="18">'
      .. '<macro macro-name="bfont" macro-cmd="font" color="#D0D0A0" size="16">'
      .. '<center><tfont>' .. name .. "<br><bfont>"
      .. format("%d.%02d",intop(version,'>',8),intop(version,'&',255))
      .. chap .. 'Authors'
      .. body .. authors
      .. chap .. 'Description'
      .. body ..  desc

   print("Inserting text ["..text_name.."]")

   if type(texts) ~= "table" then texts = {} end
   texts[text_name] = text
   return texts

end


function plugin_viewer_make_list(plugin_name)
   local plugin_list
   local plugins = get_driver_lists()
   if type(plugins) ~= "table" then return end
   local s, e, ptype, pname = strfind(plugin_name,"(.*)/(.*)")
   if type(ptype) ~= "string" or type(pname) ~= "string" then
      print("bad name ",ptype,pname)

      return
   end

   printf("type:[%s] name:[%s]\n",ptype,pname)

   plugin_list = {}

   local i,v,j,n
   n = 0
   for i,v in plugins do
      if ptype == "*" or ptype == i and v.n > 0 then
	 plugin_list[i] = {}
	 for j=1,v.n do
	    if pname == "*" or v[j].name == pname then
	       tinsert(plugin_list[i],v[j])
	       n = n + 1
	    end
	 end
      end
   end

   return n > 0 and plugin_list
end

--- Create a plugin info application.
--
--- @param  owner   owner applciation (default is desktop)
--- @param  plugin  either a plugin
---                     or a table of [table of ...] plugin
---                     or a string (type/name).
---                 type and name could be replaced by '*' for wildcard search
--- @param  dialog label (optionnal)
--- @return dialog application
---
function plugin_viewer(owner, plugin, box, label, mode)
   local z, dial, plugin_list, name

   if not driverlist_tag or not driver_tag then return end

   if type(plugin) == "string" then
      name = plugin
      plugin_list = plugin_viewer_make_list(plugin)
   elseif type(plugin) == "table" then
      name = "plugins"
      plugin_list = plugin
   elseif tag(plugin) == driver_tag then
      name = plugin.name
      plugin_list = plugin
   else
      return
   end

   local texts = plugin_viewer_build(texts, plugin_list, nil)
   local app
   if texts then
      app = gui_text_viewer(owner, texts, box, label, mode)
   end
   if app then
      app.name = name .. " info"
   end
   return app
end

--
--- @}
----

-- Create application icon sprite
--sprite_simple(nil,"textviewer.tga")

return 1
