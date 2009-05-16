#!/bin/sh

# Git Revision Extractor for Unix systems
# Written by Burlex, 2008/02/20
# Modified by Valroft for git

echo "// This file was automatically generated by the git revision 
extractor." > git_revision.h
echo "// There is no need to modify it." >> git_revision.h
echo "" >> git_revision.h
echo "#ifndef GIT_REVISION_H" >> git_revision.h
echo "#define GIT_REVISION_H" >> git_revision.h
echo "" >> git_revision.h
echo "#define BUILD_REVISION \""`git log -n 1 | grep commit | cut -d' ' -f2`"\"" >> git_revision.h
echo "#define BUILD_BRANCH \""`git status | grep 'On branch' | cut -d' ' -f4`"\"" >> git_revision.h
echo "#define BUILD_DATE __DATE__" >> git_revision.h
echo "#define BUILD_TIME __TIME__" >> git_revision.h
echo "#define BUILD_HOST \""`hostname`"\"" >> git_revision.h
echo "#define BUILD_USER \""`whoami`"\"" >> git_revision.h
echo "" >> git_revision.h
RANDOM=`date '+%H%S'`
echo "#define BUILD_RAND " $RANDOM >> git_revision.h
echo "#endif         // GIT_REVISION_H" >> git_revision.h
echo "" >> git_revision.h

mv git_revision.h src/hearthstone-shared

echo "1"
