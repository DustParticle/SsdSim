// RomCode.cpp : Defines the exported functions for the DLL application.
//

#include <memory>
#include <functional>

#include "HostComm/CustomProtocol/CustomProtocolInterface.h"
#include "Nand/Hal/NandHal.h"

std::function<bool(std::string)> _SetExecuteFunc;
std::unique_ptr<CustomProtocolInterface> _CustomProtocolInterface = nullptr;

extern "C"
{
    void __declspec(dllexport) __stdcall Execute()
    {
        if (_CustomProtocolInterface == nullptr)
        {
            throw "CustomProtocol is null";
        }

        if (_CustomProtocolInterface->HasCommand())
        {
            CustomProtocolCommand *command = _CustomProtocolInterface->GetCommand();

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
                    _CustomProtocolInterface->SubmitResponse(command);
                } break;
            }
        }
    }

    void __declspec(dllexport) __stdcall SetCustomProtocolIpcName(const std::string& protocolIpcName)
    {
        _CustomProtocolInterface = std::make_unique<CustomProtocolInterface>(protocolIpcName.c_str());
    }

    void __declspec(dllexport) __stdcall SetExecuteCallback(std::function<bool(std::string)> setExecuteFunc)
    {
        _SetExecuteFunc = setExecuteFunc;
    }
};
