#! /bin/sh

infile="$1"

# Create exceptions file
grep -e "^Unhandled exception:.*" -A 3 "${infile}" > exceptions.log
grep -e "^SYM:.*" ${infile} | sort -k 3 > symbols.log
grep -e "^SEC:.*" ${infile} | sort -k 3 > sections.log

grep -e  "^\*\* lef_load : Local \[.*\] = \[.* + .*\] \[.*\]" ${infile} | cut -d ' ' -f 5,10 | sort -

#grep -e  "^\*\*.*$" ${infile}
