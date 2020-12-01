// RomCode.cpp : Defines the exported functions for the DLL application.
//

#include <memory>
#include <functional>

#include "HostComm/CustomProtocol/CustomProtocolHal.h"
#include "Nand/Hal/NandHal.h"

std::function<bool(std::string)> _SetExecuteFunc;
CustomProtocolHal* _CustomProtocolHal = nullptr;

extern "C"
{
    void __declspec(dllexport) __stdcall Initialize(NandHal* nandHal, BufferHal* bufferHal, CustomProtocolHal* CustomProtocolHal)
    {
        _CustomProtocolHal = CustomProtocolHal;
    }

    void __declspec(dllexport) __stdcall Execute()
    {
        assert(_CustomProtocolHal != nullptr);

        if (_CustomProtocolHal->HasCommand())
        {
            CustomProtocolCommand *command = _CustomProtocolHal->GetCommand();

            switch (command->Command)
            {
                case CustomProtocolCommand::Code::DownloadAndExecute:
                {
                    // Do download and execute
                    U32 size = sizeof(command->Descriptor.DownloadAndExecute.CodeName);
                    char *temp = new char[size];
                    for (U32 i = 0; i < size; i++)
                    {
                        temp[i] = command->Descriptor.DownloadAndExecute.CodeName[i];
                    }
                    std::string filename = temp;
                    delete[] temp;

                    _SetExecuteFunc(filename);
                    _CustomProtocolHal->SubmitResponse(command);
                } break;
            }
        }
    }

    void __declspec(dllexport) __stdcall SetExecuteCallback(std::function<bool(std::string)> setExecuteFunc)
    {
        _SetExecuteFunc = setExecuteFunc;
    }
};
