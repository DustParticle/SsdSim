// RomCode.cpp : Defines the exported functions for the DLL application.
//

#include <memory>

#include "Interfaces/CustomProtocolInterface.h"

extern "C"
{
    void __declspec(dllexport) __stdcall Execute()
    {
        if (CustomProtocolInterface::HasCommand())
        {
            CustomProtocolCommand *command = CustomProtocolInterface::GetCommand();
            switch (command->Command)
            {
                case CustomProtocolCommand::CommandCode::DownloadAndExecute:
                {
                    // Do download and execute
                    CustomProtocolInterface::SubmitResponse(command);
                } break;
            }
        }
    }
};
