#! /bin/bash
#
# Convert .ase file to .c
#
# $Id: ase2c.sh,v 1.3 2003-05-04 12:20:03 benjihan Exp $
#

function debug
{
#	echo "** $*" >&2
    [ 1 ]
}

function error
{
    echo "!! $*" >&2
}

function get_int
{
    local search="$1" cnt line

    cnt=`grep -c "$search" "${asefile}"`
    if [ $cnt -ne 1 ]; then
	error "Could only convert single mesh file."
	exit 2
    fi
    debug "Count $search = $cnt"
    line=`grep "$search" "$asefile"`
    debug "line=${line}"
    expr "$line" : '.*\'"$search"' *\([0-9]*\).*'
}

#' Emacs rules !!!

function read_vertrices
{
    local tab
    
    echo "${static}vtx_t ${asename}_vtx[] ="
    echo "{"
    
    read -a tab
    while [ $? -eq  0 ]; do
	echo "  {${tab[2]}f, ${tab[3]}f, ${tab[4]}f},"
	read -a tab
    done
    echo "};"
}

function read_faces
{
    local tab

    echo "${static}tri_t ${asename}_tri[] ="
    echo "{"

    read -a tab
    while [ $? -eq  0 ]; do
	echo "  {${tab[3]}, ${tab[5]}, ${tab[7]}},"
	read -a tab
    done
# Add one more faces for invisible/visible state
    echo "  { -1, -1, -1, 0},"
    echo "};"
}

function create_c
{
    echo '#include "obj_driver.h"'
    echo
    grep "*MESH_VERTEX " "${asefile}" | read_vertrices
    echo
    grep "*MESH_FACE " "${asefile}" | read_faces
    echo

    if [ -x ./mklinks.tmp ]; then
	debug "2nd Pass"
	echo "${static}tlk_t ${asename}_tlk[] = "
	./mklinks.tmp || return 2
    else
	debug "1st Pass"
	echo "${static}tlk_t ${asename}_tlk[${numface}+1];"
    fi
    m4 -DASENAME="${asename}" -DNBV=${numvtx} -DNBF=${numface} \
	< ${scriptdir}/ase2c.m4
    return $?
}

static="static "

if [ $# -ne 1 ]; then
    echo ".ASE to .c convertor"
    echo "Copyright (C) Ben(jamin) Gerard <ben@sashipa.com>"
    echo "Usage: `basename "$0"` <ase-file>"
    exit 1
fi

scriptdir="`dirname "$0"`"
asefile="$1"
asename="`basename "${asefile}" .ase`"

numvtx=`get_int '*MESH_NUMVERTEX'`
debug "numvtx=${numvtx}"

numface=`get_int '*MESH_NUMFACES'`
debug "numface=${numface}" >&2

rm -f ./mklinks.tmp ./tmpobj.inc >&2

debug "Create tmpobj.inc"
create_c | sed -f ${SHACLEAN} > tmpobj.inc
if [ $? -eq 0 ]; then
    debug "Compile ' ${scriptdir}/mkobjlinks.c' to './mklinks.tmp'"
    if [ -z ${CC} ]; then
	error "You must define CC and CFLAGS" 
	exit 7;
    fi
    debug "$CC $CFLAGS"

    ${CC} ${CFLAGS} -DOBJECTNAME="${asename}" -o ./mklinks.tmp \
	${scriptdir}/mkobjlinks.c -lm || exit 5;
fi
if [ $? -eq 0 ]; then
    debug "Create ${asename}"
    create_c 
    debug "Finish ${asename}"
fi

ret=$?
rm -f mklinks.tmp tmpobj.inc >&2

exit $ret
