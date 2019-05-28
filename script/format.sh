#!/bin/sh

clang-format -i -style=file `find ../playing ../playing_tests -type f \( -name *.h -o -name *.cpp -o -name *.hpp -o -name *.m -o -name *.mm \)`
