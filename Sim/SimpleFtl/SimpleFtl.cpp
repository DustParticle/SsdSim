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

void SimpleFtl::ReadPage(const U32& lba, U8 *outBuffer, const tSectorOffset& sectorOffset, const tSectorCount& sectorCount)
{
	assert((sectorOffset._ + sectorCount._) <= _SectorsPerPage);

    NandHal::CommandDesc commandDesc;
    commandDesc.Operation = NandHal::CommandDesc::Op::ReadPartial;
    SimpleFtlTranslation::LbaToNandAddress(lba, commandDesc.Address);
	commandDesc.Buffer = outBuffer;
	commandDesc.ByteOffset._ = sectorOffset._ * SimpleFtlTranslation::SectorSizeInBytes;
	commandDesc.ByteCount._ = sectorCount._ * SimpleFtlTranslation::SectorSizeInBytes;

    _NandHal->QueueCommand(commandDesc);
}

void SimpleFtl::ReadPage(const U32& lba, U8* outBuffer)
{
	NandHal::CommandDesc commandDesc;
	commandDesc.Operation = NandHal::CommandDesc::Op::Read;
	SimpleFtlTranslation::LbaToNandAddress(lba, commandDesc.Address);
	commandDesc.Buffer = outBuffer;

	_NandHal->QueueCommand(commandDesc);
}

void SimpleFtl::WritePage(const U32& lba, U8 *inBuffer)
{
    NandHal::CommandDesc commandDesc;
    commandDesc.Operation = NandHal::CommandDesc::Op::Write;
    SimpleFtlTranslation::LbaToNandAddress(lba, commandDesc.Address);
    commandDesc.Buffer = inBuffer;

    _NandHal->QueueCommand(commandDesc);
}

void SimpleFtl::WritePage(const U32& lba, U8* inBuffer, const tSectorOffset& sectorOffset, const tSectorCount& sectorCount)
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
    U8* buffer = _CustomProtocolInterface->GetBuffer(command, bufferSizeInBytes);

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
		ReadPage(lba, buffer, startSectorOffset, sectorsToProcessThisPage);
		remainingSectors -= sectorsToProcessThisPage._;

		startSectorOffset._ = 0;
		lba += sectorsToProcessThisPage._;
		buffer += sectorsToProcessThisPage._ << SimpleFtlTranslation::SectorSizeInBits;
	}

	while (remainingSectors > 0)
	{
		if (remainingSectors >= _SectorsPerPage)
		{
			ReadPage(lba, buffer);
			lba += _SectorsPerPage;
			buffer += _SectorsPerPage << SimpleFtlTranslation::SectorSizeInBits;
			remainingSectors -= _SectorsPerPage;
		}
		else
		{
			tSectorOffset sectorOffset;	sectorOffset._ = 0;
			tSectorCount sectorCount; sectorCount._ = remainingSectors;
			ReadPage(lba, buffer, sectorOffset, sectorCount);
			break;
		}
	}

	while (!_NandHal->IsCommandQueueEmpty());
	command->CommandStatus = (_NandHal->IsCommandSuccess() ? CustomProtocolCommand::Status::Success : CustomProtocolCommand::Status::Failed);
}

void SimpleFtl::WriteToNand(CustomProtocolCommand *command)
{
    U32 lba = command->Descriptor.SimpleFtlPayload.Lba;
    U32 sectorCount = command->Descriptor.SimpleFtlPayload.SectorCount;
    U32 bufferSizeInBytes = 0;
    U8* buffer = _CustomProtocolInterface->GetBuffer(command, bufferSizeInBytes);

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
		WritePage(lba, buffer, startSectorOffset, sectorsToProcessThisPage);
		remainingSectors -= sectorsToProcessThisPage._;

		startSectorOffset._ = 0;
		lba += sectorsToProcessThisPage._;
		buffer += sectorsToProcessThisPage._ << SimpleFtlTranslation::SectorSizeInBits;
	}

	while (remainingSectors > 0)
	{
		if (remainingSectors >= _SectorsPerPage)
		{
			WritePage(lba, buffer);
			lba += _SectorsPerPage;
			buffer += _SectorsPerPage << SimpleFtlTranslation::SectorSizeInBits;
			remainingSectors -= _SectorsPerPage;
		}
		else
		{
			tSectorOffset sectorOffset;	sectorOffset._ = 0;
			tSectorCount sectorCount; sectorCount._ = remainingSectors;
			WritePage(lba, buffer, sectorOffset, sectorCount);
			break;
		}
	}

	while (!_NandHal->IsCommandQueueEmpty());
	command->CommandStatus = (_NandHal->IsCommandSuccess() ? CustomProtocolCommand::Status::Success : CustomProtocolCommand::Status::Failed);
}