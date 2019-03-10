#include "FirmwareCore.h"

#include <windows.h>

// TODO: fix this declaration. Cannot add to header file because of causing JSON build error
HMODULE _DllInstance;
HMODULE _NewDllInstance;

typedef void(__stdcall *fExecute)();
typedef void(__stdcall *fExecuteCallback)(std::function<bool(std::string)> callback);

FirmwareCore::FirmwareCore() : _Execute(nullptr), _NewExecute(nullptr)
{
    _DllInstance = NULL;
    _NewDllInstance = NULL;
}

bool FirmwareCore::SetExecute(std::string Filename)
{
    _NewDllInstance = LoadLibrary(Filename.c_str());

    if (!_NewDllInstance)
    {
        return false;
    }

    // resolve function address here
    auto execute = (fExecute)GetProcAddress(_NewDllInstance, "Execute");
    if (!execute)
    {
        FreeLibrary(_NewDllInstance);
        return false;
    }

    // search the SetExecuteCall and set
    auto setExecuteCallback = (fExecuteCallback)GetProcAddress(_NewDllInstance, "SetExecuteCallback");
    if (setExecuteCallback)
    {
        std::function<bool(std::string)> func = std::bind(&FirmwareCore::SetExecute, this, std::placeholders::_1);
        setExecuteCallback(func);
    }

    _NewExecute = execute;
    return true;
}

void FirmwareCore::Run()
{
    if (_NewExecute)
    {
        SwapExecute();
    }

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

    if (_NewDllInstance)
    {
        FreeLibrary(_NewDllInstance);
    }
}

void FirmwareCore::SwapExecute()
{
    if (_DllInstance)
    {
        // Wait for the current code completely
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        FreeLibrary(_DllInstance);
    }

    _Execute = _NewExecute;
    _NewExecute = nullptr;

    _DllInstance = _NewDllInstance;
    _NewDllInstance = NULL;
}