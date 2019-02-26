#include "FirmwareCore.h"

#include <windows.h>

// TODO: fix this declaration. Cannot add to header file because of causing JSON build error
HMODULE _DllInstance;

FirmwareCore::FirmwareCore() : _Execute(nullptr)
{
    _DllInstance = 0;
}

typedef void(__stdcall *f_execute)();

bool FirmwareCore::SetExecute(std::string Filename)
{
    if (_DllInstance)
    {
        FreeLibrary(_DllInstance);
    }

    _DllInstance = LoadLibrary(Filename.c_str());

    if (!_DllInstance)
    {
        return false;
    }

    // resolve function address here
    auto execute = (f_execute)GetProcAddress(_DllInstance, "Execute");
    if (!execute)
    {
        FreeLibrary(_DllInstance);
        return false;
    }

    _Execute = execute;
    return true;
}

void FirmwareCore::Run()
{
    if (_Execute)
    {
        _Execute();
    }
}

void FirmwareCore::Unload()
{
    if (_DllInstance)
    {
        FreeLibrary(_DllInstance);
    }
}