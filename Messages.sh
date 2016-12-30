#! /usr/bin/env bash
$EXTRACTRC `find . -name "*.ui"` >> rc.cpp || exit 11
$XGETTEXT `find . -name "*.cpp" -o -name "*.qml"` -o $podir/plymouth_kcm.pot
rm -f rc.cpp
