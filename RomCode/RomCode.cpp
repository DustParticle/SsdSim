// RomCode.cpp : Defines the exported functions for the DLL application.
//

#include <memory>

#include "Interfaces/CustomProtocolInterface.h"

std::function<bool(std::string)> _SetExecuteFunc;
std::unique_ptr<CustomProtocolInterface> _CustomProtocolInterface = std::make_unique<CustomProtocolInterface>();

extern "C"
{
    void __declspec(dllexport) __stdcall Execute()
    {
        if (_CustomProtocolInterface->HasCommand())
        {
            CustomProtocolCommand *command = _CustomProtocolInterface->GetCommand();

            switch (command->Command)
            {
                case CustomProtocolCommand::Code::DownloadAndExecute:
                {
                    // Do download and execute
                    U32 size = sizeof(command->Payload.DownloadAndExecute.CodeName);
                    char *temp = new char[size];
                    for (U32 i = 0; i < size; i++)
                    {
                        temp[i] = command->Payload.DownloadAndExecute.CodeName[i];
                    }
                    std::string filename = temp;
                    delete[] temp;

                    _SetExecuteFunc(filename);
                    _CustomProtocolInterface->SubmitResponse(command);
                } break;
            }
        }
    }

    void __declspec(dllexport) __stdcall SetExecuteCallback(std::function<bool(std::string)> setExecuteFunc)
    {
        _SetExecuteFunc = setExecuteFunc;
    }
};
