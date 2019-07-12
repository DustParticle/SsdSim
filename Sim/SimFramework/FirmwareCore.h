#pragma once
#ifndef __FirmwareCore_h__
#define __FirmwareCore_h__

#include "BasicTypes.h"
#include "SimFrameworkBase/FrameworkThread.h"
#include "Nand/Hal/NandHal.h"
#include "Buffer/Hal/BufferHal.h"

class FirmwareCore : public FrameworkThread
{
protected:
	virtual void Run() override;

public:
    FirmwareCore();
    bool SetExecute(std::string Filename);
    void Unload();
    void SetHalComponents(std::shared_ptr<NandHal> nandHal, std::shared_ptr<BufferHal> bufferHal);
    void SetIpcNames(const std::string& customProtocolIpcName);

private:
    void SwapExecute();

private:
    std::function<void()> _Execute;
    std::function<void()> _NewExecute;
    std::shared_ptr<NandHal> _NandHal;
    std::shared_ptr<BufferHal> _BufferHal;
    std::string _CustomProtocolIpcName;
};

#endif