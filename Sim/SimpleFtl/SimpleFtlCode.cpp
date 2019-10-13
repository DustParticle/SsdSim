#include <memory>
#include <functional>

#include "HostComm/CustomProtocol/CustomProtocolInterface.h"
#include "Nand/Hal/NandHal.h"
#include "SimpleFtl.h"

std::shared_ptr<CustomProtocolInterface> _CustomProtocolInterface = nullptr;
std::shared_ptr<BufferHal> _BufferHal = nullptr;
SimpleFtl _SimpleFtl;
std::future<void> _CustomProtocolInterfaceFuture;

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

        if (_CustomProtocolInterface->HasCommand() && !_SimpleFtl.IsProcessingCommand())
        {
            CustomProtocolCommand *command = _CustomProtocolInterface->GetCommand();
            _SimpleFtl.SubmitCustomProtocolCommand(command);
        }
        _SimpleFtl();
    }

    void __declspec(dllexport) __stdcall SetCustomProtocolIpcName(const std::string& protocolIpcName)
    {
        _CustomProtocolInterface = std::make_shared<CustomProtocolInterface>();
        _CustomProtocolInterface->Init(protocolIpcName.c_str(), _BufferHal.get());
        _SimpleFtl.SetProtocol(_CustomProtocolInterface.get());

        _CustomProtocolInterfaceFuture = std::async(std::launch::async, &CustomProtocolInterface::operator(), _CustomProtocolInterface);
    }

    void __declspec(dllexport) __stdcall Shutdown()
    {
        if (_CustomProtocolInterface == nullptr)
        {
            return;
        }

        _CustomProtocolInterface->Stop();
        _CustomProtocolInterfaceFuture.wait();
    }
};
