#pragma once
#ifndef __FirmwareCore_h__
#define __FirmwareCore_h__

#include "FrameworkThread.h"
#include "InterfaceQueues.h"

class FirmwareCore : public FrameworkThread
{
protected:
	virtual void Run() override;

public:
    FirmwareCore();
    bool SetExecute(std::string Filename);

public:
    std::shared_ptr<InterfaceQueues> _InterfaceQueues;

private:
    std::function<void(std::shared_ptr<InterfaceQueues>)> _Execute;
};

#endif