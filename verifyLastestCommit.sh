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

# check if we are running in the currect root branch and exit if not.
[ "$(git branch | awk '{print $2}')" != "main" ] && exit 1;

# setup android-ndk crap:
#   wget https:
sudo unzip -o android-ndk-r27.zip;

# exit if ndk is not present.
[ ! -d "/workspaces/android-ndk-r27d/toolchains/llvm/prebuilt/linux-x86_64/bin" ] && exit 1;

# the most recent commit hash:
thisBranchLatestCommit="$(git log | head -n 1 | awk '{print $2}')"

# so let's just try building and building for no reason other than just to revert the new "experi-mental" changes.
chmod +x ./make.sh 
if ! ./make.sh SDK=30 ARCH=arm64 all &>/dev/null; then
    echo "- Seems like the build is failed for some reason, let's just revert the changes in this branch...";
    for reverts in $(seq 1 5); do
        echo "  Reverting changes ($reverts/5)...";
        git reset --mixed HEAD~1;
        git restore "./hoshiko/hoshiko-cli/src/include/daemon.c" "./hoshiko/hoshiko-cli/src/include/daemon.h" \
            "./hoshiko/hoshiko-cli/src/yuki/main.c" "./hoshiko/hoshiko-cli/src/alya/main.c";
        if ./make.sh SDK=30 ARCH=arm64 all &>/dev/null; then
            thisBranchPreviousCommit="$(git log | head -n 1 | awk '{print $2}')"
            echo "- Reverted to a working state, force pushing stuff to the mains...";
            ./make.sh clean &>/dev/null;
            git push --force;
            git add "./hoshiko/hoshiko-cli/src/include/daemon.c" "./hoshiko/hoshiko-cli/src/include/daemon.h" \
                "./hoshiko/hoshiko-cli/src/yuki/main.c" "./hoshiko/hoshiko-cli/src/alya/main.c";
            git commit -m "github-actions: hoshiko-syn: Revert commit changes from $thisBranchLatestCommit to $thisBranchPreviousCommit to get this branch to a least-working state.";
            exit 0;
        else
            echo "  hmph~ doing another revert...";
            ./make.sh clean;
        fi
    done
fi

if [ "$reverts" == 5 ]; then
    echo "- Even the last revert is failed, seems like there are bunch of commits that are need to be reverted in-order to work.";
    exit 1;
fi