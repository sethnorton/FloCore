#!/bin/env python
import os, sys, re, inspect, subprocess, winreg
from pygit2 import Repository, discover_repository, GitError
from pygit2 import GIT_SORT_TOPOLOGICAL, GIT_SORT_REVERSE, GIT_STATUS_CURRENT
from shutil import copyfile
from pathlib import Path


def getVersion():
    repo     = Repository(discover_repository(os.getcwd()))
    release  = "0_0_0"

    try:
        release = repo.describe(max_candidates_tags=0)
    except KeyError:
        print("No git tag on current commit, not a current release")
    except GitError:
        print("No git tags found, likely unsynced tags. Run: >git pull --tags")


    return release



def genVersion():
    repo     = Repository(discover_repository(os.getcwd()))
    branch   = repo.head.shorthand
    commitID = repo.head.target
    devBuild = False
    release  = getVersion()

    if repo.diff().patch:
        # There were uncommitted changes, should mark as -dev
        print("Uncommitted changes, dev build")
        devBuild = True



    # Change release based on whether or not this is a valid release

    if devBuild:
        release = "0_0_0"
        branch += "-dirty"
    else:
        release = release.strip().replace(".", "_").replace("v", "")

    print("{0} - {1}, Dev Build = {2}, Release Tag = {3}".format(branch, commitID, devBuild, release))

    with open("src/VfVersion.h", "w") as vFile:
        print("Generating VfVersion.h file")


        vFile.write("/*THIS IS AN AUTOMATICALLY GENERATED HEADER - Please do not change*/\n")
        vFile.write("/*This file was created by scons and defined by SConstruct:genVersion()*/\n")
        vFile.write("#pragma once\n")
        vFile.write("#include <cstdint>\n")
        vFile.write("#include <string>\n\n")

        vFile.write("#define CEVERSION_TAG {0}\n".format(release))
        vFile.write("#define CEVERSION_COMMIT {0}\n".format(commitID))
        vFile.write("#define CEVERSION_BRANCH {0}\n\n".format(branch))

        vFile.write("#define CONCAT(A, B) A ## B\n")
        vFile.write("#define CONCAT_EXPAND(A, B) CONCAT(A, B)\n")

        vFile.write("#define TO_STRING(a) #a\n")
        vFile.write("#define TO_STRING_EXPAND(a) TO_STRING(a)\n")

        vFile.write("// Takes a version number in the M_m_b format, and converts it first to a string, then takes out the delimited numbers\n")
        vFile.write("// to convert to an actual number. This is slow and painful to look at. Should be upgraded in the future.\n")
        vFile.write("#define VERSION_MAJOR(target) std::stoi(std::string(TO_STRING(target)).substr(0, std::string(TO_STRING(target)).find_first_of(\"_\")))\n")
        vFile.write("#define VERSION_MINOR(target) std::stoi(std::string(TO_STRING(target)).substr(std::string(TO_STRING(target)).find_first_of(\"_\") + 1, std::string(TO_STRING(target)).find_last_of(\"_\") - std::string(TO_STRING(target)).find_first_of(\"_\") - 1))\n")
        vFile.write("#define VERSION_PATCH(target) std::stoi(std::string(TO_STRING(target)).substr(std::string(TO_STRING(target)).find_last_of(\"_\") + 1))\n")

        vFile.write("#define VERSIONED(a, b) CONCAT_EXPAND(a, b)\n")
        vFile.write("#define VERSION_LESS(v, target) v.major == VERSION_MAJOR(target) ?          \\\n")
        vFile.write("                                    v.minor == VERSION_MINOR(target) ?      \\\n")
        vFile.write("                                        v.bug < VERSION_PATCH(target) :     \\\n")
        vFile.write("                                        v.minor < VERSION_MINOR(target)     \\\n")
        vFile.write("                                    : v.major < VERSION_MAJOR(target)       \n")
        vFile.write("#define VERSION_GREATER(v, target) v.major == VERSION_MAJOR(target) ?          \\\n")
        vFile.write("                                       v.minor == VERSION_MINOR(target) ?      \\\n")
        vFile.write("                                           v.bug > VERSION_PATCH(target) :     \\\n")
        vFile.write("                                           v.minor > VERSION_MINOR(target)     \\\n")
        vFile.write("                                       : v.major > VERSION_MAJOR(target)       \n")
        vFile.write("#define VERSION_EQUAL(v, target) ((v.major == VERSION_MAJOR(target)) && (v.minor == VERSION_MINOR(target)) && (v.bug == VERSION_PATCH(target)))\n")

        vFile.write("#define VERSION_SET(v, target) v.major = VERSION_MAJOR(target),             \\\n")
        vFile.write("                               v.minor = VERSION_MINOR(target),             \\\n")
        vFile.write("                               v.bug = VERSION_PATCH(target)\n")
        vFile.write("\n")

        vFile.write("class version {\n")
        vFile.write("public:\n")
        vFile.write("    uint8_t major;\n")
        vFile.write("    uint8_t minor;\n")
        vFile.write("    uint8_t bug;\n")
        vFile.write("\n")
        vFile.write("    version() : major(0), minor(0), bug(0) {}\n")
        vFile.write("    version(std::string&& ver) : major(0), minor(0), bug(0) {\n")
        vFile.write("        const auto endMajor = ver.find_first_of(\"._\");\n")
        vFile.write("        const auto endMinor = ver.find_last_of(\"._\");\n")

        vFile.write("        if (endMajor != std::string::npos) {\n")
        vFile.write("            major = std::stoi(ver.substr(0, endMajor));\n")

        vFile.write("            if (endMinor != std::string::npos) {\n")
        vFile.write("                if (endMajor == endMinor) {\n")
        vFile.write("                    minor = std::stoi(ver.substr(endMajor + 1));\n")
        vFile.write("                    bug   = 0;\n")
        vFile.write("                }\n")
        vFile.write("                else {\n")
        vFile.write("                    minor = std::stoi(ver.substr(endMajor + 1, (endMinor - endMajor - 1)));\n")
        vFile.write("                    bug   = std::stoi(ver.substr(endMinor + 1));\n")
        vFile.write("                }\n")
        vFile.write("            }\n")
        vFile.write("        }\n")
        vFile.write("    }\n")
        vFile.write("    version(const uint8_t majorVer, const uint8_t minorVer, const uint8_t bugVer) : major(majorVer), minor(minorVer), bug(bugVer) {}\n")
        vFile.write("};\n")

    with open("res/version.rc2", "w") as vFile:
        print("Generating res/version.rc2 file")
        dotRelease = release.strip().replace("_",".") + ".1"


        vFile.write("/////////////////////////////////////////////////////////////////////////////\n")
        vFile.write("//\n")
        vFile.write("// Version\n")
        vFile.write("//\n")

        vFile.write("VS_VERSION_INFO VERSIONINFO\n")
        vFile.write(" FILEVERSION {0}\n".format(dotRelease))
        vFile.write(" PRODUCTVERSION {0}\n".format(dotRelease))
        vFile.write(" FILEFLAGSMASK 0x3fL\n")
        vFile.write("#ifdef _DEBUG\n")
        vFile.write(" FILEFLAGS 0x1L\n")
        vFile.write("#else\n")
        vFile.write(" FILEFLAGS 0x0L\n")
        vFile.write("#endif\n")
        vFile.write(" FILEOS 0x40004L\n")   # Win 10
        vFile.write(" FILETYPE 0x1L\n")     # App
        vFile.write(" FILESUBTYPE 0x0L\n")
        vFile.write("BEGIN\n")
        vFile.write("    BLOCK \"StringFileInfo\"\n")
        vFile.write("    BEGIN\n")
        vFile.write("        BLOCK \"040904b0\"\n")
        vFile.write("        BEGIN\n")
        vFile.write("            VALUE \"CompanyName\", \"VulcanForms\"\n")
        vFile.write("            VALUE \"FileDescription\", \"Configuration Editor Executable\"\n")
        vFile.write("            VALUE \"FileVersion\", \"{0}\"\n".format(dotRelease))
        vFile.write("            VALUE \"LegalCopyright\", \"Copyright (C) 2020\"\n")
        vFile.write("            VALUE \"OriginalFilename\", \"FloCore.exe\"\n")
        vFile.write("            VALUE \"ProductName\", \"Configuration Editor\"\n")
        vFile.write("            VALUE \"ProductVersion\", \"{0}\"\n".format(dotRelease))
        vFile.write("        END\n")
        vFile.write("    END\n")
        vFile.write("    BLOCK \"VarFileInfo\"\n")
        vFile.write("    BEGIN\n")
        vFile.write("        VALUE \"Translation\", 0x409, 1200\n")
        vFile.write("    END\n")
        vFile.write("END\n")


def isLineStartDecl(line):
    count = 0

    if line:
        for c in line.strip():
            if c.isspace() or c == '-':
                if c == '-':
                    count += 1

                if count >= 3:
                    break
            else:
                return ""

        if count >= 3:
            strippedLine = line.replace('-', '').strip()
            sType = strippedLine[:strippedLine.find(':')]
            name  = strippedLine[strippedLine.find(':') + 1:]

            if sType == "vert":
                name = "s_vs" + name
            elif sType == "geom":
                name = "s_gs" + name
            elif sType == "frag":
                name = "s_fs" + name

            return name

    return ""


AppName = 'FloCore'
nArgs = len(sys.argv)

CFG='dbg'
ARCH='x64'
PROC='x64'
VTK='static'

for arg in sys.argv:
    if "arch=" in arg:
        if "x32" in arg:
            ARCH='x32'
            PROC='x86'
            print("Producing 32 bit")
        else:
            ARCH='x64'
            PROC='x64'
            print("Producing 64 bit")
    elif "cfg=" in arg:
        if "rel" in arg:
            CFG='rel'
        else:
            CFG='dbg'
    elif "vtk=" in arg:
        if "dynamic" in arg:
            VTK='dynamic'
        else:
            VTK='static'

print("Building arch:", ARCH, " and cfg:", CFG, sep='')

EXENM = AppName + ".exe"
PDBNM = AppName + ".pdb"

genVersion()


MSVSVER  = '14.24.28314'
WKITVER  = '10.0.18362.0'

MSVS19RT = 'C:/MSVS/2019/Professional/'
WKITRT   = 'C:/Windows Kits/10'

# Set the windows kits root from Windows registry
rootPath        = r"SOFTWARE\WOW6432Node\Microsoft\Windows Kits\Installed Roots";
regLocalMachine = winreg.ConnectRegistry(None, winreg.HKEY_LOCAL_MACHINE)

installedKeys = winreg.OpenKey(regLocalMachine, rootPath)

try:
    val    = winreg.QueryValueEx(installedKeys, "KitsRoot10")
    WKITRT = val[0] # Pull out path, second value in tuple is the data type

# There are multiple registry keys that point to the windows kits so we have a backup check
except EnvironmentError:
    backupPath = r"SOFTWARE\WOW6432Node\Microsoft\Microsoft SDKs\Windows\v10.0"
    print("Could not find", rootPath, "in registry. Trying", backupPath)

    try:
        installedKeys = winreg.OpenKey(regLocalMachine, backupPath)
        val    = winreg.QueryValueEx(installedKeys, "InstallationFolder")
        WKITRT = val[0]
    except EnvironmentError:
        print("Could not find", backupPath, "in registry either. Defaulting to", WKITRT)


MSVS19    = MSVS19RT + 'VC/Tools/MSVC/' + MSVSVER + '/'
WKIT19LIB = WKITRT   + 'lib/'     + WKITVER + '/'
WKIT19INC = WKITRT   + 'include/' + WKITVER + '/'
WKIT19BIN = WKITRT   + 'bin/'     + WKITVER + '/' + PROC + '/'

RCINC = ' -i "' + WKIT19INC + 'um" -i "' + WKIT19INC + 'shared" '

WINLIBS = [MSVS19    + "ATLMFC/lib/" + PROC + ";", MSVS19    + "lib/" + PROC + ";", \
           WKIT19LIB + "ucrt/"       + PROC + ";", WKIT19LIB + "um/"  + PROC + ";"]
WINC = [MSVS19    + "ATLMFC/include;", MSVS19 + "include;", \
        WKIT19LIB + "ucrt;", WKIT19LIB + "shared;", WKIT19LIB + "winrt;", \
        WKIT19LIB + "um;",   WKIT19LIB + "cppwinrt;"]



#Compile resource file
print("Compiling program resources")
print("Calling " + WKIT19BIN + 'rc.exe')
#os.system('"' + WKIT19BIN + 'rc.exe"' + RCINC + '/nologo /fo res/FloCore.res res/FloCore.rc')
subprocess.call([WKIT19BIN + 'rc.exe',          \
                 '/i', WKIT19INC + 'um',        \
                 '/i', WKIT19INC + 'shared',    \
                 '/nologo',                     \
                 '/fo', 'res/FloCore.res', \
                 'res/FloCore.rc'])

AFLGS = ['']

if ARCH == 'x32':
    AFLGS = AFLGS + ['/machine:x86']
else:
    AFLGS = AFLGS + ['/machine:x64']


if CFG == 'dbg':
    AFLGS = AFLGS + ['/DEBUG','/INCREMENTAL:NO']
else:
    AFLGS = AFLGS + ['/DEBUG', '/INCREMENTAL:NO', '/LTCG'] # /LTCG allows link-time optimization

WLNM = "/DWINDOWS_LEAN_AND_MEAN /DWIN32_LEAN_AND_MEAN"

CINC = ['src',
        'src/tools',
        'src/components']


if CFG == 'dbg':
    MFLG = ['/MDd', '/Zi', '/Od', '/fp:precise', '/W3'] #, '/D_ITERATOR_DEBUG_LEVEL=1'] # For faster debug std::vectors
    MLDF = ['/DEBUG', '/D_DEBUG']
    SYSG = ['/SUBSYSTEM:CONSOLE']
else:
    MFLG = ['/MD', '/Zi', '/O2', '/GL', '/fp:precise'] # /GL allows link-time optimization
    MLDF = ['/DNDEBUG']
    SYSG = ['/SUBSYSTEM:WINDOWS']

SCOPE = '/Zc:forScope,wchar_t,auto,inline,rvalueCast,strictStrings,implicitNoexcept,threadSafeInit,ternary'
DEFS  = ['/DWIN32','/D_WIN32',WLNM,'/D_WINDOWS',MLDF]
FLAGS = [MFLG,'/GR','/arch:AVX2','/MP','/fp:precise','/std:c++latest','/utf-8','/W3','/WX-','/EHsc', DEFS, SCOPE]

#LFLAGS  = [SYSG, '/NOLOGO', '/INCREMENTAL:NO', AFLGS]
LFLAGS  = [SYSG, '/NOLOGO', AFLGS]
LPATH   = [WINLIBS]
WINLIST = ['dbghelp','kernel32','user32','gdi32','winspool','shell32','Shlwapi','ole32','oleaut32','uuid','comdlg32','advapi32','wsock32']
LLIST = [WINLIST]

print("Cloning Environment")

env = Environment()

#envN = env.Clone()
#print("Test3")
#envN.Export('envN CFG ARCH')
#envN.SConscript('ext/nanogui/src/SConscript', variant_dir='bin/obj/nanogui', dublicate=0)

print("Setting Environment Variables")
env.Append(CPPPATH=WINC+CINC)
env.Append(CXXFLAGS=FLAGS)
env.Append(CFLAGS=FLAGS)
env.Replace(LIBPATH=LPATH)
env.Replace(LIBS=LLIST)
env.Append(LINKFLAGS=LFLAGS)

# Variant Dir tells compiler to move all *.obj to bin/obj
# to do compilation there and not clog up src directories
env.VariantDir('bin/obj',                           'src',                          duplicate=0)
env.VariantDir('bin/obj/tools',                     'src/tools',                    duplicate=0)

files = [Glob('bin/obj/*.cpp'),
         Glob('bin/obj/tools/*.cpp'),
         'ext/pugixml/pugi/pugixml.cpp',
         'res/FloCore.res']

print("Building")
env.Program('bin/' + EXENM, files)
