/* AutoSKSE
* By ahmouse (and StackOverflow :P)
* 
* Allows SKSE to be loaded automatically without needing a custom launcher.
* 
* The oldrim approach to loading SKSE automatically was simple:
* Skyrim's launcher simply launches the program named "Skyrim.exe" no matter what,
* so simply rename the SKSE loader to Skyrim.exe, and rename Skyrim.exe to something else.
* Then, in skse.ini make sure it knows the new name of the game executable.
* Pretty simple, right?
*
* With Skyrim SE, though, things changed.
* Now, the game executable must be named SkyrimSE.exe, or things start to break.
* With this approach, I try to have my cake and eat it too:
* I want AutoSKSE to be named SkyrimSE.exe when the launcher checks,
* but I want the game executable to be named SkyrimSE.exe when SKSE checks.
*
* We can achieve this by simply naming ourselves SkyrimSE.exe,
* and the game executable something else. Once our process is running,
* we rename it to a temporary name, and change the actual game back to SkyrimSE.exe.
* Once the game exits we can undo the name changes so that we are named SkyrimSE.exe
* and the actual game is named RealSkyrimSE.exe
*
* Rinse and repeat.
*/

#include <iostream>
#include <cstdio>
#include <string>

#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>

/* TODO: Switch to symbolic or hard links if they're more reliable */

int main() {

    bool errorCode = false;
    int result = 0;

    PROCESSENTRY32 entry;

    STARTUPINFOA info;
    PROCESS_INFORMATION processInfo;

    entry.dwSize = sizeof(PROCESSENTRY32);
    info.cb = sizeof(info);

    //Rename ourself to a temp name, and name the actual game SkyrimSE.exe
    result = rename("SkyrimSE.exe", "AutoSKSE.exe");
    result = rename("RealSkyrimSE.exe", "SkyrimSE.exe");

    errorCode = CreateProcessA("skse64_loader.exe",NULL, NULL, 
        NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &info, &processInfo);

    if (errorCode)
    {
        // For some reason SKSE can't read the game exe without this sleep
        // Only 1ms is needed, but I put 100ms for safety since its negligible
        // It's odd that closing the handle on the SKSE loader
        // could prevent it from launching the game.
        Sleep(100);

        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
    }
    //TODO: Error handling;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry) == TRUE)
    {
        bool skippedFirst = false;
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            // A little hacky, but necessary
            // AutoSKSE and Skyrim itself are both listed as SkyrimSE.exe in the process list
            // so when we open the SkyrimSE.exe process, we end up opening our own process.
            // However, it seems if we skip the first SkyrimSE.exe, it will open
            // the actual Skyrim consistently. This leads me to believe that
            // given two processes with the same name, this function returns
            // them in the order they were created.
            // If that's the case, this should consistently work on all systems.
            
            if ((_tcsicmp(entry.szExeFile, _T("SkyrimSE.exe")) == 0) && skippedFirst)
            {
                HANDLE skyrimProcess = OpenProcess(SYNCHRONIZE, FALSE, entry.th32ProcessID);

                WaitForSingleObject(skyrimProcess, INFINITE);

                CloseHandle(skyrimProcess);
                break;
            }

            //Found first SkyrimSE.exe, update flag
            if (_tcsicmp(entry.szExeFile, _T("SkyrimSE.exe")) == 0)
            {
                skippedFirst = true;
            }
        }
    }

    CloseHandle(snapshot);

    //Revert name changes
    result = rename("SkyrimSE.exe", "RealSkyrimSE.exe");
    result = rename("AutoSKSE.exe", "SkyrimSE.exe");

    if (result != 0) {
        // TODO: Error handling 
    }
    
    return 0;
}
