#! /bin/sh
#
# This script clean dependencies added by makedepend.
# In other words, it deletes all line from the [# DO NOT DELETE] one
#
# (C) 2002 ben(jamin) gerard <ben@sashipa.com> 
#
# $Id: cleandep.sh,v 1.1 2002-08-26 15:21:55 ben Exp $
#

read line
while [ $? -eq 0 ]; do
    a=`expr "$line" : '# DO NOT DELETE.*'` && exit 0
    echo "$line"
    read line
done
test 0 -eq 0
