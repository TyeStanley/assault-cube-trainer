#include <iostream>
#include <vector>
#include <Windows.h>
#include <conio.h>
#include "proc.h"
#include "mem.h"

void static DisplayInterface(bool bHealth, bool bAmmo, bool bRecoil)
{
    system("cls");

    std::cout << "========================================\n";
    std::cout << "    AssaultCube Trainer\n";
    std::cout << "========================================\n\n";

    // Trainer Options
    std::cout << "[NUMPAD1] Invincibility -> " << (bHealth ? "ON" : "OFF") << " <-\n";
    std::cout << "[NUMPAD2] Unlimited Ammo -> " << (bAmmo ? "ON" : "OFF") << " <-\n";
    std::cout << "[NUMPAD3] No Recoil -> " << (bRecoil ? "ON" : "OFF") << " <-\n";
    std::cout << "[INSERT] Exit\n";
    std::cout << "========================================\n";
}

int main()
{
    // Initialize variables
    HANDLE hProcess = 0;
    uintptr_t moduleBase = 0, localPlayerPtr = 0, healthAddr = 0;
    bool bHealth = false, bAmmo = false, bRecoil = false;
    const int newValue = 9999;

    // Initialize last variables (for refreshing the interface)
    bool lastHealth = false, lastAmmo = false, lastRecoil = false;

    // Get ProcId of the target process
    DWORD procId = GetProcId(L"ac_client.exe");

    if (procId)
    {
        // Get Handle to Process
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);

        // Get module base address
        moduleBase = GetModuleBaseAddress(procId, L"ac_client.exe");

        // Resolve address
        localPlayerPtr = moduleBase + 0x10f4f4;

        // Resolve base address of the pointer chain
        healthAddr = FindDMAAddy(hProcess, localPlayerPtr, { 0xF8 });
    }
    else
    {
        std::cout << "Process not found, press enter to exit\n";
        getchar();
        return 0;
    }
    
    // Display initial interface
    DisplayInterface(bHealth, bAmmo, bRecoil);
    
    DWORD dwExit = 0;
    while (GetExitCodeProcess(hProcess, &dwExit) && dwExit == STILL_ACTIVE)
    {
        // Refresh the interface if any of the options have changed
        if (bHealth != lastHealth || bAmmo != lastAmmo || bRecoil != lastRecoil)
        {
            DisplayInterface(bHealth, bAmmo, bRecoil);
            lastHealth = bHealth;
            lastAmmo = bAmmo;
            lastRecoil = bRecoil;
        }
        
        // Toggle health boolean
        if (GetAsyncKeyState(VK_NUMPAD1) & 1)
        {
            bHealth = !bHealth;
        }
   
        // Unlimited ammo patch
        if (GetAsyncKeyState(VK_NUMPAD2) & 1)
        {
            bAmmo = !bAmmo;
   
            if (bAmmo)
            {
                // FF 06 = inc [esi] (increases ammo per shot)
                mem::PatchEx((BYTE*)(moduleBase + 0x637e9), (BYTE*)"\xFF\x06", 2, hProcess);
            }
            else
            {
                // FF 0E = dec [esi] (decreases ammo per shot)
                mem::PatchEx((BYTE*)(moduleBase + 0x637e9), (BYTE*)"\xFF\x0E", 2, hProcess);
            }
        }
   
        // No recoil nop
        if (GetAsyncKeyState(VK_NUMPAD3) & 1)
        {
            bRecoil = !bRecoil;
   
            if (bRecoil)
            {
                mem::NopEx((BYTE*)(moduleBase + 0x63786), 10, hProcess);
            }
   
            else
            {
                // 50 8D 4C 24 1C 51 8B CE FF D2; the original stack setup and call
                mem::PatchEx((BYTE*)(moduleBase + 0x63786), (BYTE*)"\x50\x8D\x4C\x24\x1C\x51\x8B\xCE\xFF\xD2", 10, hProcess);
            }
        }
        
        // Exit loop
        if (GetAsyncKeyState(VK_INSERT) & 1)
        {
            return 0;
        }
   
        // If health is true, patch the health address
        if (bHealth)
        {
            mem::PatchEx((BYTE*)healthAddr, (BYTE*)&newValue, sizeof(newValue), hProcess);
        }
   
        Sleep(10);
    }
   
    std::cout << "\nAssaultCube has closed. Press enter to exit...\n";
    getchar();
    return 0;
}

