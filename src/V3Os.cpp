// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Os-specific function wrapper
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

// clang-format off
#ifndef PSAPI_VERSION
# define PSAPI_VERSION 1  // Needed for compatibility with Windows 7
#endif
#define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
// clang-format on

#include "config_build.h"
#include "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#include "V3Os.h"
#include "V3String.h"

#ifndef V3ERROR_NO_GLOBAL_
#include "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif

#include <cerrno>
#include <climits>  // PATH_MAX
#include <cstdarg>
#include <filesystem>  // C++17
#define PATH_MAX MAX_PATH
#include <fcntl.h>
#include <fstream>
#include <memory>

#include <sys/stat.h>
#include <sys/types.h>

#ifdef HAVE_TCMALLOC
#include <gperftools/malloc_extension.h>
#endif
#ifdef HAVE_JEMALLOC
#include <jemalloc/jemalloc.h>
#endif

// clang-format off
# include <windows.h>   // LONG for bcrypt.h
# include <algorithm>  // replace
# include <bcrypt.h>  // BCryptGenRandom
# include <chrono>
# include <direct.h>  // mkdir
# include <io.h>  // open, read, write, close
# include <thread>
// These macros taken from gdbsupport/gdb_wait.h in binutils-gdb
# ifndef WIFEXITED
#  define WIFEXITED(w) (((w) & 0377) == 0)
#  define WEXITSTATUS(w) (((w) >> 8) & 0377)
# endif
// clang-format on

#define VL_ALLOW_VERILATEDOS_C
#include "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(const string& envvar, const string& defaultValue) {
    return VString::escapeStringForPath(VlOs::getenvStr(envvar, defaultValue));
}

void V3Os::setenvStr(const string& envvar, const string& value, const string& why) {
    if (why != "") {
        UINFO(1, "export " << envvar << "='" << value << "'  # " << why);
    } else {
        UINFO(1, "export " << envvar << "='" << value << "'");
    }
    _putenv_s(envvar.c_str(), value.c_str());
}

//######################################################################
// Generic filename utilities

static constexpr char V3OS_SLASH = '\\';

static bool isSlash(char ch) VL_PURE { return ch == '/' || ch == '\\'; }

string V3Os::filenameCleanup(const string& filename) VL_PURE {
    string str;
    str.reserve(filename.length());
    bool lastIsSlash = false;
    for (const char ch : filename) {
        const bool lastIsSlashOld = lastIsSlash;
        lastIsSlash = isSlash(ch);
        if (lastIsSlash && lastIsSlashOld) continue;
        str += ch;
    }
    if (str.size() > 1 && isSlash(str.back())) str.pop_back();
    while (str.size() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    return str;
}

string V3Os::filenameJoin(std::initializer_list<const std::string> paths) VL_PURE {
    string fullpath;
    for (const auto& item : paths) {
        if (item.empty() || item == ".") continue;
        if (!fullpath.empty()) fullpath += V3OS_SLASH;
        fullpath += item;
    }
    return fullpath;
}

string V3Os::filenameDir(const string& filename) VL_PURE {
    // std::filesystem::path::parent_path
    auto it = filename.rbegin();
    for (; it != filename.rend(); ++it) {
        if (isSlash(*it)) break;
    }
    if (it.base() == filename.begin()) return ".";
    return string{filename.begin(), (++it).base()};
}

string V3Os::filenameExt(const string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    const string::size_type pos = base.find('.');
    if (pos != string::npos) {
        base.erase(0, pos);
        return base;
    }
    return "";
}

string V3Os::filenameNonDir(const string& filename) VL_PURE {
    // std::filesystem::path::filename
    auto it = filename.rbegin();
    for (; it != filename.rend(); ++it) {
        if (isSlash(*it)) break;
    }
    return string{it.base(), filename.end()};
}

string V3Os::filenameNonDirExt(const string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    const string::size_type pos = base.find('.');
    if (pos != string::npos) base.erase(pos);
    return base;
}

string V3Os::filenameSubstitute(const string& filename) {
    string result;
    // cppcheck-has-bug-suppress unusedLabel
    enum : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    for (string::size_type pos = 0; pos < filename.length(); ++pos) {
        if ((filename[pos] == '$') && (pos + 1 < filename.length())) {
            switch (filename[pos + 1]) {
            case '{': brackets = CURLY; break;
            case '(': brackets = PAREN; break;
            default: brackets = NONE; break;
            }
            if (brackets != NONE) pos = pos + 1;
            string::size_type endpos = pos + 1;
            while (((endpos + 1) < filename.length())
                   && (((brackets == NONE)
                        && (std::isalnum(filename[endpos + 1]) || filename[endpos + 1] == '_'))
                       || ((brackets == CURLY) && (filename[endpos + 1] != '}'))
                       || ((brackets == PAREN) && (filename[endpos + 1] != ')'))))
                ++endpos;
            // Catch bracket errors
            if (((brackets == CURLY) && (filename[endpos + 1] != '}'))
                || ((brackets == PAREN) && (filename[endpos + 1] != ')'))) {
                v3fatal("Unmatched brackets in variable substitution in file: " + filename);
            }
            const string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            if (!envvar.empty()) envvalue = getenvStr(envvar, "");
            if (!envvalue.empty()) {
                result += envvalue;
                if (brackets == NONE) {
                    pos = endpos;
                } else {
                    pos = endpos + 1;
                }
            } else {
                result += filename[pos];  // *pos == '$'
            }
        } else {
            result += filename[pos];
        }
    }
    return result;
}

string V3Os::filenameRealPath(const string& filename) VL_PURE {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    char retpath[PATH_MAX];
    if (_fullpath(retpath, filename.c_str(), PATH_MAX)) { return std::string{retpath}; }
    return filename;
}

string V3Os::filenameRelativePath(const string& filename, const string& base) VL_PURE {
    const string a = V3Os::filenameRealPath(filename);
    const string b = V3Os::filenameRealPath(base);
    string result;
    if (a == b) return ".";

    auto aIt = a.begin();
    auto bIt = b.begin();
    while (aIt != a.end() && bIt != b.end()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        auto aWordIt = aIt;  // position of next slash
        for (; aWordIt != a.end(); ++aWordIt) {
            if (isSlash(*aWordIt)) break;
        }

        auto bWordIt = bIt;  // position of next slash
        for (; bWordIt != b.end(); ++bWordIt) {
            if (isSlash(*bWordIt)) break;
        }

        const string aWord = a.substr(aIt - a.begin(), aWordIt - aIt);
        const string bWord = b.substr(bIt - b.begin(), bWordIt - bIt);
        if (aWord != bWord) break;
        aIt = aWordIt;
        bIt = bWordIt;
        if (aIt != a.end()) ++aIt;  // Skip slash
        if (bIt != b.end()) ++bIt;  // Skip slash
    }

    while (bIt != b.end()) {
        for (; bIt != b.end(); ++bIt) {
            if (isSlash(*bIt)) {
                ++bIt;
                break;
            }
        }
        if (!result.empty()) result += "/";
        result += "..";
    }

    const string aLeft = a.substr(aIt - a.begin());
    if (!aLeft.empty()) {
        if (!result.empty()) result += "/";
        result += aLeft;
    }
    return filenameCleanup(result);
}

bool V3Os::filenameIsRel(const string& filename) VL_PURE {
    return std::filesystem::path(filename).is_relative();
}

string V3Os::filenameSlashPath(const string& filename) VL_PURE {
    string slashedPath = filename;
    std::replace(slashedPath.begin(), slashedPath.end(), '\\', '/');
    return slashedPath;
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, char delim) {
    string line;
    std::getline(is, line, delim);
    return line;
}

//######################################################################
// Directory utilities

void V3Os::createDir(const string& dirname) { _mkdir(dirname.c_str()); }

void V3Os::filesystemFlush(const string& dirname) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
    if (int fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        if (fd > 0) ::close(fd);
    }
}

void V3Os::filesystemFlushBuildDir(const string& dirname) { filesystemFlush(dirname); }

void V3Os::unlinkRegexp(const string& dir, const string& regexp) {
    try {
        for (const auto& dirEntry : std::filesystem::directory_iterator(dir.c_str())) {
            if (VString::wildmatch(dirEntry.path().filename().string(), regexp.c_str())) {
                const string fullname = dir + "/" + dirEntry.path().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } catch (std::filesystem::filesystem_error const& ex) {}
}

//######################################################################
// METHODS (memory)

void V3Os::releaseMemory() {
#ifdef HAVE_TCMALLOC
    MallocExtension::instance()->ReleaseFreeMemory();
#endif
#ifdef HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    unsigned narenas = 0;
    size_t sz = sizeof(narenas);
    if (mallctl("arenas.narenas", &narenas, &sz, nullptr, 0)) {
        return;  // Failed to get number of arenas, give up
    }
    char buf[64];
    // Index equal to narenas represents all arenas
    (void)VL_SNPRINTF(buf, sizeof(buf), "arena.%u.purge", narenas);
    mallctl(buf, nullptr, nullptr, nullptr, 0);
#endif
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    const uint64_t result = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    return result;
}

string V3Os::trueRandom(size_t size) VL_MT_SAFE {
    string result(size, '\xFF');
    char* const data = const_cast<char*>(result.data());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
    const NTSTATUS hr = BCryptGenRandom(nullptr, reinterpret_cast<BYTE*>(data), size,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
    return result;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::timeUsecs() {
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    static constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    GetSystemTimeAsFileTime(&ft);
    const uint64_t us
        = ((static_cast<uint64_t>(ft.dwHighDateTime) << 32) + ft.dwLowDateTime + 5ULL) / 10ULL;
    return us - EPOCH_DIFFERENCE_USECS;
}

void V3Os::u_sleep(int64_t usec) { std::this_thread::sleep_for(std::chrono::microseconds(usec)); }

//######################################################################
// METHODS (sub command)

int V3Os::system(const string& command) {
    UINFO(1, "Running system: " << command);
    const int ret = ::system(command.c_str());
    if (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::strerror(errno));  // LCOV_EXCL_LINE
        return -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    const int exit_code = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_code);
    UASSERT(exit_code >= 0, "exit code must not be negative");
    return exit_code;
}

void V3Os::selfTest() {
#ifdef VL_DEBUG
    UASSERT_SELFTEST(filenameCleanup(""), "");
    UASSERT_SELFTEST(filenameCleanup("."), ".");
    UASSERT_SELFTEST(filenameCleanup(".."), "..");
    UASSERT_SELFTEST(filenameCleanup("/"), "/");
    UASSERT_SELFTEST(filenameCleanup("../"), "..");
    UASSERT_SELFTEST(filenameCleanup("//"), "/");
    UASSERT_SELFTEST(filenameCleanup("//."), "/.");
    UASSERT_SELFTEST(filenameCleanup("./"), ".");
    UASSERT_SELFTEST(filenameCleanup("././"), ".");
    UASSERT_SELFTEST(filenameCleanup(".///"), ".");
    UASSERT_SELFTEST(filenameCleanup("a"), "a");
    UASSERT_SELFTEST(filenameCleanup("a/"), "a");
    UASSERT_SELFTEST(filenameCleanup("a/b"), "a/b");
    UASSERT_SELFTEST(filenameCleanup("././//./a/b"), "a/b");
    UASSERT_SELFTEST(filenameCleanup(".//./a///"), "a");
    UASSERT_SELFTEST(filenameCleanup("///a/./b///."), "/a/./b/.");
    UASSERT_SELFTEST(filenameCleanup("aaa/bbb/ccc/"), "aaa/bbb/ccc");
    UASSERT_SELFTEST(filenameCleanup("./aaa/bbb/ccc/"), "aaa/bbb/ccc");
    UASSERT_SELFTEST(filenameCleanup("../aaa/bbb/ccc/"), "../aaa/bbb/ccc");
    UASSERT_SELFTEST(filenameDir("a.a/b.b/f.e"), "a.a/b.b");
    UASSERT_SELFTEST(filenameExt("a.a/b.b/f"), "");
    UASSERT_SELFTEST(filenameExt("a.a/b.b/f.e"), ".e");
    UASSERT_SELFTEST(filenameNonDirExt("a.a/b.b/f.e"), "f");
    UASSERT_SELFTEST(filenameRelativePath("/a/b", "/a/b"), ".");
    UASSERT_SELFTEST(filenameRelativePath("/a/b", "/a/b/c"), "..");
    UASSERT_SELFTEST(filenameRelativePath("/a/b", "/a/b/c/d"), "../..");
    UASSERT_SELFTEST(filenameRelativePath("/a/b/x", "/a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filenameRelativePath("/a/b/x/y", "/"), "a/b/x/y");
    UASSERT_SELFTEST(filenameRelativePath("/a/b/x/y", "/a/b"), "x/y");
    UASSERT_SELFTEST(filenameRelativePath("/a/b/x/y", "/a/q"), "../b/x/y");
    UASSERT_SELFTEST(filenameRelativePath("a/b", "a/b"), ".");
    UASSERT_SELFTEST(filenameRelativePath("a/b", "a/b/c"), "..");
    UASSERT_SELFTEST(filenameRelativePath("a/b", "a/b/c/d"), "../..");
    UASSERT_SELFTEST(filenameRelativePath("a/b/x", "a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filenameRelativePath("a/b/x/y", ""), "a/b/x/y");
    UASSERT_SELFTEST(filenameRelativePath("a/b/x/y", "a/b"), "x/y");
    UASSERT_SELFTEST(filenameRelativePath("a/b/x/y", "a/q"), "../b/x/y");
#endif
}
