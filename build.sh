#!/bin/bash

export OS=`uname -s`

if [ "$OS" == "Darwin" -o "$OS" == "Linux" ]; then
    
    echo ""
    echo "-------------------------------------------------------------------------------------------------"
    echo "    Compiling for XP11 for OS $OS"
    echo "-------------------------------------------------------------------------------------------------"
    echo ""
    make clean
    make
fi

if [ "$OS" == "Linux" -o "$OS" == "Windows" ]; then

    export MINGW=${MINGW:-"1"}

    if [ "$MINGW" == "1" ]; then

        echo ""
        echo "-------------------------------------------------------------------------------------------------"
        echo "    Compiling for XP11 for OS Windows using MingW-64"
        echo "-------------------------------------------------------------------------------------------------"
        echo ""
        make -f Makefile.win clean
        make -f Makefile.win
    fi

fi

cp rextconfig.txt ./rext
cp LICENSE.txt ./rext

echo ""
echo "-------------------------------------------------------------------------------------------------"
echo "    Done"
echo "-------------------------------------------------------------------------------------------------"
echo ""
