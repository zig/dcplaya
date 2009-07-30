#! /bin/bash
#
# This script clean dependencies added by makedepend.
# In other words, it deletes all line from the [# DO NOT DELETE] one
#
# (C) 2002 ben(jamin) gerard <ben@sashipa.com> 
#
# $Id: cleandep.sh,v 1.2 2002-08-26 20:12:22 ben Exp $
#

echo "This script does not currently works !!!" >&2
exit 1

read line
while [ $? -eq 0 ]; do
    a=`expr "$line" : '# DO NOT DELETE.*'` && exit 0
    echo "$line"
    read line
done
test 0 -eq 0
