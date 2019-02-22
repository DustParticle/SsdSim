#include "FirmwareCore.h"

void FirmwareCore::SetExecute(std::function<void()> execute)
{
    _Execute = execute;
}

void FirmwareCore::Run()
{
    _Execute();
}