# lua2dox version 2 : sed strikes back.
#
# by benjamin gerard
#
# Fill free to use, modify this script.
#
# Please send any enhancement / debugging / comments to <ben@sashipa.com>
#
# Note : I am sure it can be done more efficiently but it is my very
#        first sed program :P. Anyway it is about 2-3 times faster
#        than my older sh script.
#
# $Id: lua2dox.sed,v 1.1 2003-03-25 09:23:18 ben Exp $
#

#comment='^[[:space:]]*--[-:].*$'
#empty='^[[:space:][:cntrl:]]*$'
#function='^[[:space:]]*function[[:space:]]*\w*(.*).*$'

#grep -E -e "${comment}|${empty}|${function}" | transform

/^[[:space:]]*--[-:].*\|^[[:space:][:cntrl:]]*$/p

# ----------------------------------------------------------------------
# Print out `:-- ' lines. This one are prior to any other
# transformation, except trim spaces
# ----------------------------------------------------------------------


# Match line with only `:--' and replace by blank line.
/^--:$/{
i\

d
}

# Match line brigining by `:-- ' and removes header and trailing space.
/^--: .*$/{
s/^--: \(.*\)$/\1/
s/^\(.*[^[:space:]]\)\([[:space:]][[:space:]]*\)$/\1/
p
d
}


# ----------------------------------------------------------------------
# Print out @code @endcode section unchanged excepted 
# that its could be commented if not. 
# ----------------------------------------------------------------------
/^[[:space:]]*---[[:space:]]*@code[[:space:]]*$/,/^[[:space:]]*---[[:space:]]*@endcode[[:space:]]*$/{
# Remove existing `---'
s/^[[:space:]]*---\([[:space:]]*.*\)$/\1/
# Comment all lines and trim space :)
s¬^\(.*[^[:space:]]\)[[:space:]]*$¬///\1¬
s¬^[[:space:]]*$¬///¬
p
d
}
# ----------------------------------------------------------------------


# Remove standard comment. We must do that here (before function) because
# function "declaration" could have comment such as :
# function test ( a, -- comment a.
#                 b, -- comment b.
#                 c) -- comment c
s/^\([^-]*\)\(--[^-:].*\)$/\1/

# Trim trailing spaces
s/^\(.*[^[:space:]]\)\([[:space:]][[:space:]]*\)$/\1/
 
# ----------------------------------------------------------------------
# First get one function definition.
# ----------------------------------------------------------------------
/^[[:space:]]*function[[:space:]]*[[:alnum:]_]*[[:space:]]*(.*)/{
s/^[[:space:]]*function[[:space:]]*\([[:alnum:]_]*\)[[:space:]]*\((.*)\).*$/\1\2;/
p
d
}

# ----------------------------------------------------------------------
# Print out function block.
# ----------------------------------------------------------------------
/^[[:space:]]*function[[:space:]]*[[:alnum:]_]*[[:space:]]*(/,/)/{

# We are in function address space (I hope so)
# Removes `function' keyword en space around.
# !!! This will replace `function' in all case !!!
s/[[:space:]]*function[[:space:]]*//

# For lines in comment address space `--', removes comment
/--/s/^\([^-]*\).*$/\1/

# Removes all spaces
s/[[:space:]]//g
# Indent line without `('.
/(/!s/\(.*\)/	\1/

# Add ';' after )
/)/s/^\(.*)\).*/\1;/
# Finally print line not composed uniquely of space
/^[[:space:]]*$/!p
d
}
# ----------------------------------------------------------------------

# Trim starting spaces
s/^[[:space:]]*\(.*\)$/\1/

# Print blank line.
/^$/p

# Print direct block
/^--[-:][[:space:]].*$/{
s¬^\(--[-:]\)\([[:space:]].*\)$¬///\2¬
p
d
}


#s/^[[:space:]]*function[[:space:]]*\([[:alnum:]_]\)*[[:space:]]*(.*/y/f/F/

# Replace all --- by /// at the begining
#/^---/s#---#///#g
#t skip

#: skip
#l
#p
