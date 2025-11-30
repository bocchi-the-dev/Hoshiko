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
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <libgen.h>
#include <signal.h>
#include <ctype.h>
#include <dirent.h>

// vars
extern int blockedMod;
extern int blockedSys;
extern bool useStdoutForAllLogs;
extern bool shouldNotForceReMalwackUpdateNextTime;
extern char *version;
extern char *versionCode;
extern char *daemonPackageLists;
extern const char *MODPATH;
extern const char *modulePropFile;
extern const char *configScriptPath;
extern const char *hostsPath;
extern const char *hostsBackupPath;
extern const char *persistDir;
extern const char *daemonLogs;
extern const char *daemonLockFile;
extern const char *daemonLockFileStuck;
extern const char *daemonLockFileSuccess;
extern const char *systemHostsPath;
extern const char *killDaemon;

// used for logging shits
enum elogLevel {
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_ABORT
};

// used to determine the current daemon state.
enum daemonProcessState {
    RUNNING_RUNABLE,
    NOT_RUNNING_CANT_RUN
};

// used for toggling the daemon shit
enum daemonState {
    ENABLE_ENABLED,
    DISABLE_DISABLED
};

// functions:
int putConfig(const char *variableName, const int variableValue);
int putConfigAppend(const char *variableName, int variableValue, bool addVariable404);
bool isPackageInList(const char *packageName);
bool removePackageFromList(const char *packageName);
bool addPackageToList(const char *packageName);
bool eraseFile(const char *file);
bool executeShellCommands(const char *command, char *const args[]);
bool isDefaultHosts(const char *filename);
bool canDaemonRun(void);
bool copyTextFile(const char *src, const char *dest);
char *grepProp(const char *variableName, const char *propFile);
char *combineStringsFormatted(const char *format, ...);
char *getCurrentPackage();
void consoleLog(enum elogLevel loglevel, const char *service, const char *message, ...);
void abort_instance(const char *service, const char *format, ...);
void printBannerWithRandomFontStyle();
void pauseADBlock();
void resumeADBlock();
void help(const char *wehgcfbkfbjhyghxdrbtrcdfv);
void freePointer(void **ptr);
void refreshBlockedCounts();
void reWriteModuleProp(const char *desk);
void killDaemonWhenSignaled(int sig);
void checkIfModuleExists(void);
void appendAlyaProps(void);
void wipePointers(void);
