// RomCode.cpp : Defines the exported functions for the DLL application.
//

#include <chrono>

#include "Interfaces/CustomProtocolInterface.h"

using namespace std::chrono;
milliseconds startMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

static milliseconds _StartMs;
static U32 _NopCount;

extern "C"
{
    void __declspec(dllexport) __stdcall Execute()
    {
        if (CustomProtocolInterface::HasCommand())
        {
            CustomProtocolCommand *command = CustomProtocolInterface::GetCommand();

            switch (command->Command)
            {
                case CustomProtocolCommand::CommandCode::BenchmarkStart:    // No response
                {
                    _StartMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
                    _NopCount = 0;
                } break;

                case CustomProtocolCommand::CommandCode::BenchmarkEnd:
                {
                    milliseconds endMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
                    command->Payload.BenchmarkPayload.Response.Duration = (endMs.count() - _StartMs.count());
                    command->Payload.BenchmarkPayload.Response.NopCount = _NopCount;
                } break;

                case CustomProtocolCommand::CommandCode::Nop:      // No response
                {
                    ++_NopCount;
                } break;
            }

            CustomProtocolInterface::SubmitResponse(command);
        }
    }
};
