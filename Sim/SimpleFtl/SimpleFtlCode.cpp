#include <memory>
#include <functional>

#include "HostComm/CustomProtocol/CustomProtocolHal.h"
#include "Nand/Hal/NandHal.h"
#include "SimpleFtl.h"

CustomProtocolHal* _CustomProtocolHal = nullptr;
SimpleFtl _SimpleFtl;

extern "C"
{
    void __declspec(dllexport) __stdcall Initialize(NandHal* nandHal, BufferHal* bufferHal, CustomProtocolHal* CustomProtocolHal)
    {
        _SimpleFtl.SetNandHal(nandHal);
        _SimpleFtl.SetBufferHal(bufferHal);
        _SimpleFtl.SetProtocol(CustomProtocolHal);
        _CustomProtocolHal = CustomProtocolHal;
    }

    void __declspec(dllexport) __stdcall Execute()
    {
        if (_CustomProtocolHal == nullptr)
        {
            return;
        }

        if (_CustomProtocolHal->HasCommand() && !_SimpleFtl.IsProcessingCommand())
        {
            CustomProtocolCommand *command = _CustomProtocolHal->GetCommand();
            _SimpleFtl.SubmitCustomProtocolCommand(command);
        }
        _SimpleFtl();
    }

    void __declspec(dllexport) __stdcall Shutdown()
    {
        _SimpleFtl.Shutdown();
    }
};
