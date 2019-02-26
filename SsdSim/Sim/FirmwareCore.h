#pragma once
#ifndef __FirmwareCore_h__
#define __FirmwareCore_h__

#include "FrameworkThread.h"

class FirmwareCore : public FrameworkThread
{
protected:
	virtual void Run() override;

public:
    FirmwareCore();
    bool SetExecute(std::string Filename);
    void Unload();

private:
    std::function<void()> _Execute;
};

#endif