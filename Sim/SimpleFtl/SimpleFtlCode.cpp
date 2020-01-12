#include <memory>
#include <functional>

#include "HostComm/CustomProtocol/CustomProtocolInterface.h"
#include "Nand/Hal/NandHal.h"
#include "SimpleFtl.h"

CustomProtocolInterface* _CustomProtocolInterface = nullptr;
SimpleFtl _SimpleFtl;

extern "C"
{
    void __declspec(dllexport) __stdcall Initialize(NandHal* nandHal, BufferHal* bufferHal, CustomProtocolInterface* customProtocolInterface)
    {
        _SimpleFtl.SetNandHal(nandHal);
        _SimpleFtl.SetBufferHal(bufferHal);
        _SimpleFtl.SetProtocol(customProtocolInterface);
        _CustomProtocolInterface = customProtocolInterface;
    }

    void __declspec(dllexport) __stdcall Execute()
    {
        if (_CustomProtocolInterface == nullptr)
        {
            return;
        }

        if (_CustomProtocolInterface->HasCommand() && !_SimpleFtl.IsProcessingCommand())
        {
            CustomProtocolCommand *command = _CustomProtocolInterface->GetCommand();
            _SimpleFtl.SubmitCustomProtocolCommand(command);
        }
        _SimpleFtl();
    }

    void __declspec(dllexport) __stdcall Shutdown()
    {
        
    }
};
