#include "FirmwareCore.h"

#include <windows.h>

// TODO: fix this declaration. Cannot add to header file because of causing JSON build error
HMODULE _DllInstance;

typedef void(__stdcall *fExecute)();
typedef void(__stdcall *fExecuteCallback)(std::function<bool(std::string)> callback);

using namespace std::placeholders;

FirmwareCore::FirmwareCore() : _Execute(nullptr)
{
    _DllInstance = 0;
}

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
    auto execute = (fExecute)GetProcAddress(_DllInstance, "Execute");
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

bool FirmwareCore::SetCallbackForRomCode()
{
    if (!_DllInstance)
    {
        return false;
    }

    // resolve function address here
    auto execute = (fExecuteCallback)GetProcAddress(_DllInstance, "SetExecuteCallback");
    if (!execute)
    {
        return false;
    }

    std::function<bool(std::string)> func = std::bind(&FirmwareCore::SetExecute, this, _1);
    execute(func);
    return true;
}