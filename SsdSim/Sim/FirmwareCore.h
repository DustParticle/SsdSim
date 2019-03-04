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
    void Init(std::string Filename);
    bool SetExecute(std::string Filename);
    void Unload();

private:
    void SwapExecute();

private:
    std::function<void()> _Execute;
    std::function<void()> _NewExecute;
};

#endif