#! /bin/sh
# 
# (C) 2002 benjamin gerard <ben@sashipa.com>
#
# Convert lua file to fake C file for doxygen documentation.
#
# Very simple but still working if this rules are respected :
#
# - All lines which 1st word is `---' is considered as a documentaion comment.
#   In this lines all `---' sequence are transformed to '///'. The
#   consequence of is that '---<' are transformed to '///<' which is used to
#   document the left side of the line in structure documentation. Original
#   "C" comments could be use too.
#   e.g.:
#
#   --- toto; ---< Documents toto
#   --- titi; ///< Documents toto
#   --- tata; /**< Documents tata */

# - Do NOT put comment right to a documented function :
#   e.g ``function test(a,b) -- a test function'' will NOT work correctly.
#
# - Structures documentation looks like that:
#
#   --- struct name_of_struct {
#   ---   field1; ---< Documentation of field1
#   ---   /** Documentation of field2 */
#   ---   field2;
#     ... more fields ...
#   --- };
#
# - 

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
		"struct"|"};*")
			echo "$*" | sed "y#---#///#"
			;;
		"")
			;;
		*)
			echo "  $*" | sed "y#---#///#"
			;;
   esac
}

function func
{
	shift
	echo "$@;" | sed "y#---#///#"
}

function transform
{
	local s w
	local l e type prevtype
	prevtype=0
	s="[[:space:]]*"
	w="[_[:alnum:]]*"
	read -a l
	while [ $? -eq 0 ]; do
		linetype ${l[@]}
		type=$?
#		echo "type:$type"
		case ${type} in
			1) #COMMENT
				if [ ${prevtype} -ne 2 ]; then
					echo "${l[*]}" | sed "y#---#///#"
				else
					struct "${l[@]}"
					type=2
				fi
			;;
			2) struct "${l[@]}"
			;;
			3) [ ${prevtype} -eq 1 ] && func "${l[@]}" || type=0
			;;
			0) [ ${prevtype} -ne 0 ] && echo
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

