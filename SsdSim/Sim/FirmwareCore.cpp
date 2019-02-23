#include "FirmwareCore.h"

FirmwareCore::FirmwareCore()
{
    _InterfaceQueues = std::shared_ptr<InterfaceQueues>(new InterfaceQueues());
}

void FirmwareCore::SetExecute(std::function<void(std::shared_ptr<InterfaceQueues>)> execute)
{
    _Execute = execute;
}

void FirmwareCore::Run()
{
    _Execute(this->_InterfaceQueues);
}