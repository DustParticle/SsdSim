#pragma once
#ifndef __FirmwareCore_h__
#define __FirmwareCore_h__

#include "BasicTypes.h"
#include "SimFrameworkBase/FrameworkThread.h"
#include "Nand/Hal/NandHal.h"
#include "Buffer/Hal/BufferHal.h"
#include "HostComm/CustomProtocol/CustomProtocolHal.h"

class FirmwareCore : public FrameworkThread
{
protected:
	virtual void Run() override;

public:
    FirmwareCore();
    bool SetExecute(std::string Filename);
    void Unload();
    void SetHalComponents(NandHal* nandHal, BufferHal* bufferHal, CustomProtocolHal* CustomProtocolHal);

private:
    void SwapExecute();

private:
    std::function<void()> _Execute;
    std::function<void()> _NewExecute;
    NandHal* _NandHal;
    BufferHal* _BufferHal;
    CustomProtocolHal* _CustomProtocolHal;
};

#endif