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

int putConfig(const char *variableName, int variableValue) {
    return putConfigAppend(variableName, variableValue, false);
}

int putConfigAppend(const char *variableName, int variableValue, bool addVariable404) {
    FILE *fp = fopen(configScriptPath, "r");
    if(!fp) {
        consoleLog(LOG_LEVEL_ERROR, "putConfig", "Failed to open %s in read-only mode, please try again..", configScriptPath);
        return 127;
    }
    char lines[64][100];
    int lineCount = 0;
    bool didItGetChanged = false;
    while(fgets(lines[lineCount], sizeof(lines[lineCount]), fp)) {
        if(lineCount >= 100) break;
        lineCount++;
    }
    fclose(fp);
    FILE *fpt = fopen(configScriptPath, "w");
    if(!fpt) {
        consoleLog(LOG_LEVEL_ERROR, "putConfig", "Failed to open %s in write-only mode, please try again..", configScriptPath);
        return 127;
    }
    for(int i = 0; i < lineCount; i++) {
        if(strncmp(lines[i], variableName, strlen(variableName)) == 0 && 
            lines[i][strlen(variableName)] == '=') {
            fprintf(fpt, "%s=%d\n", variableName, variableValue);
            didItGetChanged = true;
        }
        else fprintf(fpt, "%s", lines[i]);
    }
    if(!didItGetChanged && addVariable404) {
        fprintf(fpt, "%s=%d\n", variableName, variableValue);
        didItGetChanged = true;
    }
    fclose(fpt);
    return (didItGetChanged) ? 0 : 1;
}

bool isPackageInList(const char *packageName) {
    FILE *packageFile = fopen(daemonPackageLists, "r");
    if(!packageFile) abort_instance("isPackageInList", "Failed to open the package lists file, please run this command again or report this issue to the devs.");
    char contentFromFile[1028];
    while(fgets(contentFromFile, sizeof(contentFromFile), packageFile)) {
        contentFromFile[strcspn(contentFromFile, "\n")] = 0;
        if(strcmp(contentFromFile, packageName) == 0) {
            fclose(packageFile);
            return true;
        }
    }
    fclose(packageFile);
    return false;
}

bool removePackageFromList(const char *packageName) {
    FILE *packageFile = fopen(daemonPackageLists, "r");
    if(!packageFile) abort_instance("removePackageFromList", "Failed to open the package lists file, please run this command again or report this issue to the devs.");
    eraseFile("/data/local/tmp/temp");
    // no need to put ts in w mode to erase previous stuff as the eraseFile itself opens the file in W to remove the previous contents.
    FILE *tempFile = fopen("/data/local/tmp/temp", "a");
    if(!tempFile) abort_instance("removePackageFromList", "Failed to open a temporary file, please run this command again or report this issue to the devs.");
    char contentFromFile[1028];
    bool status = false;
    while(fgets(contentFromFile, sizeof(contentFromFile), packageFile)) {
        eraseFile(daemonLockFileStuck);
        contentFromFile[strcspn(contentFromFile, "\n")] = 0;
        if(strcmp(contentFromFile, packageName) == 0) {
            consoleLog(LOG_LEVEL_INFO, "removePackageFromList", "Found %s on the list, removing the package from the list...", packageName);
            // skip writing to the temp file so we can write other strings aka the packages.
            // use a bool to indicate that we skipped copying it to the temp file.
            status = true;
        }
        else fprintf(tempFile, "%s\n", contentFromFile);
    }
    fclose(tempFile);
    fclose(packageFile);
    if(status) {
        rename("/data/local/tmp/temp", daemonPackageLists);
        eraseFile(daemonLockFileSuccess);
    }
    remove(daemonLockFileStuck);
    return status;
}

bool eraseFile(const char *file) {
    FILE *fileConstantAgain = fopen(file, "w");
    if(!fileConstantAgain) return false;
    fclose(fileConstantAgain);
    return true;
}

bool executeShellCommands(const char *command, char *const args[]) {
    pid_t ProcessID = fork();
    switch(ProcessID) {
        case -1:
            abort_instance("executeShellCommands", "Failed to fork to continue.");
        break;
        case 0:
            // throw output to /dev/null- LOGS THIS TIME!
            consoleLog(LOG_LEVEL_DEBUG, "executeShellCommands", "Redirecting stdout/stderr output to log file..");
            consoleLog(LOG_LEVEL_DEBUG, "executeShellCommands", "The log could contain information that are not supposed to be shared or it might be blank, but who knows?");
            int devNull = open(daemonLogs, O_WRONLY);
            if(devNull == -1) exit(EXIT_FAILURE);
            dup2(devNull, STDOUT_FILENO);
            dup2(devNull, STDERR_FILENO);
            close(devNull);
            execvp(command, args);
        break;
        default:
            consoleLog(LOG_LEVEL_DEBUG, "executeShellCommands", "Waiting for current %d to finish", ProcessID);
            consoleLog(LOG_LEVEL_DEBUG, "executeShellCommands", "Finished loggin' things - switch");
            int status;
            wait(&status);
            return (WIFEXITED(status)) ? WEXITSTATUS(status) : false;
        }
    // me: shut up compiler
    // evil gurt: yo
    // me: shut up
    consoleLog(LOG_LEVEL_DEBUG, "executeShellCommands", "Finished loggin' things - local scope");
    return false;
}

bool canDaemonRun(void) {
    char *enableDaemon = grepProp("enable_daemon", configScriptPath);
    char *isDaemonRunning = grepProp("is_daemon_running", configScriptPath);
    if(enableDaemon && isDaemonRunning && strcmp(enableDaemon, "0") == 0 && strcmp(isDaemonRunning, "1") == 0) {
        freePointer((void **)&enableDaemon);
        freePointer((void **)&isDaemonRunning);
        return true;
    }
    freePointer((void **)&enableDaemon);
    freePointer((void **)&isDaemonRunning);
    return false;
}

bool copyTextFile(const char *src, const char *dest) {
    FILE *source = fopen(src, "r");
    FILE *destination = fopen(dest, "w");
    if(!source) {
        consoleLog(LOG_LEVEL_ERROR, "copyTextFile", "Failed to open the source file: %s, No such file or directory", src);
        return false;
    }
    if(!destination) {
        consoleLog(LOG_LEVEL_ERROR, "copyTextFile", "Failed to open the destination file: %s, No such file or directory", dest);
        fclose(source);
        return false;
    }
    char contentFromFile[1024][1000];
    int lines = 0;
    while(fgets(contentFromFile[lines], sizeof(contentFromFile[lines]), source)) {
        fprintf(destination, "%s", contentFromFile[lines]);
        lines++;
    }
    fclose(source);
    fclose(destination);
    return true;
}

char *grepProp(const char *variableName, const char *propFile) {
    FILE *filePointer = fopen(propFile, "r");
    if(!filePointer) {
        consoleLog(LOG_LEVEL_ERROR, "grepProp", "Failed to open properties file: %s", propFile);
        return NULL;
    }
    char theLine[1024];
    size_t lengthOfTheString = strlen(variableName) + 2;
    while(fgets(theLine, sizeof(theLine), filePointer)) {
        if(strncmp(theLine, variableName, lengthOfTheString) == 0 && theLine[lengthOfTheString] == '=') {
            strtok(theLine, "=");
            if(strtok(NULL, "\n")) {
                char *result = strdup(strtok(NULL, "\n"));
                fclose(filePointer);
                return result;
            }
        }
    }
    fclose(filePointer);
    return NULL;
}

char *getCurrentPackage() {
    char packageName[100] = {0};
    FILE *fptr = popen("timeout 1 dumpsys 2>/dev/null | grep mFocused | awk '{print $3}' | head -n 1 | awk -F'/' '{print $1}'", "r");
    if(!fptr) abort_instance("getCurrentPackage", "Failed to fetch shell output. Are you running on Android shell?");
    while(fgets(packageName, sizeof(packageName), fptr)) {
        packageName[strcspn(packageName, "\n")] = 0;
        pclose(fptr);
        return strdup(packageName);
    }
    pclose(fptr);
    return NULL;
}

char *combineStringsFormatted(const char *format, ...) {
    va_list args;
    va_start(args, format);
    va_list args_copy;
    va_copy(args_copy, args);
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wformat-nonliteral"
    int len = vsnprintf(NULL, 0, format, args_copy);
    #pragma clang diagnostic pop
    va_end(args_copy);
    if(len < 0) {
        va_end(args);
        return NULL;
    }
    size_t size = (size_t)len + 1;
    char *result = malloc(size);
    if(!result) {
        va_end(args);
        return NULL;
    }
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wformat-nonliteral"
    vsnprintf(result, size, format, args);
    #pragma clang diagnostic pop
    va_end(args);
    return result;
}

void consoleLog(enum elogLevel loglevel, const char *service, const char *message, ...) {
    va_list args;
    va_start(args, message);
    FILE *out = NULL;
    bool toFile = false;
    if(useStdoutForAllLogs) {
        out = stdout;
        if(loglevel == LOG_LEVEL_ERROR || loglevel == LOG_LEVEL_WARN || loglevel == LOG_LEVEL_ABORT) out = stderr;
    }
    else {
        out = fopen(daemonLogs, "a");
        if(!out) exit(EXIT_FAILURE);
        toFile = true;
    }
    switch(loglevel) {
        case LOG_LEVEL_INFO:
            if(!toFile) fprintf(out, "\033[2;30;47mINFO: ");
            else fprintf(out, "INFO: %s(): ", service);
        break;
        case LOG_LEVEL_WARN:
            if(!toFile) fprintf(out, "\033[1;33mWARNING: ");
            else fprintf(out, "WARNING: %s(): ", service);
        break;
        case LOG_LEVEL_DEBUG:
            // skip dbg texts if we are going to put them in std{out,err}.
            if(!toFile) {
                va_end(args);
                return;
            }
            else fprintf(out, "DEBUG: %s(): ", service);
        break;
        case LOG_LEVEL_ERROR:
            if(!toFile) fprintf(out, "\033[0;31mERROR: ");
            else fprintf(out, "ERROR: %s(): ", service);
        break;
        case LOG_LEVEL_ABORT:
            if(!toFile) fprintf(out, "\033[0;31mABORT: ");
            else fprintf(out, "ABORT: %s(): ", service);
        break;
    }
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wformat-nonliteral"
    vfprintf(out, message, args);
    #pragma clang diagnostic pop
    if(!toFile) fprintf(out, "\033[0m\n");
    else fprintf(out, "\n");
    if(!useStdoutForAllLogs && out) fclose(out);
    va_end(args);
}

void abort_instance(const char *service, const char *format, ...) {
    va_list args;
    va_start(args, format);
    consoleLog(LOG_LEVEL_ABORT, service, format, args);
    va_end(args);
    freePointer((void **)&version);
    freePointer((void **)&versionCode);
    exit(EXIT_FAILURE);
}

void printBannerWithRandomFontStyle() {
    const char *banner1 =
        "\033[0;31mdb   db  .d88b.  .d8888. db   db d888888b db   dD  .d88b.  \n"
        "88   88 .8P  Y8. 88'  YP 88   88   `88'   88 ,8P' .8P  Y8. \n"
        "88ooo88 88    88 `8bo.   88ooo88    88    88,8P   88    88 \n"
        "88~~~88 88    88   `Y8b. 88~~~88    88    88`8b   88    88 \n"
        "88   88 `8b  d8' db   8D 88   88   .88.   88 `88. `8b  d8' \n"
        "YP   YP  `Y88P'  `8888Y' YP   YP Y888888P YP   YD  `Y88P'  \033[0m\n";
    const char *banner2 =
        "\033[0;31m __  __               _        \\           \n"
        " |   |    __.    ____ /      ` |   ,   __. \n"
        " |___|  .'   \\  (     |,---. | |  /  .'   \\\n"
        " |   |  |    |  `--.  |'   ` | |-<   |    |\n"
        " /   /   `._.' \\___.' /    | / /  \\_  `._.'\n"
        "                                            \033[0m\n";
    const char *banner3 =
        "\033[0;31m  ::   .:      ...      .::::::.   ::   .:  ::: :::  .      ...     \n"
        " ,;;   ;;,  .;;;;;;;.  ;;;`    `  ,;;   ;;, ;;; ;;; .;;,..;;;;;;;.  \n"
        ",[[[,,,[[[ ,[[     \\[[,'[==/[[[[,,[[[,,,[[[ [[[ [[[[[/' ,[[     \\[[,\n"
        "\"$$$\"\"\"$$$ $$$,     $$$  '''    $\"$$$\"\"\"$$$ $$$_$$$$,   $$$,     $$$\n"
        " 888   \"88o\"888,_ _,88P 88b    dP 888   \"88o888\"888\"88o,\"888,_ _,88P\n"
        " MMM    YMM  \"YMMMMMP\"   \"YMmMY\"  MMM    YMMMMM MMM \"MMP\" \"YMMMMMP\" \033[0m\n";
    const char *banner4 =
        "\033[0;31mM\"\"MMMMM\"\"MM                   dP       oo dP                \n"
        "M  MMMMM  MM                   88          88                \n"
        "M         `M .d8888b. .d8888b. 88d888b. dP 88  .dP  .d8888b. \n"
        "M  MMMMM  MM 88'  `88 Y8ooooo. 88'  `88 88 88888\"   88'  `88 \n"
        "M  MMMMM  MM 88.  .88       88 88    88 88 88  `8b. 88.  .88 \n"
        "M  MMMMM  MM `88888P' `88888P' dP    dP dP dP   `YP `88888P' \n"
        "MMMMMMMMMMMM                                                 \n"
        "                                                              \033[0m\n";
    const char *banner5 =
        "\033[0;31m                                                                                                         \n"
        "8 8888        8     ,o888888o.       d888888o.   8 8888        8  8 8888 8 8888     ,88'  ,o888888o.     \n"
        "8 8888        8  . 8888     `88.   .`8888:' `88. 8 8888        8  8 8888 8 8888    ,88'. 8888     `88.   \n"
        "8 8888        8 ,8 8888       `8b  8.`8888.   Y8 8 8888        8  8 8888 8 8888   ,88',8 8888       `8b  \n"
        "8 8888        8 88 8888        `8b `8.`8888.     8 8888        8  8 8888 8 8888  ,88' 88 8888        `8b \n"
        "8 8888        8 88 8888         88  `8.`8888.    8 8888        8  8 8888 8 8888 ,88'  88 8888         88 \n"
        "8 8888        8 88 8888         88   `8.`8888.   8 8888        8  8 8888 8 8888 88'   88 8888         88 \n"
        "8 8888888888888 88 8888        ,8P    `8.`8888.  8 8888888888888  8 8888 8 888888<    88 8888        ,8P \n"
        "8 8888        8 `8 8888       ,8P 8b   `8.`8888. 8 8888        8  8 8888 8 8888 `Y8.  `8 8888       ,8P  \n"
        "8 8888        8  ` 8888     ,88'  `8b.  ;8.`8888 8 8888        8  8 8888 8 8888   `Y8. ` 8888     ,88'   \n"
        "8 8888        8     `8888888P'     `Y8888P ,88P' 8 8888        8  8 8888 8 8888     `Y8.  `8888888P'     \033[0m\n";
    const char *banners[] = {banner1, banner2, banner3, banner4, banner5};
    srand((unsigned int)time(NULL));
    printf("%s\n", banners[rand() % (int)(sizeof(banners) / sizeof(banners[0]))]);
}

void pauseADBlock() {
    if(access(hostsBackupPath, F_OK) == 0) consoleLog(LOG_LEVEL_WARN, "pauseADBlock", "Protection is already paused!");
    refreshBlockedCounts(); // refresh it because both default val is set to 0
    if(blockedMod + blockedSys == 0) abort_instance("pauseADBlock", "You can't pause Re-Malwack while the hosts file is reset. Please reset the hosts file first.");
    consoleLog(LOG_LEVEL_INFO, "pauseADBlock", "Trying to pause Re-Malwack's protection...");
    FILE *backupHostsFile = fopen(hostsBackupPath, "w");
    if(!backupHostsFile) abort_instance("pauseADBlock", "Failed to open hosts backup file: %s", hostsBackupPath);
    FILE *hostsFile = fopen(hostsPath, "r+");
    if(!hostsFile) {
        fclose(backupHostsFile);
        abort_instance("pauseADBlock", "Failed to open hosts file: %s", hostsPath);
    }
    char string[1024];
    while(fgets(string, sizeof(string), hostsFile)) fprintf(backupHostsFile, "%s", string);
    fclose(backupHostsFile);
    fprintf(hostsFile, "127.0.0.1 localhost\n::1 localhost\n");
    fclose(hostsFile);
    chmod(hostsPath, 0644);
    putConfig("adblock_switch", 1);
    refreshBlockedCounts();
    reWriteModuleProp("Status: Protection is temporarily disabled due to the daemon toggling the module for an app");
}

void resumeADBlock() {
    consoleLog(LOG_LEVEL_INFO, "resumeADBlock", "Trying to resume protection...");
    if(access(hostsBackupPath, F_OK) == 0) {
        reWriteModuleProp("Status: Protection is temporarily enabled due to the daemon toggling the module for an app");
        FILE *backupHostsFile = fopen(hostsBackupPath, "r");
        if(!backupHostsFile) abort_instance("resumeADBlock", "Failed to open backup hosts file: %s\nDue to this failure, we are unable to restore previous backed-up hosts.", hostsBackupPath);
        FILE *hostsFile = fopen(hostsPath, "w");
        if(!hostsFile) {
            fclose(backupHostsFile);
            abort_instance("resumeADBlock", "Failed to open hosts file: %s", hostsPath);
        }
        char string[1024];
        while(fgets(string, sizeof(string), backupHostsFile)) fprintf(hostsFile, "%s", string);
        fclose(hostsFile);
        fclose(backupHostsFile);
        chmod(hostsPath, 0644);
        remove(hostsBackupPath);
        executeShellCommands("sync", (char * const[]){"sync", NULL});
        refreshBlockedCounts();
        putConfig("adblock_switch", 0);
        consoleLog(LOG_LEVEL_INFO, "resumeADBlock", "Protection services have been resumed.");
    }
    else {
        consoleLog(LOG_LEVEL_INFO, "resumeADBlock", "No backup hosts file found to resume, force resuming protection and running hosts update as a fallback action");
        putConfig("adblock_switch", 0);
        // i've come to the conclusion that i should have a boolean for this action
        // to stop running --update-hosts everytime.
        if(!shouldNotForceReMalwackUpdateNextTime) {
            if(executeShellCommands("/data/adb/modules/Re-Malwack/rmlwk.sh", (char * const[]){"/data/adb/modules/Re-Malwack/rmlwk.sh", "--hoshiko", "--update-hosts"}) == 0) shouldNotForceReMalwackUpdateNextTime = true;
        }
    }
}

void help(const char *wehgcfbkfbjhyghxdrbtrcdfv) {
    char *binaryName = basename(wehgcfbkfbjhyghxdrbtrcdfv);
    printf("Usage:\n");
    printf("  %s [OPTION] [ARGUMENTS]\n\n", binaryName);
    printf("Options:\n");
    printf("-a  |  --add-app <app_name>\t\tAdd an application to the list to stop ad blocker when the app is opened.\n");
    printf("-r  |  --remove-app <app_name>\tRemove an application from the list.\n");
    printf("-i  |  --import-package-list <file>\tImport the app list from the already exported file.\n");
    printf("-e  |  --export-package-list <file>\tExport the encoded app list to a path for restoration.\n");
    printf("-x  |  --enable-daemon\tEnables \n");
    printf("-d  |  --disable-daemon\n");
    printf("-h  |  --help\t\t\tDisplay this help message.\n\n");
    printf("Examples:\n");
    printf("  %s --add-app com.example.myapp\n", binaryName);
    printf("  %s --remove-app com.example.myapp\n", binaryName);
    printf("  %s --export-package-list apps.txt\n", binaryName);
    printf("  %s --import-package-list apps.txt\n", binaryName);
    printf("  %s --enable-daemon\n", binaryName);
    printf("  %s --disable-daemon\n", binaryName);
}

void freePointer(void **ptr) {
    if(ptr && *ptr) {
        free(*ptr);
        *ptr = NULL;
    }
}

void refreshBlockedCounts() {
    // reset it before counting all over again.
    blockedMod = 0;
    blockedSys = 0;
    mkdir(combineStringsFormatted("%s/counts", persistDir), 0755);
    FILE *hostsThingMod = fopen(hostsPath, "r");
    if(!hostsThingMod) abort_instance("refreshBlockedCounts", "Failed to open %s for reading to update blocklist count", hostsPath);
    char contentSizeMod[1024];
    while(fgets(contentSizeMod, sizeof(contentSizeMod), hostsThingMod)) if(strncmp(contentSizeMod, "0.0.0.0 ", 8) == 0) blockedMod++;
    fclose(hostsThingMod);
    FILE *hostsThingSys = fopen(systemHostsPath, "r");
    if(!hostsThingSys) abort_instance("refreshBlockedCounts", "Failed to open %s for reading to update blocklist count", systemHostsPath);
    char contentSizeSys[1024];
    while(fgets(contentSizeSys, sizeof(contentSizeSys), hostsThingSys)) if(strncmp(contentSizeSys, "0.0.0.0 ", 8) == 0) blockedSys++;
    fclose(hostsThingSys);
    FILE *sysCountFile = fopen(combineStringsFormatted("%s/counts/blocked_sys.count", persistDir), "w");
    if(!sysCountFile) abort_instance("refreshBlockedCounts", "Failed to open %s to update adblock count", combineStringsFormatted("%s/counts/blocked_sys.count", persistDir));
    fprintf(sysCountFile, "%d", blockedSys);
    fclose(sysCountFile);
    FILE *modCountFile = fopen(combineStringsFormatted("%s/counts/blocked_mod.count", persistDir), "w");
    if(!modCountFile) abort_instance("refreshBlockedCounts", "Failed to open %s to update adblock count", combineStringsFormatted("%s/counts/blocked_mod.count", persistDir));
    fprintf(modCountFile, "%d", blockedMod);
    fclose(modCountFile);
}

void reWriteModuleProp(const char *desk) {
    // "write" instead of "append" to rewrite everything line by line.
    // you know what? forget it, i was just being lame.
    // so basically i hate the way i wrote the property before. it felt like crap.
    FILE *moduleProp = fopen(modulePropFile, "r+");
    if(!moduleProp) abort_instance("reWriteModuleProp", "Failed to open module's property file. Please report this error in my discord!");
    char content[1024];
    while(fgets(content, sizeof(content), moduleProp)) {
        // let's remove the newline if we are in desc.
        if(strstr(content, "description=")) { 
            content[strcspn(content, "\n")] = 0;
            fprintf(moduleProp, "description=%s\n", desk);
        }
        else fputs(content, moduleProp);
    }
    fclose(moduleProp);
}

void killDaemonWhenSignaled(int sig) {
    putConfig("is_daemon_running", NOT_RUNNING_CANT_RUN); 
    putConfig("current_daemon_pid", -1);
    consoleLog(LOG_LEVEL_DEBUG, "killDaemonWhenSignaled", "sig: %d", sig);
    exit(EXIT_FAILURE);
}

void checkIfModuleExists(void) {
    DIR *modulePath = opendir("/data/adb/modules/Re-Malwack");
    if(!modulePath) abort_instance("checkIfModuleExists", "Failed to open the module directory, please install Re-Malwack if not already!");
    closedir(modulePath);
}

void appendAlyaProps(void) {
    int appendedProps = 0;
    int defaultPropertyValues[] = {0, 1, -1};
    char *defaultPropertyNames[] = {"is_daemon_running", "enable_daemon", "current_daemon_pid"};
    consoleLog(LOG_LEVEL_INFO, "appendAlyaProps", "Appending %d properties that might not exist...", sizeof(defaultPropertyNames) / sizeof(defaultPropertyNames[0]));
    for(size_t i = 0; i < sizeof(defaultPropertyNames) / sizeof(defaultPropertyNames[0]); i++) {
        if(putConfig(defaultPropertyNames[i], defaultPropertyValues[i]) != 0) {
            putConfigAppend(defaultPropertyNames[i], defaultPropertyValues[i], true);
            appendedProps++;
        }
    }
    if(appendedProps == 0) consoleLog(LOG_LEVEL_INFO, "appendAlyaProps", "Nothing is appended, required properties are all in place!");
    else consoleLog(LOG_LEVEL_INFO, "appendAlyaProps", "Finished appending %d properties..", appendedProps);
}

void wipePointers() {
    freePointer((void **)&version);
    freePointer((void **)&versionCode);
}

void addPackageToList(const char *packageName) {
    FILE *packageFile = fopen(daemonPackageLists, "a");
    if(!packageFile) abort_instance("addPackageToList", "Failed to open the package lists file, please run this command again or report this issue to the devs.");
    fprintf(packageFile, "\n%s", packageName);
    consoleLog(LOG_LEVEL_INFO, "addPackageToList", "Successfully added %s into the list, the daemon will add the packages to the list after a short period of time.", packageName);
    fclose(packageFile);
}
