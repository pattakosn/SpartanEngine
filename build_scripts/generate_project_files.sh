#!/bin/bash
# Copyright(c) 2016-2025 Panos Karabelas
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is furnished
# to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

set -e

BINARY_DIR="binaries"
THIRD_PARTY_DIR="third_party"
LIB_URL="https://www.dropbox.com/scl/fi/mxox9g0j1rb3re0arpu0x/libraries.7z?rlkey=nhfpr02mhuxwf151p9ka495fc&st=19h4l57p&dl=1"
EXPECTED_HASH="ec483b7875ab97afe7d70b95a2ed5ecdeae78e275cefabd9305b4d0b2428a389"
ASSETS=("data" "project/models" "project/music" "project/terrain" "project/materials")
LIBS=("dxcompiler.dll")
IS_CI=false

copy() {
    src=$1
    dest=$2
    echo "Copying $src to $dest"
    mkdir -p "$(dirname "$dest")"
    cp -r "$src" "$dest"
}

download_file() {
    url=$1
    dest=$2
    echo "Downloading $url to $dest"
    curl -L --output "$dest" "$url"
}

extract_archive() {
    archive=$1
    dest=$2
    echo "Extracting $archive to $dest"
    7z x "$archive" -o"$dest"
}

generate_project_files() {
    action=$1
    platform=$2
    is_windows=false

    if [[ $action == vs* ]]; then
        is_windows=true
    fi

    premake_exe="premake5"
    if $is_windows; then
        premake_exe="premake5.exe"
    fi

    premake_lua="build_scripts/premake.lua"
    cmd="./$premake_exe --file=\"$premake_lua\" \"$action\" \"$platform\""

    echo "Running command: $cmd"
    eval $cmd
}

main() {
    if [[ " ${@} " == *" ci "* ]]; then
        IS_CI=true
    fi

    echo "1. Create binaries folder with the required data files..."
    copy "data" "$BINARY_DIR/data"
    copy "build_scripts/download_assets.py" "$BINARY_DIR"
    copy "build_scripts/file_utilities.py" "$BINARY_DIR"
    copy "build_scripts/7z.exe" "$BINARY_DIR"
    copy "build_scripts/7z.dll" "$BINARY_DIR"

    echo "2. Download and extract libraries..."
    library_destination="$THIRD_PARTY_DIR/libraries/libraries.7z"
    download_file "$LIB_URL" "$library_destination"
    extract_archive "$library_destination" "$THIRD_PARTY_DIR/libraries"

    echo "3. Copying required DLLs to the binary directory..."
    for lib in "${LIBS[@]}"; do
        copy "$THIRD_PARTY_DIR/libraries/$lib" "$BINARY_DIR"
    done

    echo "4. Generate project files..."
    generate_project_files "$1" "$2"

    if ! $IS_CI; then
        read -p "Press any key to continue..."
    fi

    exit 0
}

main "$@"
