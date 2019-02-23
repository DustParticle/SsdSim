#include <windows.h>

#include "FirmwareCore.h"

#include "InterfaceQueues.h"

FirmwareCore::FirmwareCore() : _Execute(nullptr)
{
    _InterfaceQueues = std::shared_ptr<InterfaceQueues>(new InterfaceQueues());
}

typedef void(__stdcall *f_execute)(std::shared_ptr<InterfaceQueues> InterfaceQueues);

bool FirmwareCore::SetExecute(std::string Filename)
{
    HINSTANCE hGetProcIDDLL = LoadLibrary(Filename.c_str());

    if (!hGetProcIDDLL)
    {
        return false;
    }

    // resolve function address here
    auto execute = (f_execute)GetProcAddress(hGetProcIDDLL, "Execute");
    if (!execute)
    {
        return false;
    }

    _Execute = execute;
    return true;
}

void FirmwareCore::Run()
{
    if (_Execute)
    {
        _Execute(this->_InterfaceQueues);
    }
}