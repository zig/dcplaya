# txt2zml version 1
#
# by benjamin gerard
#
# Fill free to use, modify this script.
#
# Convert a html converted to text by lynx into  zml !!!
#
# Crappy, isn't it ?
#
# $Id: txt2zml.sed,v 1.2 2003-03-31 16:49:12 ben Exp $
#

# some regexpr
# FNAME := [[:alpha:]_][[:alnum:]_]*
#
#

# filter reference
s|\[[[:digit:]]\{1,\}\]||g

# Quit after references
/^References$/q
# hack !
/doxygen/{
p
q
}

# entering new chapter / section ...
/^$/b skipblank

# filter body
b filter

# Complete function loop
: funcloop
s|^[[:blank:]]\{1,\}\([[:alpha:]_][[:alnum:]_]*\)[[:blank:]]*(\(.*\)|<fct><fct-color> \1 (\2|
: funcloop2
p
n
/)/!b funcloop2
i\
</fct2>
p
b funcreturn

# Complete list loop
: liloop
s|^[[:blank:]]\{1,\}\*[[:blank:]]\{1,\}\(.*\)|<br><li>\1|
p
n
/^$/b liend
/^[[:blank:]]\{1,\}\*[[:blank:]]\{1,\}.*/i\
</li>
b liloop

: liend
i\
</li><p>

: skipblank
i\
<ul>
: loopblank
n
s|\[[[:digit:]]\{1,\}\]||g
/^$/b loopblank
# uncomplete function
/\([[:alpha:]_]\{1,\}\)[[:blank:]]*(.*/b filter

s|^[[:alpha:]]\{1,\}.*|<chap><chap-color>\0</chap><p>|
s|^[[:blank:]]\{1,\}\([[:alpha:]]\{1,\}.*\)|<sect><sect-color>\1</sect><p>|

: filter
# complete function line
s|^[[:blank:]]\{1,\}\([[:alpha:]_][[:alnum:]_]*\)[[:blank:]]*(\(.*\))|<fct><fct-color> \1 (\2)</fct>|
t end
# uncomplete function line
/^[[:blank:]]\{1,\}\([[:alpha:]_][[:alnum:]_]*\)[[:blank:]]*(.*/b funcloop
t funcloop
: funcreturn
# entering list
/^[[:blank:]]\{1,\}\*[[:blank:]]\{1,\}\(.*\)/b liloop

b end
: end

: print
p
