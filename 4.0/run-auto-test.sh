#!/bin/bash

export MG_GAL_ENGINE=dummy
export MG_IAL_ENGINE=dummy

start_epoch=$(date +%s)

./sliceallocator
if test ! $? -eq 0; then
    echo "sliceallocator not passed"
    exit 1
fi

./ustrgetbreaks
if test ! $? -eq 0; then
    echo "ustrgetbreaks not passed"
    exit 1
fi

./biditest
if test ! $? -eq 0; then
    echo "biditest not passed"
    exit 1
fi

./bidicharactertest
if test ! $? -eq 0; then
    echo "bidicharactertest not passed"
    exit 1
fi

./createlogfontex 3600
if test ! $? -eq 0; then
    echo "createlogfontex 3600 not passed"
    exit 1
fi

./drawglyphstringex 3600
if test ! $? -eq 0; then
    echo "drawglyphstringex 3600 not passed"
    exit 1
fi

./createtextruns 1
if test ! $? -eq 0; then
    echo "createtextruns 1 not passed"
    exit 1
fi

./createtextruns 2
if test ! $? -eq 0; then
    echo "createtextruns 2 not passed"
    exit 1
fi

./createlayout
if test ! $? -eq 0; then
    echo "createlayout not passed"
    exit 1
fi

./createlayout 1
if test ! $? -eq 0; then
    echo "createlayout 1 not passed"
    exit 1
fi

./createlayout 2
if test ! $? -eq 0; then
    echo "createlayout 2 not passed"
    exit 1
fi

./createlayout 3
if test ! $? -eq 0; then
    echo "createlayout 3 not passed"
    exit 1
fi

./basicshapingengine 3600
if test ! $? -eq 0; then
    echo "basicshapingengine 3600 not passed"
    exit 1
fi

./complexshapingengine 3600
if test ! $? -eq 0; then
    echo "complexshapingengine 3600 not passed"
    exit 1
fi

end_epoch=$(date +%s)
time=$(($end_epoch - $start_epoch))

echo "ALL TEST CASES PASSED ($time S)!"
exit 0
