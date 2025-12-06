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

// vars:
int blockedMod = 0;
int blockedSys = 0;
static int i = 0;
bool useStdoutForAllLogs = false;
bool shouldNotForceReMalwackUpdateNextTime = false;
static bool loadPackagesAgain = false;
static char **packageArray;
char *version = NULL;
char *versionCode = NULL;
char *daemonPackageLists = "/data/adb/Re-Malwack/remalwack-package-lists.txt";
char *previousDaemonPackageLists = "/data/adb/Re-Malwack/previousDaemonList";
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

int main(void) {
    makeDir(daemonLogs);
    version = grepProp("version", modulePropFile);
    if(!version) abort_instance("main-yuki", "Could not find 'version' in module.prop");
    versionCode = grepProp("versionCode", modulePropFile);
    if(!versionCode) abort_instance("main-yuki", "Could not find 'versionCode' in module.prop");
    consoleLog(LOG_LEVEL_INFO, "main-yuki", "Re-Malwack %s (versionCode: %s) is starting...", version, versionCode);
    printBannerWithRandomFontStyle();
    checkIfModuleExists();
    appendAlyaProps();
    if(getuid()) abort_instance("main-yuki", "daemon is not running as root.");
    // force stop termux instance if it's found to be in top. Just to be sure that 
    // termux should't handle the loop and it can't run some basic commands, that's why im stopping termux users.
    if(getCurrentPackage() != NULL && strcmp(getCurrentPackage(), "com.termux") == 0) {
        consoleLog(LOG_LEVEL_WARN, "main-yuki", "Sorry dear termux user, you CANNOT run this daemon in termux. Termux is not supported by Re-Malwack Daemon.");
        wipePointers();
        executeShellCommands("exit", (char * const[]){ "exit", "1", NULL });
    }
    // set the variable to true to load the packages for the first time.
    loadPackagesAgain = true;
    while(canDaemonRun()) {
        // put the pid of this daemon process.
        putConfig("current_daemon_pid", getpid());
        // set signal actions.
        signal(SIGINT, killDaemonWhenSignaled);
        signal(SIGTERM, killDaemonWhenSignaled);
        signal(SIGPWR, killDaemonWhenSignaled);
        // set the prop to 0 to indicate that we are running already.
        putConfig("is_daemon_running", 0);
        if(access(killDaemon, F_OK) == 0) {
            system(combineStringsFormatted("su -c rm -rf %s", killDaemon));
            killDaemonWhenSignaled(1);
        }
        // load packages again.
        if(loadPackagesAgain) {
            char stringsToFetch[1000];
            packageArray = malloc(sizeof(char *) * 300);
            FILE *packageLists = fopen(daemonPackageLists, "r");
            if(!packageLists) abort_instance("main-yuki", "Failed to reopen package list file.");
            // always have a backup of the daemonPackageLists because we need to have to use this
            // backup as a failsafe method when the crap didn't import properly.
            if(executeShellCommands("su", (char * const[]){"su", "-c", "cp", "-af", daemonPackageLists, "/data/adb/Re-Malwack/previousDaemonList", NULL}) != 0) abort_instance("main-yuki", "Failed to backup the daemon package lists, please try again!");
            while(fgets(stringsToFetch, sizeof(stringsToFetch), packageLists) && i < 100) {
                if(stringsToFetch[0] == '\0') continue;
                stringsToFetch[strcspn(stringsToFetch, "\n")] = 0;
                packageArray[i] = malloc(sizeof(char *));
                if(!packageArray[i]) abort_instance("main-yuki", "Failed to prepare %d st/th array for caching package list.", i);
                sprintf(packageArray[i], "%s", stringsToFetch);
                i++;
            }
            fclose(packageLists);
            consoleLog(LOG_LEVEL_DEBUG, "main-yuki", "Loaded %d packages into blocklist", i);
            consoleLog(LOG_LEVEL_DEBUG, "main-yuki", "Entering blocklist monitoring loop...");
            continue;
        }
        if(strcmp(grepProp("enable_daemon", configScriptPath), "1") == 0) {
            if(access(daemonLockFileStuck, F_OK) == 0) {
                consoleLog(LOG_LEVEL_DEBUG, "main-yuki", "Waiting for user configurations to finish...");
                usleep(500000);
                continue;
            }
            if(access(daemonLockFileSuccess, F_OK) == 0) {
                consoleLog(LOG_LEVEL_DEBUG, "main-yuki", "A package list update was triggered. Reloading packages...");
                loadPackagesAgain = true;
                remove(daemonLockFileSuccess);
            }
            else if(access(daemonLockFileFailure, F_OK) == 0) {
                consoleLog(LOG_LEVEL_DEBUG, "main-yuki", "An reset was triggered. Reverting to previous state...");
                if(executeShellCommands("su", (char * const[]){"su", "-c", "cp", "-af", previousDaemonPackageLists, daemonPackageLists, NULL}) != 0) {
                    abort_instance("main-yuki", "Failed to restore the package list, please try again!");
                }
                else {
                    remove(daemonLockFileFailure);
                    consoleLog(LOG_LEVEL_INFO, "main-yuki", "Reset finished successfully! Skipping this loop and building list again...");
                    // skip this after doing this because i dont want to code too much!
                    loadPackagesAgain = true;
                    continue;
                }
            }
            char *currentPackage = getCurrentPackage();
            if(currentPackage == NULL) {
                consoleLog(LOG_LEVEL_WARN, "main-yuki", "Failed to get current package, retrying...");
                usleep(500000);
                continue;
            }
            for(int k = 0; k < i; k++) {
                if(strcmp(currentPackage, packageArray[k]) == 0) {
                    consoleLog(LOG_LEVEL_DEBUG, "main-yuki", "%s is currently running. Handling blocklist actions.", packageArray[k]);
                    pauseADBlock();
                }
                // call ts, dw as the function will ignore if it's resumed already.
                else resumeADBlock();
            }
            freePointer((void **)&currentPackage);
            // hmm, let's not fry the cpu.
            usleep(500000);
        }
        else {
            // kill ourselves!
            exit(EXIT_SUCCESS);
        }
    }
    wipePointers();
    return 0;
}
