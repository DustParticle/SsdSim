#include "SimpleFtl.h"

void SimpleFtl::SetProtocol(CustomProtocolInterface *interface)
{
    SimpleFtl::_CustomProtocolInterface = interface;
}

void SimpleFtl::SetNandHal(NandHal *nandHal)
{
    _NandHal = nandHal;
    NandHal::Geometry geometry = _NandHal->GetGeometry();

    SimpleFtlTranslation::SetGeometry(geometry);
}

void SimpleFtl::SetBufferHal(BufferHal *bufferHal)
{
    _BufferHal = bufferHal;
    SetSectorInfo(DefaultSectorInfo);
}

bool SimpleFtl::SetSectorInfo(const SectorInfo &sectorInfo)
{
    if (_BufferHal->SetSectorInfo(sectorInfo) == false)
    {
        return false;
    }

    NandHal::Geometry geometry = _NandHal->GetGeometry();
    _SectorsPerPage = geometry.BytesPerPage >> sectorInfo.SectorSizeInBit;
    _TotalSectors = geometry.ChannelCount * geometry.DevicesPerChannel
        * geometry.BlocksPerDevice * geometry.PagesPerBlock * _SectorsPerPage;
    SimpleFtlTranslation::SetSectorSize(sectorInfo.SectorSizeInBit);
    return true;
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
            command->Descriptor.DeviceInfoPayload.SectorInfo = _BufferHal->GetSectorInfo();
            command->Descriptor.DeviceInfoPayload.SectorsPerPage = _SectorsPerPage;
			command->CommandStatus = CustomProtocolCommand::Status::Success;
            _CustomProtocolInterface->SubmitResponse(command);
        } break;

        case CustomProtocolCommand::Code::SetSectorSize:
        {
            SectorInfo sectorInfo = command->Descriptor.SectorInfoPayload.SectorInfo;
            if (SetSectorInfo(sectorInfo))
            {
                command->CommandStatus = CustomProtocolCommand::Status::Success;
            }
            else
            {
                command->CommandStatus = CustomProtocolCommand::Status::Failed;
            }
            _CustomProtocolInterface->SubmitResponse(command);
        } break;
        }
    }
}

void SimpleFtl::ReadPage(const NandHal::NandAddress &nandAddress, const Buffer &outBuffer, const U32 &descSectorIndex)
{
    assert((nandAddress.Sector._ + nandAddress.SectorCount._) <= _SectorsPerPage);

	NandHal::CommandDesc commandDesc;
    commandDesc.Address = nandAddress;
    commandDesc.Operation = (nandAddress.SectorCount._ == _SectorsPerPage)
        ? NandHal::CommandDesc::Op::Read : NandHal::CommandDesc::Op::ReadPartial;
	commandDesc.Buffer = outBuffer;
    commandDesc.DescSectorIndex = descSectorIndex;

	_NandHal->QueueCommand(commandDesc);
}

void SimpleFtl::WritePage(const NandHal::NandAddress &nandAddress, const Buffer &inBuffer)
{
    assert((nandAddress.Sector._ + nandAddress.SectorCount._) <= _SectorsPerPage);

    NandHal::CommandDesc commandDesc;
    commandDesc.Address = nandAddress;
    commandDesc.Operation = (nandAddress.SectorCount._ == _SectorsPerPage)
        ? NandHal::CommandDesc::Op::Write : NandHal::CommandDesc::Op::WritePartial;
    commandDesc.Buffer = inBuffer;

    _NandHal->QueueCommand(commandDesc);
}

void SimpleFtl::ReadFromNand(CustomProtocolCommand *command)
{
    U32 lba = command->Descriptor.SimpleFtlPayload.Lba;
    U32 sectorCount = command->Descriptor.SimpleFtlPayload.SectorCount;

    Buffer buffer;
    NandHal::NandAddress nandAddress;
    U32 descSectorCount = 0;
	while (sectorCount > 0)
	{
        SimpleFtlTranslation::LbaToNandAddress(lba, sectorCount, nandAddress, lba, sectorCount);
        _BufferHal->AllocateBuffer(BufferType::User, nandAddress.SectorCount._, buffer);
		ReadPage(nandAddress, buffer, descSectorCount);
        descSectorCount += nandAddress.SectorCount._;
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
            _CustomProtocolInterface->TransferOut(command, commandDesc.Buffer, commandDesc.DescSectorIndex);
        }
        _BufferHal->DeallocateBuffer(commandDesc.Buffer);
	}
}

void SimpleFtl::WriteToNand(CustomProtocolCommand *command)
{
    U32 lba = command->Descriptor.SimpleFtlPayload.Lba;
    U32 sectorCount = command->Descriptor.SimpleFtlPayload.SectorCount;

    Buffer buffer;
    NandHal::NandAddress nandAddress;
    U32 srcSectorCount = 0;
    while (sectorCount > 0)
    {
        SimpleFtlTranslation::LbaToNandAddress(lba, sectorCount, nandAddress, lba, sectorCount);
        _BufferHal->AllocateBuffer(BufferType::User, nandAddress.SectorCount._, buffer);
        _CustomProtocolInterface->TransferIn(command, buffer, srcSectorCount);
        WritePage(nandAddress, buffer);
        srcSectorCount += nandAddress.SectorCount._;
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