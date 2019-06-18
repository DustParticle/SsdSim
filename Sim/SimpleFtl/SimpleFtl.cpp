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

void SimpleFtl::SetBufferHal(BufferHal *bufferHal)
{
    _BufferHal = bufferHal;
}

void SimpleFtl::operator()()
{
    if (_CustomProtocolInterface->HasCommand())
    {
        CustomProtocolCommand *command = _CustomProtocolInterface->GetCommand();

		// Set default command staus is Success
		command->CommandStatus = CustomProtocolCommand::Status::Success;

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
			command->CommandStatus = CustomProtocolCommand::Status::Success;
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
			command->CommandStatus = CustomProtocolCommand::Status::Success;
            _CustomProtocolInterface->SubmitResponse(command);
        } break;

        case CustomProtocolCommand::Code::GetDeviceInfo:
        {
            command->Descriptor.DeviceInfoPayload.TotalSector = _TotalSectors;
            command->Descriptor.DeviceInfoPayload.BytesPerSector = SimpleFtlTranslation::SectorSizeInBytes;
            command->Descriptor.DeviceInfoPayload.SectorsPerPage = _SectorsPerPage;
			command->CommandStatus = CustomProtocolCommand::Status::Success;
            _CustomProtocolInterface->SubmitResponse(command);
        } break;
        }
    }
}

void SimpleFtl::ReadPage(const U32& lba, const Buffer &outBuffer, const tSectorOffset& sectorOffset, const tSectorCount& sectorCount, U8* const descBuffer)
{
	assert((sectorOffset._ + sectorCount._) <= _SectorsPerPage);

    NandHal::CommandDesc commandDesc;
    commandDesc.Operation = NandHal::CommandDesc::Op::ReadPartial;
    SimpleFtlTranslation::LbaToNandAddress(lba, commandDesc.Address);
	commandDesc.Buffer = outBuffer;
    commandDesc.DescBuffer = descBuffer;
	commandDesc.ByteOffset._ = sectorOffset._ * SimpleFtlTranslation::SectorSizeInBytes;
	commandDesc.ByteCount._ = sectorCount._ * SimpleFtlTranslation::SectorSizeInBytes;

    _NandHal->QueueCommand(commandDesc);
}

void SimpleFtl::ReadPage(const U32& lba, const Buffer &outBuffer, U8* const descBuffer)
{
	NandHal::CommandDesc commandDesc;
	commandDesc.Operation = NandHal::CommandDesc::Op::Read;
	SimpleFtlTranslation::LbaToNandAddress(lba, commandDesc.Address);
	commandDesc.Buffer = outBuffer;
    commandDesc.DescBuffer = descBuffer;

	_NandHal->QueueCommand(commandDesc);
}

void SimpleFtl::WritePage(const U32& lba, const Buffer &inBuffer)
{
    NandHal::CommandDesc commandDesc;
    commandDesc.Operation = NandHal::CommandDesc::Op::Write;
    SimpleFtlTranslation::LbaToNandAddress(lba, commandDesc.Address);
    commandDesc.Buffer = inBuffer;

    _NandHal->QueueCommand(commandDesc);
}

void SimpleFtl::WritePage(const U32& lba, const Buffer &inBuffer, const tSectorOffset& sectorOffset, const tSectorCount& sectorCount)
{
	assert((sectorOffset._ + sectorCount._) <= _SectorsPerPage);

	NandHal::CommandDesc commandDesc;
	commandDesc.Operation = NandHal::CommandDesc::Op::WritePartial;
	SimpleFtlTranslation::LbaToNandAddress(lba, commandDesc.Address);
	commandDesc.Buffer = inBuffer;
	commandDesc.ByteOffset._ = sectorOffset._ * SimpleFtlTranslation::SectorSizeInBytes;
	commandDesc.ByteCount._ = sectorCount._ * SimpleFtlTranslation::SectorSizeInBytes;

	_NandHal->QueueCommand(commandDesc);
}

void SimpleFtl::ReadFromNand(CustomProtocolCommand *command)
{
    U32 lba = command->Descriptor.SimpleFtlPayload.Lba;
    U32 sectorCount = command->Descriptor.SimpleFtlPayload.SectorCount;
    U32 bufferSizeInBytes = 0;
    U8* rawBuffer = _CustomProtocolInterface->GetBuffer(command, bufferSizeInBytes);

    tSectorOffset startSectorOffset;
	startSectorOffset._ = lba % _SectorsPerPage;
	U32 remainingSectors = sectorCount;
	if (startSectorOffset._ != 0)
	{
		tSectorCount sectorsToProcessThisPage;
		if (startSectorOffset._ + sectorCount <= _SectorsPerPage)
		{
			sectorsToProcessThisPage._ = sectorCount;
		}
		else
		{
			sectorsToProcessThisPage._ = _SectorsPerPage - startSectorOffset._;
		}

        Buffer buffer = _BufferHal->AllocateBuffer(sectorsToProcessThisPage._ << SimpleFtlTranslation::SectorSizeInBits);
		ReadPage(lba, buffer, startSectorOffset, sectorsToProcessThisPage, rawBuffer);
        rawBuffer += sectorsToProcessThisPage._ << SimpleFtlTranslation::SectorSizeInBits;
		remainingSectors -= sectorsToProcessThisPage._;

		startSectorOffset._ = 0;
		lba += sectorsToProcessThisPage._;
	}

	while (remainingSectors > 0)
	{
		if (remainingSectors >= _SectorsPerPage)
		{
            Buffer buffer = _BufferHal->AllocateBuffer(_SectorsPerPage << SimpleFtlTranslation::SectorSizeInBits);
			ReadPage(lba, buffer, rawBuffer);
            rawBuffer += _SectorsPerPage << SimpleFtlTranslation::SectorSizeInBits;
			lba += _SectorsPerPage;
			remainingSectors -= _SectorsPerPage;
		}
		else
		{
			tSectorOffset sectorOffset;	sectorOffset._ = 0;
			tSectorCount sectorCount; sectorCount._ = remainingSectors;
            Buffer buffer = _BufferHal->AllocateBuffer(remainingSectors << SimpleFtlTranslation::SectorSizeInBits);
            ReadPage(lba, buffer, sectorOffset, sectorCount, rawBuffer);
            rawBuffer += remainingSectors << SimpleFtlTranslation::SectorSizeInBits;
			break;
		}
	}

	while (!_NandHal->IsCommandQueueEmpty());

	NandHal::CommandDesc commandDesc;
	while(true == _NandHal->PopFinishedCommand(commandDesc))
	{
		if (NandHal::CommandDesc::Status::Success != commandDesc.CommandStatus)
		{
			command->CommandStatus = CustomProtocolCommand::Status::ReadError;
		}
        else
        {
            _BufferHal->Memcpy(commandDesc.DescBuffer, commandDesc.Buffer);
        }
        _BufferHal->DeallocateBuffer(commandDesc.Buffer);
	}
}

void SimpleFtl::WriteToNand(CustomProtocolCommand *command)
{
    U32 lba = command->Descriptor.SimpleFtlPayload.Lba;
    U32 sectorCount = command->Descriptor.SimpleFtlPayload.SectorCount;
    U32 bufferSizeInBytes = 0;
    U8* rawBuffer = _CustomProtocolInterface->GetBuffer(command, bufferSizeInBytes);

	tSectorOffset startSectorOffset;
	startSectorOffset._ = lba % _SectorsPerPage;
	U32 remainingSectors = sectorCount;
	if (startSectorOffset._ != 0)
	{
		tSectorCount sectorsToProcessThisPage;
		if (startSectorOffset._ + sectorCount <= _SectorsPerPage)
		{
			sectorsToProcessThisPage._ = sectorCount;
		}
		else
		{
			sectorsToProcessThisPage._ = _SectorsPerPage - startSectorOffset._;
		}
        Buffer buffer = _BufferHal->AllocateBuffer(sectorsToProcessThisPage._ << SimpleFtlTranslation::SectorSizeInBits);
        _BufferHal->Memcpy(buffer, rawBuffer);
        WritePage(lba, buffer, startSectorOffset, sectorsToProcessThisPage);
        rawBuffer += sectorsToProcessThisPage._ << SimpleFtlTranslation::SectorSizeInBits;
		remainingSectors -= sectorsToProcessThisPage._;

		startSectorOffset._ = 0;
		lba += sectorsToProcessThisPage._;
	}

	while (remainingSectors > 0)
	{
		if (remainingSectors >= _SectorsPerPage)
		{
            Buffer buffer = _BufferHal->AllocateBuffer(_SectorsPerPage << SimpleFtlTranslation::SectorSizeInBits);
            _BufferHal->Memcpy(buffer, rawBuffer);
			WritePage(lba, buffer);
            rawBuffer += _SectorsPerPage << SimpleFtlTranslation::SectorSizeInBits;
			lba += _SectorsPerPage;
			remainingSectors -= _SectorsPerPage;
		}
		else
		{
			tSectorOffset sectorOffset;	sectorOffset._ = 0;
			tSectorCount sectorCount; sectorCount._ = remainingSectors;
            Buffer buffer = _BufferHal->AllocateBuffer(remainingSectors << SimpleFtlTranslation::SectorSizeInBits);
            WritePage(lba, buffer, sectorOffset, sectorCount);
            rawBuffer += remainingSectors << SimpleFtlTranslation::SectorSizeInBits;
            break;
		}
	}

	while (!_NandHal->IsCommandQueueEmpty());

	NandHal::CommandDesc commandDesc;
	while(true == _NandHal->PopFinishedCommand(commandDesc))
	{
		if (NandHal::CommandDesc::Status::Success != commandDesc.CommandStatus)
		{
			command->CommandStatus = CustomProtocolCommand::Status::WriteError;
		}
        _BufferHal->DeallocateBuffer(commandDesc.Buffer);
	}
}