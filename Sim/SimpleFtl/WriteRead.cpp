// WriteRead.cpp : Defines the exported functions for the DLL application.
//

#include <memory>
#include <functional>

#include "Translation.h"
#include "HostComm/CustomProtocol/CustomProtocolInterface.h"
#include "Nand/Hal/NandHal.h"

std::unique_ptr<CustomProtocolInterface> _CustomProtocolInterface = std::make_unique<CustomProtocolInterface>();
std::shared_ptr<NandHal> _NandHal;
NandHal::Geometry _Geometry;
U32 _LbaCount;
U32 _LbasPerPage;
U32 _SectorSizeInBytes = 512;
U8 _SectorsPerPage;

extern "C"
{
    void CreateNandCommand(U32 lba, U8 *buffer, NandHal::CommandDesc::Op operation, NandHal::CommandDesc &commandDesc)
    {
        commandDesc.Operation = operation;
        commandDesc.Buffer = buffer;

        //Translate from LBA to NAND address
        SimpleFtlTranslation::LbaToNandAddress(_Geometry, lba, commandDesc);
    }

    U32 CalculateTransferPageCount(U32 sectorCount)
    {
        U32 transferPageCount = (sectorCount / _LbasPerPage);
        transferPageCount += (0 < (sectorCount % _LbasPerPage) ? 1 : 0);
        return transferPageCount;
    }

    void WriteToNand(CustomProtocolCommand *command)
    {
        NandHal::CommandDesc commandDesc;
        U32 lba = command->Descriptor.SimpleFtlPayload.Lba;
        U32 sectorCount = command->Descriptor.SimpleFtlPayload.SectorCount;
        U32 bufferSizeInBytes = 0;
        auto buffer = _CustomProtocolInterface->GetBuffer(command, bufferSizeInBytes);
        auto writeBuffer = std::make_unique<U8[]>(_Geometry._BytesPerPage);
        U32 bufferOffset = 0;

        //Calculate number of pages to loop write per page
        U32 writePageCount = CalculateTransferPageCount(sectorCount);
        for (U32 count(0); count < writePageCount; ++count)
        {
            if (bufferOffset + _Geometry._BytesPerPage <= bufferSizeInBytes)
            {
                memcpy_s(writeBuffer.get(), _Geometry._BytesPerPage, buffer + bufferOffset, _Geometry._BytesPerPage);
            }
            else
            {
                auto remainingSizeInBytes = bufferSizeInBytes - bufferOffset;
                memcpy_s(writeBuffer.get(), remainingSizeInBytes, buffer + bufferOffset, remainingSizeInBytes);
            }

            //Create Nand command
            CreateNandCommand(lba, writeBuffer.get(), NandHal::CommandDesc::Op::WRITE, commandDesc);

            //Submit to the NandHal
            _NandHal->QueueCommand(commandDesc);

            //Wait for nand command completed
            while (!_NandHal->IsCommandQueueEmpty());

            //Update next lba and buffer offset
            lba += _LbasPerPage;
            bufferOffset += _Geometry._BytesPerPage;
        }
    }

    void ReadFromNand(CustomProtocolCommand *command)
    {
        NandHal::CommandDesc commandDesc;
        U32 lba = command->Descriptor.SimpleFtlPayload.Lba;
        U32 sectorCount = command->Descriptor.SimpleFtlPayload.SectorCount;
        U32 bufferSizeInBytes = 0;
        auto buffer = _CustomProtocolInterface->GetBuffer(command, bufferSizeInBytes);
        auto readBuffer = std::make_unique<U8[]>(_Geometry._BytesPerPage);
        U32 bufferOffset = 0;

        //Calculate number of pages to loop write per page
        U32 readPageCount = CalculateTransferPageCount(sectorCount);
        for (U32 count(0); count < readPageCount; ++count)
        {
            //Create Nand command
            CreateNandCommand(lba, readBuffer.get(), NandHal::CommandDesc::Op::READ, commandDesc);

            //Submit to the NandHal
            _NandHal->QueueCommand(commandDesc);

            //Wait for nand command completed
            while (!_NandHal->IsCommandQueueEmpty());

            // Copy read page buffer to buffer
            if (bufferOffset + _Geometry._BytesPerPage <= bufferSizeInBytes)
            {
                memcpy_s(buffer + bufferOffset, _Geometry._BytesPerPage, readBuffer.get(), _Geometry._BytesPerPage);
            }
            else
            {
                auto remainingSizeInBytes = bufferSizeInBytes - bufferOffset;
                memcpy_s(buffer + bufferOffset, remainingSizeInBytes, readBuffer.get(), remainingSizeInBytes);
            }

            //Update next lba and buffer offset
            lba += _LbasPerPage;
            bufferOffset += _Geometry._BytesPerPage;
        }
    }

    void __declspec(dllexport) __stdcall Initialize(std::shared_ptr<NandHal> nandHal)
    {
        _NandHal = nandHal;
        _Geometry = nandHal->GetGeometry();
        _LbasPerPage = _Geometry._BytesPerPage >> 9;
        _LbaCount = _Geometry._ChannelCount * _Geometry._DevicesPerChannel * _Geometry._BlocksPerDevice * _Geometry._PagesPerBlock * _LbasPerPage;
		_SectorsPerPage = _Geometry._BytesPerPage / _SectorSizeInBytes;
    }

    void __declspec(dllexport) __stdcall Execute()
    {
        if (_CustomProtocolInterface->HasCommand())
        {
            CustomProtocolCommand *command = _CustomProtocolInterface->GetCommand();

            switch (command->Command)
            {
                case CustomProtocolCommand::Code::Write:
                {
                    //Write command to nand
                    WriteToNand(command);

                    _CustomProtocolInterface->SubmitResponse(command);
                } break;
				case CustomProtocolCommand::Code::LoopbackWrite:
				{
					_CustomProtocolInterface->SubmitResponse(command);
				} break;
                case CustomProtocolCommand::Code::Read:
                {
                    //Read command from nand
                    ReadFromNand(command);

                    _CustomProtocolInterface->SubmitResponse(command);
                } break;
				case CustomProtocolCommand::Code::LoopbackRead:
				{
					_CustomProtocolInterface->SubmitResponse(command);
				} break;
                case CustomProtocolCommand::Code::GetDeviceInfo:
                {
                    command->Descriptor.DeviceInfoPayload.LbaCount = _LbaCount;
                    command->Descriptor.DeviceInfoPayload.BytesPerSector = _SectorSizeInBytes;
					command->Descriptor.DeviceInfoPayload.SectorsPerPage = _SectorsPerPage;

                    _CustomProtocolInterface->SubmitResponse(command);
                } break;
            }
        }
    }
};
