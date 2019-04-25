// WriteRead.cpp : Defines the exported functions for the DLL application.
//

#include <memory>
#include <functional>

#include "Translation.h"
#include "HostComm/CustomProtocol/CustomProtocolInterface.h"
#include "Nand/Hal/NandHal.h"

constexpr U32 _SectorSizeInBytes = 512;

std::unique_ptr<CustomProtocolInterface> _CustomProtocolInterface = nullptr;
std::shared_ptr<NandHal> _NandHal;
NandHal::Geometry _Geometry;
U32 _TotalSectors;
U8 _SectorsPerPage;

extern "C"
{
    void CreateNandCommand(U32 lba, U8 *buffer, NandHal::CommandDesc::Op operation, NandHal::CommandDesc &commandDesc)
    {
        commandDesc.Operation = operation;
        commandDesc.Buffer = buffer;

        //Translate from LBA to NAND address
        SimpleFtlTranslation::LbaToNandAddress(_Geometry, lba, commandDesc.Address);
    }

    U32 CalculateTransferPageCount(U32 sectorCount)
    {
        U32 transferPageCount = (sectorCount / _SectorsPerPage);
        transferPageCount += (0 < (sectorCount % _SectorsPerPage) ? 1 : 0);
        return transferPageCount;
    }

    void ReadPage(NandHal::CommandDesc &commandDesc, U32 lba, U8 *outBuffer, U8 startSectorIndex, U8 sectorToRead)
    {
        if (startSectorIndex + sectorToRead > _SectorsPerPage)
        {
            throw "Out of bound";
        }

        U8 *tempBuffer = commandDesc.Buffer;
        if (sectorToRead == _SectorsPerPage)
        {
            // Read directly to outputBuffer to optimize performance
            commandDesc.Buffer = outBuffer;
        }

        // Create Nand command
        commandDesc.Operation = NandHal::CommandDesc::Op::READ;
        SimpleFtlTranslation::LbaToNandAddress(_Geometry, lba, commandDesc.Address);

        // Submit to the NandHal
        _NandHal->QueueCommand(commandDesc);

        // Wait for nand command completed
        while (!_NandHal->IsCommandQueueEmpty());

        if (sectorToRead < _SectorsPerPage)
        {
            // Copy read page buffer to buffer
            memcpy(outBuffer, commandDesc.Buffer + (startSectorIndex * _SectorSizeInBytes), sectorToRead * _SectorSizeInBytes);
            commandDesc.Buffer = tempBuffer;
        }
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
            lba += _SectorsPerPage;
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

        U32 alignedStartLba = (lba / _SectorsPerPage) * _SectorsPerPage;
        U8 readSectorIndex = lba - alignedStartLba;
        lba = alignedStartLba;

        U32 remainingSectors = sectorCount;
        while (remainingSectors > 0)
        {
            U8 sectorToRead = _SectorsPerPage - readSectorIndex;
            if (sectorToRead > remainingSectors)
            {
                sectorToRead = remainingSectors;
            }

            ReadPage(commandDesc, lba, buffer, readSectorIndex, sectorToRead);

            buffer += (sectorToRead * _SectorSizeInBytes);
            remainingSectors -= sectorToRead;
            lba += sectorToRead;

            // read from the beginning of the page since next command
            readSectorIndex = 0;
        }
    }

    void __declspec(dllexport) __stdcall Initialize(std::shared_ptr<NandHal> nandHal)
    {
        _NandHal = nandHal;
        _Geometry = nandHal->GetGeometry();
        _SectorsPerPage = _Geometry._BytesPerPage / _SectorSizeInBytes;
        _TotalSectors = _Geometry._ChannelCount * _Geometry._DevicesPerChannel * _Geometry._BlocksPerDevice * _Geometry._PagesPerBlock * _SectorsPerPage;
    }

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
                    command->Descriptor.DeviceInfoPayload.TotalSector = _TotalSectors;
                    command->Descriptor.DeviceInfoPayload.BytesPerSector = _SectorSizeInBytes;
					command->Descriptor.DeviceInfoPayload.SectorsPerPage = _SectorsPerPage;

                    _CustomProtocolInterface->SubmitResponse(command);
                } break;
            }
        }
    }

    void __declspec(dllexport) __stdcall SetCustomProtocolIpcName(const std::string& protocolIpcName)
    {
        _CustomProtocolInterface = std::make_unique<CustomProtocolInterface>(protocolIpcName.c_str());
    }
};
