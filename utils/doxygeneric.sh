#! /bin/sh
# 
# (C) 2003 benjamin gerard <ben@sashipa.com>
#
# Generate a generic doxygen configuration file
#

function transform
{
    name="`expr "$1" : '\(.*\)=.*' \| "$1"`"
    val="`expr "$1" : '.*=\(.*\)' \| ""`"
    echo "s¬${name}\([[:space:]]*\)=.*¬${name}\1= ${val}¬"
}

export line=

file="$1"
shift
i=0;
while [ $# -ne 0 ]; do
    case "$1" in
	-D)
	    line[$i]="-e"
	    ;;
	*)
	    line[$i]="`transform "$1"`"
	    ;;
    esac
    shift
    i=`expr $i + 1`
done
sed  "${line[@]}" > "${file}"
if [ $? -ne 0 ]; then
    rm -f "${file}"
    exit 255
fi

