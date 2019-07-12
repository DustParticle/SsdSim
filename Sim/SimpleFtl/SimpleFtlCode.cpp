// WriteRead.cpp : Defines the exported functions for the DLL application.
//

#include <memory>
#include <functional>

#include "HostComm/CustomProtocol/CustomProtocolInterface.h"
#include "Nand/Hal/NandHal.h"
#include "SimpleFtl.h"

std::unique_ptr<CustomProtocolInterface> _CustomProtocolInterface = nullptr;
std::shared_ptr<BufferHal> _BufferHal = nullptr;
SimpleFtl _SimpleFtl;

extern "C"
{
    void __declspec(dllexport) __stdcall Initialize(std::shared_ptr<NandHal> nandHal, std::shared_ptr<BufferHal> bufferHal)
    {
        _SimpleFtl.SetNandHal(nandHal.get());
        _SimpleFtl.SetBufferHal(bufferHal.get());
        _BufferHal = bufferHal;
    }

    void __declspec(dllexport) __stdcall Execute()
    {
        if (_CustomProtocolInterface == nullptr)
        {
            return;
        }

        _SimpleFtl();
    }

    void __declspec(dllexport) __stdcall SetCustomProtocolIpcName(const std::string& protocolIpcName)
    {
        _CustomProtocolInterface = std::make_unique<CustomProtocolInterface>(protocolIpcName.c_str(), _BufferHal.get());
        _SimpleFtl.SetProtocol(_CustomProtocolInterface.get());
    }
};
