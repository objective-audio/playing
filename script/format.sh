#!/bin/sh

export PATH=$PATH:/opt/homebrew/bin
clang-format -i -style=file `find ../playing ../playing_tests ../playing_ios_sample/playing_ios_sample ../playing_mac_sample/playing_mac_sample ../playing_sample -type f \( -name *.h -o -name *.cpp -o -name *.hpp -o -name *.m -o -name *.mm \)`
