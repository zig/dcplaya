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

function linetype
{
	case "$1" in
		---*)
			if test "$2" = "struct"; then return 2; fi
			return 1
		;;
		function)
			return 3
		;;
	esac
	return 0
}

function struct
{
	shift
	case "$1" in
		"struct"|"};"*)
			echo "$*"
			;;
		"")
			;;
		'*'*)
			echo -n "   $1"
			shift
			case "$1" in
				-*)
					echo -n "$1" | tr '-' ' '
					echo -n "  -"
					shift
					;;
			esac
			echo "  $*"
			;;
		*)
			echo "  $*"
			;;
   esac
}

function func
{
	shift
	echo "$@;"
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
				if [ ${prevtype} -ne 2 ]; then
					echo "${l[*]}" | sed "s#---#///#"
				else
					struct "${l[@]}"
					type=2
				fi
				;;
			2) # STRUTURES
				struct "${l[@]}"
				;;
			3) # FUNCTIONS
				[ ${prevtype} -eq 1 ] && func "${l[@]}" || type=0
				;;
			0) # BLANK-LINE
				[ ${prevtype} -ne 0 ] && echo
				;;
		esac
		prevtype=${type}
		read -a l
	done
	return 0
}

comment='^[[:space:]]*---.*$'
empty='^[[:space:][:cntrl:]]*$'
function='^[[:space:]]*function[[:space:]]*\w*(.*).*$'

grep -E -e "${comment}|${empty}|${function}" | transform

