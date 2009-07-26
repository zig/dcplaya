#! /bin/bash
# 
# (C) 2002 benjamin gerard <ben@sashipa.com>
#

line=""

function transform
{
    name="`expr "$1" : '\(.*\)=.*' \| "$1"`"
    val="`expr "$1" : '.*=\(.*\)' \| ""`"
##    echo "name:[$name]" >&2
##    echo "val:[$val]" >&2
    echo "s¬\\\$${name}\\\$¬${val}¬"
}


function tam
{
    i=1
    echo "NUMBER: $#" >&2
    while [ $# -ne 0 ] ; do
		echo $i "[$1]" [$2] >&2
		shift 2
		i=`expr $i + 1`
    done
}

i=0;
while [ $# -ne 0 ]; do
    case "$1" in
	-D) line[$i]="-e";;
	*) line[$i]="`transform "$1"`";;
    esac
    shift
    i=`expr $i + 1`
done

#echo -e "[${line}]" >&2
#tam "${line[@]}"

exec sed  "${line[@]}"
