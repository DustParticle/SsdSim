#include "SimpleFtl.h"

SimpleFtl::SimpleFtl() : _CurrentState(IDLE)
{
}

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

void SimpleFtl::CheckForCommand()
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
            // Write command to nand
            _ProcessingCommand = command;
            _RemainingSectorCount = command->Descriptor.SimpleFtlPayload.SectorCount;
            _CurrentLba = command->Descriptor.SimpleFtlPayload.Lba;
            _ProcessedSectorCount = 0;
            _PendingCommandCount = 0;
            _CurrentState = PROCESSING;
        } break;

        case CustomProtocolCommand::Code::LoopbackWrite:
        {
            command->CommandStatus = CustomProtocolCommand::Status::Success;
            _CustomProtocolInterface->SubmitResponse(command);
        } break;

        case CustomProtocolCommand::Code::Read:
        {
            // Read command from nand
            _ProcessingCommand = command;
            _RemainingSectorCount = command->Descriptor.SimpleFtlPayload.SectorCount;
            _CurrentLba = command->Descriptor.SimpleFtlPayload.Lba;
            _ProcessedSectorCount = 0;
            _PendingCommandCount = 0;
            _CurrentState = PROCESSING;
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

void SimpleFtl::operator()()
{
    switch (_CurrentState)
    {
    case State::IDLE:
    {
        CheckForCommand();
    } break;

    case State::PROCESSING:
    {
        switch (_ProcessingCommand->Command)
        {
        case CustomProtocolCommand::Code::Write:
        {
            // Write command to nand
            if (WriteToNand())
            {
                _CustomProtocolInterface->SubmitResponse(_ProcessingCommand);
                _CurrentState = IDLE;
            }
        } break;

        case CustomProtocolCommand::Code::Read:
        {
            // Read command from nand
            if (ReadFromNand())
            {
                _CustomProtocolInterface->SubmitResponse(_ProcessingCommand);
                _CurrentState = IDLE;
            }
        } break;
        }
    } break;
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

bool SimpleFtl::ReadFromNand()
{
    Buffer buffer;
    NandHal::NandAddress nandAddress;
    U32 nextLba;
    U32 remainingSectorCount;
    if (_RemainingSectorCount > 0 || _PendingCommandCount > 0)
	{
        while (_RemainingSectorCount > 0)
        {
            SimpleFtlTranslation::LbaToNandAddress(_CurrentLba, _RemainingSectorCount, nandAddress, nextLba, remainingSectorCount);
            if (_BufferHal->AllocateBuffer(BufferType::User, nandAddress.SectorCount._, buffer))
            {
                ReadPage(nandAddress, buffer, _ProcessedSectorCount);
                _ProcessedSectorCount += nandAddress.SectorCount._;
                _CurrentLba = nextLba;
                _RemainingSectorCount = remainingSectorCount;
                ++_PendingCommandCount;
            }
            else
            {
                break;
            }
        }

        NandHal::CommandDesc commandDesc;
        while (true == _NandHal->PopFinishedCommand(commandDesc))
        {
            TransferCommandDesc transferCommand;
            transferCommand.Buffer = commandDesc.Buffer;
            transferCommand.Command = _ProcessingCommand;
            transferCommand.Direction = TransferCommandDesc::Direction::Out;
            transferCommand.SectorIndex = commandDesc.DescSectorIndex;
            _CustomProtocolInterface->QueueCommand(transferCommand);
            
            if (NandHal::CommandDesc::Status::Success != commandDesc.CommandStatus)
            {
                _ProcessingCommand->CommandStatus = CustomProtocolCommand::Status::ReadError;
            }
        }

        TransferCommandDesc transferCommand;
        while (true == _CustomProtocolInterface->PopFinishedCommand(transferCommand))
        {
            _BufferHal->DeallocateBuffer(transferCommand.Buffer);
            --_PendingCommandCount;
        }
	}

    return (_RemainingSectorCount == 0 && _PendingCommandCount == 0);
}

bool SimpleFtl::WriteToNand()
{
    Buffer buffer;
    NandHal::NandAddress nandAddress;
    U32 nextLba;
    U32 remainingSectorCount;
    if (_RemainingSectorCount > 0 || _PendingCommandCount > 0)
    {
        while (_RemainingSectorCount > 0)
        {
            SimpleFtlTranslation::LbaToNandAddress(_CurrentLba, _RemainingSectorCount, nandAddress, nextLba, remainingSectorCount);
            if (_BufferHal->AllocateBuffer(BufferType::User, nandAddress.SectorCount._, buffer))
            {
                TransferCommandDesc transferCommand;
                transferCommand.Buffer = buffer;
                transferCommand.Command = _ProcessingCommand;
                transferCommand.Direction = TransferCommandDesc::Direction::In;
                transferCommand.SectorIndex = _ProcessedSectorCount;
                transferCommand.NandAddress = nandAddress;
                _CustomProtocolInterface->QueueCommand(transferCommand);

                _ProcessedSectorCount += nandAddress.SectorCount._;
                _CurrentLba = nextLba;
                _RemainingSectorCount = remainingSectorCount;
                ++_PendingCommandCount;
            }
            else
            {
                break;
            }
        }

        TransferCommandDesc transferCommand;
        while (true == _CustomProtocolInterface->PopFinishedCommand(transferCommand))
        {
            WritePage(transferCommand.NandAddress, transferCommand.Buffer);
        }

        NandHal::CommandDesc commandDesc;
        while (true == _NandHal->PopFinishedCommand(commandDesc))
        {
            if (NandHal::CommandDesc::Status::Success != commandDesc.CommandStatus)
            {
                _ProcessingCommand->CommandStatus = CustomProtocolCommand::Status::WriteError;
            }
            _BufferHal->DeallocateBuffer(commandDesc.Buffer);
            --_PendingCommandCount;
        }
    }

    return (_RemainingSectorCount == 0 && _PendingCommandCount == 0);
}