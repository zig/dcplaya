#! /bin/sh
#
# by Benjamin Gerard <ben@sashipa.com>
#
# This script construct a very simple symbol table in "C" format.
# It is used for exporting dcplaya main executable symbils.
#
# $Id: makesymb.sh,v 1.3 2002-11-28 04:22:44 ben Exp $
# 

function Error
{
	echo "$@" 1>&2
}

function Debug
{
	echo '** '"$@" 1>&2
}

function read_symb
{
	local tab

	echo "{"

	read -a tab
	while [ $? -eq  0 ]; do
		echo "  {0x${tab[0]}, '${tab[1]}', \"${tab[2]}\"},"
		read -a tab
	done
	echo "};"
}

## Start

# Verify argument 
if [ $# -ne 1 ]; then
	Error "`basename "$0"` : missing elf file";
	exit 1
fi

# Find nm	
if [ -z "$NM" ]; then
    NM="`which ${KOS_NM}`"
fi

if [ ! -x "$NM" ]; then
    Error "Could not find nm [$NM] executable"
    exit 3
fi

if [ -r "$1" ]; then
    "$NM" "$1" \
    | grep -xe '[0-9a-fA-F]\{8\} [^A] .*' \
    | sed "s/\([0-9a-fA-F]\{8\}\) \([^A]\) \(.*\)/  \{ (void *) 0x\1, '\2', \"\3\" \},/p" \
    | sort -u -k 6,6
else
    Debug "[$1] does not exist or is unreadable : create empty file."
fi


