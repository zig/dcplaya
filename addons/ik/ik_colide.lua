--- @ingroup dcplaya_addons_ik
--- @file    ik_colide.lua
--- @date    2002/12/24
--- @author  benjamin gerard <ben@sashipa.com>
--- @brief   Internationnal Karate Remix (colision)
--

function ik_box_intersect(a,b)
   local xa1,ya1 = a[1],a[2]
   local xa2,ya2 = xa1+a[3], ya1+a[4]
   local xb1,yb1 = b[1],b[2]
   local xb2,yb2 = xb1+b[3], yb1+b[4]

   if xa1 >= xb2 or xa2 <= xb1 or ya1 >= yb2 or ya2 <= yb1 then
      return nil
   end

   local r = {
      max(xa1,xb1), max(ya1,yb1), min(xa2,xb2), min(ya2,yb2),
      (a[5] or 1) * (b[5] or 1)
   }
--    dump(a,"A")
--    dump(b,"B")
--    dump(r,"R")
   return r
end

function ik_colides(attack, targets)
   if not attack or not targets then return end
   local i,v
   local r = {}
   for i,v in targets do
      local box = ik_box_intersect(attack,v)
      if box then tinsert(r, box) end
   end
   
   if getn(r) > 0 then
--       print (getn(r))
      r.n = nil
      return r
   end
end

return 1
