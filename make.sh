#!/usr/bin/env bash
#
# Copyright (C) 2025 ぼっち <ayumi.aiko@outlook.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# shutt up
CC_ROOT="/home/ayumi/android-ndk-r27d/toolchains/llvm/prebuilt/linux-x86_64/bin"
STRIP="${CC_ROOT}/llvm-strip"
CFLAGS="-std=c23 -O3 -static -Wall -Wextra -Werror -pedantic \
        -Wshadow -Wconversion -Wsign-conversion -Wpointer-arith -Wcast-qual \
        -Wmissing-prototypes -Wstrict-prototypes -Wformat=2 -Wundef"
BUILD_LOGFILE="./hoshiko-cli/build/log"
OUTPUT_DIR="./hoshiko-cli/build"
HOSHIKO_HEADERS="./hoshiko-cli/src/include"
HOSHIKO_SOURCES="./hoshiko-cli/src/include/daemon.c"
TARGETS=("hoshiko-cli/src/yuki/main.c" "hoshiko-cli/src/alya/main.c")
OUTPUT_BINARY_NAMES=("hoshiko-yuki" "hoshiko-alya")
SDK=""
CC=""
TARGET_SELECTED=""

# first of all, let's just switch to the directory of this script temporarily.
cd "$(realpath "$(dirname "$0")")" || {
    printf "\033[0;31mmake: Error: Could not enter script directory.\033[0m\n"
    exit 1
}

# uhrm ig
[ ! -d "${CC_ROOT}" ] && CC_ROOT="/workspaces/android-ndk-r27d/toolchains/llvm/prebuilt/linux-x86_64/bin"

# print the banner:
printf "\033[0;31mM\"\"MMMMM\"\"MM                   dP       oo dP\n"
printf "M  MMMMM  MM                   88          88\n"
printf "M         \`M .d8888b. .d8888b. 88d888b. dP 88  .dP  .d8888b.\n"
printf "M  MMMMM  MM 88'  \`88 Y8ooooo. 88'  \`88 88 88888\"   88'  \`88\n"
printf "M  MMMMM  MM 88.  .88       88 88    88 88 88  \`8b. 88.  .88\n"
printf "M  MMMMM  MM \`88888P' \`88888P' dP    dP dP dP   \`YP \`88888P'\n"
printf "MMMMMMMMMMMM                                                 \n"

# jst mkdir
mkdir -p "$(dirname "$BUILD_LOGFILE")" "$OUTPUT_DIR"
for arg in "$@"; do
    lower=$(echo "$arg" | tr '[:upper:]' '[:lower:]')
    case "$lower" in
        clean)
            rm -f "$BUILD_LOGFILE" "$OUTPUT_DIR"/hoshiko-*
            echo -e "\033[0;32mmake: Info: Clean complete.\033[0m"
            exit 0
        ;;
        sdk=*)
            SDK="${lower#sdk=}"
        ;;
        arch=arm)
            [[ -n "$SDK" ]] && CC="${CC_ROOT}/armv7a-linux-androideabi${SDK}-clang"
        ;;
        arch=arm64)
            [[ -n "$SDK" ]] && CC="${CC_ROOT}/aarch64-linux-android${SDK}-clang"
        ;;
        arch=x86)
            [[ -n "$SDK" ]] && CC="${CC_ROOT}/i686-linux-android${SDK}-clang"
        ;;
        arch=x86_64)
            [[ -n "$SDK" ]] && CC="${CC_ROOT}/x86_64-linux-android${SDK}-clang"
        ;;
        yuki)
            TARGET_SELECTED="single:0"
        ;;
        alya)
            TARGET_SELECTED="single:1"
        ;;
        all)
            TARGET_SELECTED="all"
        ;;
    esac
done

# functions:
function buildTarget() {
    idx=$1
    src="${TARGETS[$idx]}"
    out="${OUTPUT_BINARY_NAMES[$idx]}"
    echo -e "\e[0;35mmake: Info: Building $(echo "${OUTPUT_BINARY_NAMES[$idx]}" | cut -c 9-12)...\e[0;37m"
    if ! "$CC" $CFLAGS "$HOSHIKO_SOURCES" -I"$HOSHIKO_HEADERS" "$src" -o "$OUTPUT_DIR/$out" &> "$BUILD_LOGFILE"; then
        printf "\033[0;31mmake: Error: Build failed (%s). Check %s\033[0m\n" "$out" "$BUILD_LOGFILE"
        exit 1
    fi
    "$STRIP" -s "$OUTPUT_DIR/$out"
    echo -e "\e[0;36mmake: Info: Build finished without errors, be sure to check logs if concerned. Thank you!\e[0;37m"
}
# functions:

# help:
if [[ -z "$SDK" || -z "$CC" || -z "$TARGET_SELECTED" ]]; then
    echo -e "\033[1;36mUsage:\033[0m make.sh [SDK=<level>] [ARCH=<arch>] <target>"
    echo ""
    echo -e "\033[1;36mTargets:\033[0m"
    echo -e "  \033[0;32myuki\033[0m     Build the Hoshiko daemon binary"
    echo -e "  \033[0;32malya\033[0m     Build the Hoshiko daemon manager"
    echo -e "  \033[0;32mall\033[0m      Build all binaries"
    echo -e "  \033[0;32mclean\033[0m      Remove build artifacts"
    exit 1
fi

# target selector.
case "$TARGET_SELECTED" in
    all)
        buildTarget 0
        buildTarget 1
    ;;
    single:*)
        buildTarget "${TARGET_SELECTED#single:}"
    ;;
esac