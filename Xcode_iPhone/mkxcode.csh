#!/bin/csh

###
## This script creates "Xcode.tar.gz" in the parent directory
###

# remove build products
rm -rf build
rm -rf showimage/build

# remove Finder info files
find . -name ".DS_Store" -exec rm "{}" ";"

# remove user project prefs
find . -name "*.perspective*" -exec rm "{}" ";"
find . -name "*.pbxuser*" -exec rm "{}" ";"
find . -name "*.mode*" -exec rm "{}" ";"

# create the archive
(cd .. && gnutar -zcvf Xcode_iPhone.tar.gz Xcode_iPhone)



