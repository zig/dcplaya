#! /bin/sh
# 
# (C) 2002 benjamin gerard <ben@sashipa.com>
#
# Convert lua file to fake C file for doxygen documentation.
#
# Very limited but still working if these rules are respected :
#
# - All lines which 1st word is ``---'' is considered as a documentation
#   comment. It will be transformed to ``///''. The rest of the line is copied
#   ``as is''. Standard comments should be used inside lua comments.
#
#   e.g.:
#
#   --- titi; ///< Documents toto
#   --- tata; /**< Documents tata */

# - Do NOT put comment right to a documented function :
#   e.g ``function test(a,b) -- a test function'' will NOT work correctly.
#
# - Structures documentation looks like that:
#
#   --- This is an example of structure documentation.
#   --- struct name_of_struct {
#   ---   field1; ---< Documentation of field1
#   ---   /** Documentation of field2 */
#   ---   field2;
#     ... more fields ...
#   --- };
#

function indent
{
    openstruct=`expr ${openstruct} + 2`
    istr="`expr substr '                       ' 1 ${openstruct}`"
}

function unindent
{
    [ ${openstruct} -ge 2 ] && openstruct=`expr ${openstruct} - 2`
    istr="`expr substr '                       ' 1 ${openstruct}`"
}

function linetype
{
    case "$1" in
	---*)
	    case "$2" in
		struct|class|enum|union)
		    # Opening case 2 (struct|class|enum|union..)
		    return 2
		    ;;
		'};')
		    # Closing case 2 (struct|class|enum|union..)
		    return -2
		    ;;
	    esac
	    return 1
	    ;;
	function)
	    return 3
	    ;;
	--:) # Line starting by are interpreted as is (for variable doc)
	    return 4
    esac
    # Unknown or blank
    return 0
}

function struct
{
    shift
    echo "${istr}$*"
}

function func
{
    shift
    echo "${istr}$*;"
}

function uncomment
{
    shift
    echo "${istr}$*"
}

function strcomment
{
    echo -e "${cb}\n" | sed "s#---##"
}

# If a comment block exist then
#  0 : open comment block.
#  1 : close comment block
function comment
{
    if [ ! -z "${cb}" ]; then
	if [ $1 -ne 0 -a ${openstruct} -ne 0 ]; then
	    echo -e "${cb}\n" | sed "s#[ /]\*[ *]##"
	else
	    echo -e "${cb}\n${istr} */"
	fi
	cb=""
    fi
}

function addcomment
{
    shift
    if [ -z "${cb}" ]; then
	cb="\n${istr}/** ${*}"
    else
	cb="${cb}\n${istr}  * ${*}"
    fi
}

function transform
{
    local s w
    local l e type prevtype

    prevtype=0
    read -a l
    while [ $? -eq 0 ]; do
	linetype ${l[@]}
	type=$?
	case ${type} in
	    1) # COMMENTS
		addcomment "${l[@]}"
		;;
	    2) # STRUCTURES
		comment 1
		comment 0
		struct "${l[@]}"
		indent
		;;
	    -2) # END OF STRUCTURES
		comment 1
		unindent
		struct "${l[@]}"
		;;
	    [34]) # FUNCTIONS or # UNCOMMENT LINE
		if [ ! -z "${cb}" ]; then
		    comment 0
		    if [ ${type} -eq 3 ]; then
			func "${l[@]}"
		    else
			uncomment "${l[@]}"
		    fi
		else
		    type=0
		fi
		;;
	    0) # BLANK
		comment 1
	esac
	prevtype=${type}
	read -a l
    done
    return 0
}

cb=""
istr=""
openstruct=0
comment='^[[:space:]]*--[-:].*$'
empty='^[[:space:][:cntrl:]]*$'
function='^[[:space:]]*function[[:space:]]*\w*(.*).*$'

grep -E -e "${comment}|${empty}|${function}" | transform

