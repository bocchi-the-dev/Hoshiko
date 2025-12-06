//
// Copyright (C) 2025 ぼっち <ayumi.aiko@outlook.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <daemon.h>
#include <getopt.h>

// vars:
int blockedMod = 0;
int blockedSys = 0;
bool useStdoutForAllLogs = true;
bool shouldNotForceReMalwackUpdateNextTime = false;
char *version = NULL;
char *versionCode = NULL;
char *daemonPackageLists = "/data/adb/Re-Malwack/remalwack-package-lists.txt";
const char *configScriptPath = "/data/adb/Re-Malwack/config.sh";
const char *MODPATH = "/data/adb/modules/Re-Malwack";
const char *modulePropFile = "/data/adb/modules/Re-Malwack/module.prop";
const char *hostsPath = "/data/adb/modules/Re-Malwack/system/etc/hosts";
const char *hostsBackupPath = "/data/adb/modules/Re-Malwack/hosts.bak";
const char *daemonLogs = "/sdcard/Android/data/alya.roshidere.lana/logs";
const char *persistDir = "/data/adb/Re-Malwack";
const char *daemonLockFileStuck = "/data/adb/Re-Malwack/.daemon0";
const char *daemonLockFileSuccess = "/data/adb/Re-Malwack/.daemon1";
const char *daemonLockFileFailure = "/data/adb/Re-Malwack/.daemon2";
const char *killDaemon = "/data/adb/Re-Malwack/.daemon3";
const char *systemHostsPath = "/system/etc/hosts";

// never used struct that much, but here i'm using this 
// "class" kinda thing.
struct option longOptions[] = {
    {"help", no_argument, 0, 'h'},
    {"add-app", required_argument, 0, 'a'},
    {"remove-app", required_argument, 0, 'r'},
    {"import-package-list", required_argument, 0, 'i'},
    {"export-package-list", required_argument, 0, 'e'},
    {"enable-daemon", no_argument, 0, 'x'},
    {"disable-daemon", no_argument, 0, 'd'},
    {"lana-app", no_argument, 0, 'l'},
    {"kill-daemon", no_argument, 0, 'k'},
    {0,0,0,0}
};

int main(int argc, char *argv[]) {
    makeDir(daemonLogs);
    printBannerWithRandomFontStyle();
    checkIfModuleExists();
    appendAlyaProps();
    if(getuid()) abort_instance("main-alya", "This binary should be running as root.");
    int opt;
    int longindex = 0;
    while((opt = getopt_long(argc, argv, "ha:r:i:e:xdy:k", longOptions, &longindex)) != -1) {
        switch(opt) {
            default:
            case 'h':
                help(argv[0]);
            break;
            case 'l':
                useStdoutForAllLogs = false;
            break;
            case 'a':
                if(isPackageInList(optarg)) {
                    consoleLog(LOG_LEVEL_INFO, "main-alya", "Existing package can't be added once again!");
                    return -1;
                }
                else addPackageToList(optarg);
                return 0;
            break;
            case 'r':
                if(isPackageInList(optarg)) {
                    removePackageFromList(optarg);
                    consoleLog(LOG_LEVEL_INFO, "main-alya", "Requested package has been removed successfully!");
                    return 0;
                }
                else {
                    consoleLog(LOG_LEVEL_ERROR, "main-alya", "%s is not found in the list.", optarg);
                    return -1;
                }
            break;
            case 'i':
                eraseFile(daemonLockFileStuck);
                if(access(optarg, F_OK) != 0) {
                    consoleLog(LOG_LEVEL_ERROR, "main-alya", "Failed to access the given import package lists file.");
                    return -1;
                }
                if(copyTextFile(optarg, daemonPackageLists)) {
                    eraseFile(daemonLockFileSuccess);
                    remove(daemonLockFileStuck);
                    consoleLog(LOG_LEVEL_INFO, "main-alya", "Successfully imported the package list. Thank you for using Hoshiko!");
                    return 0;
                }
                else {
                    eraseFile(daemonLockFileFailure);
                    consoleLog(LOG_LEVEL_ERROR, "main-alya", "Failed to import the package list. Please try again!");
                    return -1;
                }
            break;
            case 'e':
                if(access(daemonPackageLists, F_OK) != 0) abort_instance("main-alya", "Failed to access the package lists file. It might be corrupted or missing.");
                if(copyTextFile(daemonPackageLists, optarg)) {
                    consoleLog(LOG_LEVEL_INFO, "main-alya", "Successfully copied the file to the requested location. Thank you for using Hoshiko!");
                    return 0;
                }
                else {
                    consoleLog(LOG_LEVEL_ERROR, "main-alya", "Failed to copy the file to the requested location. Please try again!");
                    return -1;
                }
            break;
            case 'x':
                if(putConfig("enable_daemon", ENABLE_ENABLED) != 0) abort_instance("main-alya", "Failed to enable the daemon, please try again!");
                consoleLog(LOG_LEVEL_INFO, "main-alya", "Successfully enabled the daemon, enjoy!");
                consoleLog(LOG_LEVEL_INFO, "main-alya", "The app will start yuki soon if you used this argument via yuki. Thank you!");
                return 0;
            break;
            case 'd':
                if(putConfig("enable_daemon", DISABLE_DISABLED) != 0) abort_instance("main-alya", "Failed to disable the daemon, please try again!");
                consoleLog(LOG_LEVEL_INFO, "main-alya", "Successfully disabled the daemon, enjoy!");
                eraseFile(killDaemon);
                return 0;
            break;
            case 'k':
                consoleLog(LOG_LEVEL_INFO, "main-alya", "Yuki should die within 1-2 seconds, if it takes more than it, it means that it's either updating or got stucked somehow. Reboot your device if you are unsure!");
                eraseFile(killDaemon);
                putConfig("current_daemon_pid", -1);
                return 0;
            break;
        }
    }
    return 1;
}
