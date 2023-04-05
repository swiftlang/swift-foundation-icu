//===----------------------------------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2022 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===/

// First, the platform type. Need this for U_PLATFORM.
#include "unicode/platform.h"

#include "cfbundle.h"
#include "cmemory.h"

/* Include standard headers. */
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <dlfcn.h>

#define RESOURCE_BUNDLE_NAME "FoundationICU_FoundationICU"
#define BUNDLE_CONTENTS_DIRECTORY_NAME "Contents"
#define BUNDLE_RESOURCES_DIRECTORY_NAME "Resources"


static void __CFBundleRemoveLastPathComponent(char *path) {
    char *lastSlash = strrchr(path, '/');
    if (lastSlash) {
        *lastSlash = 0;
    }
}

static const char *__CFBundleCopyAppendingPathComponent(const char *path, const char *pathComponent) {
    size_t pathLen = strnlen(path, PATH_MAX + 1);
    size_t componentLen = strnlen(pathComponent, PATH_MAX + 1);

    if (pathLen + componentLen >= PATH_MAX) {
        return strdup(path);
    }
    char result[PATH_MAX + 1];
    strncpy(result, path, PATH_MAX);
    // Append the resource bundle name
    strcat(result, "/");
    strncat(result, pathComponent, PATH_MAX);
    return strdup(result);
}

static const char *__CFBundleGetLastPathComponent(char *path) {
    char *nullTerminator = strrchr(path, 0);
    long pathLength = nullTerminator - path;
    if (pathLength <= 0) {
        return path;
    }
    char *lastSlash = strrchr(path, '/');
    if (!lastSlash) {
        return path;
    }
    // If there's more contents after `/`, that's
    // the last path component
    if (nullTerminator - lastSlash > 1) {
        return (lastSlash + 1);
    }
    // There's no more contents (the string ends
    // with `/`. Remove it and try again
    char *newPath = (char *)uprv_malloc(sizeof(char) * (pathLength - 1));
    strncpy(newPath, path, pathLength - 2);
    return __CFBundleGetLastPathComponent(newPath);
}

static const char * __CFBundleGetPlatformExecutablesSubdirectoryName() {
#if U_PLATFORM_IS_DARWIN_BASED
    return "MacOS";
#elif U_PLATFORM_IS_LINUX_BASED
    #if U_PF_CYGWIN
        return "Cygwin";
    #else
        return "Linux";
    #endif
#elif U_PF_WINDOWS
    return "Windows";
#elif U_PF_SOLARIS
    return "Solaris";
#elif U_PF_HPUX
    return "HPUX";
#endif
}

static char * __CFBundleCopyBundlePathForExecutablePath(const char *executablePath) {
    char path[PATH_MAX + 1];
    size_t executablePathLen = strnlen(executablePath, PATH_MAX + 1);
    if (executablePathLen > PATH_MAX) {
        return strdup(executablePath);
    }
    strncpy(path, executablePath, PATH_MAX);
    // First remove the executable name
    __CFBundleRemoveLastPathComponent(path);
    // Check if the executable is contained within
    // platform executable subdirectory. If so,
    // remove those
    if (strcmp(__CFBundleGetLastPathComponent(path),
        __CFBundleGetPlatformExecutablesSubdirectoryName()) == 0) {
        // Remove platform folder (e.g. "MacOS")
        __CFBundleRemoveLastPathComponent(path);
        // Remove the support files folder (e.g. "Contents")
        __CFBundleRemoveLastPathComponent(path);
    }

    return strdup(path);
}

static const char * __CFBundleSearchDirectoryForResourceBundle(const char *directory) {
    struct dirent *dirEntry;
    DIR *dir = opendir(directory);
    while ((dirEntry = readdir(dir)) != NULL) {
        if (strcasestr(dirEntry->d_name, RESOURCE_BUNDLE_NAME)) {
            // Found!
            return __CFBundleCopyAppendingPathComponent(directory, dirEntry->d_name);
        }
        // If the current item is the "Contents" or "Resources" folder, dig down
        if (strcasestr(dirEntry->d_name, BUNDLE_CONTENTS_DIRECTORY_NAME) || strcasestr(dirEntry->d_name, BUNDLE_RESOURCES_DIRECTORY_NAME)) {
            const char *newPath = __CFBundleCopyAppendingPathComponent(directory, dirEntry->d_name);
            const char *result = __CFBundleSearchDirectoryForResourceBundle(newPath);
            uprv_free((void *)newPath);
            return result;
        }
    }

    return NULL;
}

const char* getPackageICUDataPath() {
    // First determine the executable path
    Dl_info dl_info;
    dladdr(reinterpret_cast<const void*>(getPackageICUDataPath), &dl_info);
    const char *libraryFilename = dl_info.dli_fname;
    if (libraryFilename == 0 || libraryFilename[0] == 0) {
        return "";
    }
    char *mainBundlePath = __CFBundleCopyBundlePathForExecutablePath(libraryFilename);
    // First search the bundle to see if the resource bundle
    // is embedded within the main bundle
    const char *result = __CFBundleSearchDirectoryForResourceBundle(mainBundlePath);
    if (!result) {
        // Now search sibling bundles
        __CFBundleRemoveLastPathComponent(mainBundlePath);
        result = __CFBundleSearchDirectoryForResourceBundle(mainBundlePath);
    }
    uprv_free((void *)mainBundlePath);
    if (!result) {
        return "";
    }
    // Now we found the bundle. We might need to dig down more
    // If the bundle also contains Contents/Resources, add those too
    DIR *dir = opendir(result);
    struct dirent *dirEntry;
    while ((dirEntry = readdir(dir)) != NULL) {
        // If the current item is the "Contents" folder, dig down
        if (strcasestr(dirEntry->d_name, BUNDLE_CONTENTS_DIRECTORY_NAME)) {
            const char *innerResult = __CFBundleCopyAppendingPathComponent(result, "Contents/Resources");
            uprv_free((void *)result);
            return innerResult;
        }
    }
    return result;
}
