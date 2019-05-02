#pragma once
#ifndef __FirmwareCore_h__
#define __FirmwareCore_h__

#include "BasicTypes.h"
#include "SimFrameworkBase/FrameworkThread.h"
#include "Nand/Hal/NandHal.h"

class FirmwareCore : public FrameworkThread
{
protected:
	virtual void Run() override;

public:
    FirmwareCore();
    bool SetExecute(std::string Filename);
    void Unload();
    void LinkNandHal(std::shared_ptr<NandHal> nandHal);
    void SetIpcNames(const std::string& customProtocolIpcName);

private:
    void SwapExecute();

private:
    std::function<void()> _Execute;
    std::function<void()> _NewExecute;
    std::shared_ptr<NandHal> _NandHal;
    std::string _CustomProtocolIpcName;
};

#endif