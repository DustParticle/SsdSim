#include "SimpleFtl.h"

void SimpleFtl::SetProtocol(CustomProtocolInterface *interface)
{
    SimpleFtl::_CustomProtocolInterface = interface;
}

void SimpleFtl::SetNandHal(NandHal *nandHal)
{
    _NandHal = nandHal;
    NandHal::Geometry geometry = _NandHal->GetGeometry();
    _SectorsPerPage = geometry._BytesPerPage >> SimpleFtlTranslation::SectorSizeInBits;
    _TotalSectors = geometry._ChannelCount * geometry._DevicesPerChannel
        * geometry._BlocksPerDevice * geometry._PagesPerBlock * _SectorsPerPage;

    SimpleFtlTranslation::SetGeometry(geometry);
}

void SimpleFtl::operator()()
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
            command->Descriptor.DeviceInfoPayload.TotalSector = _TotalSectors;
            command->Descriptor.DeviceInfoPayload.BytesPerSector = SimpleFtlTranslation::SectorSizeInBytes;
            command->Descriptor.DeviceInfoPayload.SectorsPerPage = _SectorsPerPage;
            _CustomProtocolInterface->SubmitResponse(command);
        } break;
        }
    }
}

bool SimpleFtl::ReadPage(const U32 &lba, U8 *outBuffer, const U8 &startSectorIndex, const U8 &sectorToRead)
{
    if (startSectorIndex + sectorToRead > _SectorsPerPage)
    {
        // Out of bound
        return false;
    }

    // Create Nand command
    NandHal::CommandDesc commandDesc;
    commandDesc.Operation = NandHal::CommandDesc::Op::READ;
    SimpleFtlTranslation::LbaToNandAddress(lba, commandDesc.Address);
    commandDesc.Buffer = (sectorToRead == _SectorsPerPage) ? outBuffer : _SharedBuffer;  // Read directly to outputBuffer to optimize performance if reading full page

    // Submit to the NandHal
    _NandHal->QueueCommand(commandDesc);

    // Wait for nand command completed
    while (!_NandHal->IsCommandQueueEmpty());

    if (sectorToRead < _SectorsPerPage)
    {
        // Copy sectors from SharedBuffer to output
        CopySectors(outBuffer,
            commandDesc.Buffer + (startSectorIndex << SimpleFtlTranslation::SectorSizeInBits), 
            sectorToRead);
    }

    return true;
}

bool SimpleFtl::WritePage(const U32 &lba, U8 *inBuffer)
{
    // Create Nand command
    NandHal::CommandDesc commandDesc;
    commandDesc.Operation = NandHal::CommandDesc::Op::WRITE;
    SimpleFtlTranslation::LbaToNandAddress(lba, commandDesc.Address);
    commandDesc.Buffer = inBuffer;  // Read directly to outputBuffer to optimize performance if reading full page

    // Submit to the NandHal
    _NandHal->QueueCommand(commandDesc);

    // Wait for nand command completed
    while (!_NandHal->IsCommandQueueEmpty());

    return true;
}

void SimpleFtl::ReadFromNand(CustomProtocolCommand *command)
{
    U32 lba = command->Descriptor.SimpleFtlPayload.Lba;
    U32 sectorCount = command->Descriptor.SimpleFtlPayload.SectorCount;
    U32 bufferSizeInBytes = 0;
    U8* buffer = _CustomProtocolInterface->GetBuffer(command, bufferSizeInBytes);

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

        ReadPage(lba, buffer, readSectorIndex, sectorToRead);

        buffer += (sectorToRead << SimpleFtlTranslation::SectorSizeInBits);
        remainingSectors -= sectorToRead;
        lba += _SectorsPerPage;

        // read from the beginning of the page since next command
        readSectorIndex = 0;
    }
}

void SimpleFtl::WriteToNand(CustomProtocolCommand *command)
{
    U32 lba = command->Descriptor.SimpleFtlPayload.Lba;
    U32 sectorCount = command->Descriptor.SimpleFtlPayload.SectorCount;
    U32 bufferSizeInBytes = 0;
    U8* buffer = _CustomProtocolInterface->GetBuffer(command, bufferSizeInBytes);

    U32 alignedStartLba = (lba / _SectorsPerPage) * _SectorsPerPage;
    U8 writeSectorIndex = lba - alignedStartLba;
    lba = alignedStartLba;

    U32 remainingSectors = sectorCount;
    if (writeSectorIndex > 0)
    {
        // Unaligned start lba
        // Read full page
        ReadPage(lba, _SharedBuffer, 0, _SectorsPerPage);

        // Override the changing lba
        U32 sectorToWrite = _SectorsPerPage - writeSectorIndex;
        if (sectorToWrite > sectorCount)
        {
            sectorToWrite = sectorCount;
        }
        CopySectors(_SharedBuffer + (writeSectorIndex << SimpleFtlTranslation::SectorSizeInBits),
            buffer, sectorToWrite);

        // Write full page
        WritePage(lba, _SharedBuffer);

        buffer += (sectorToWrite << SimpleFtlTranslation::SectorSizeInBits);
        remainingSectors -= sectorToWrite;
        lba += _SectorsPerPage;
    }

    while (remainingSectors >= _SectorsPerPage)
    {
        WritePage(lba, buffer);

        buffer += (_SectorsPerPage << SimpleFtlTranslation::SectorSizeInBits);
        remainingSectors -= _SectorsPerPage;
        lba += _SectorsPerPage;
    }

    if (remainingSectors > 0)
    {
        // Unaligned end lba
        // Read full page
        ReadPage(lba, _SharedBuffer, 0, _SectorsPerPage);

        // Override the changing lba
        CopySectors(_SharedBuffer, buffer, remainingSectors);

        // Write full page
        WritePage(lba, _SharedBuffer);

        buffer += (remainingSectors << SimpleFtlTranslation::SectorSizeInBits);
        remainingSectors -= remainingSectors;
        lba += _SectorsPerPage;
    }
}

void SimpleFtl::CopySectors(U8 *dest, U8 *src, const U32 &lengthInSector)
{
    memcpy(dest, src, lengthInSector << SimpleFtlTranslation::SectorSizeInBits);
}
