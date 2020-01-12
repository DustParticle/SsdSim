#include "SimpleFtl.h"

SimpleFtl::SimpleFtl() : _ProcessingCommand(nullptr)
{
    _EventQueue = std::unique_ptr<boost::lockfree::queue<Event>>(new boost::lockfree::queue<Event>{ 1024 });
}

void SimpleFtl::SetProtocol(CustomProtocolHal *customProtocolHal)
{
    SimpleFtl::_CustomProtocolHal = customProtocolHal;
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
    while (_EventQueue->empty() == false)
    {
        Event event;
        _EventQueue->pop(event);
        switch (event.EventType)
        {
        case Event::Type::CustomProtocolCommand:
        {
            OnNewCustomProtocolCommand(event.EventParams.CustomProtocolCommand);
        } break;

        case Event::Type::TransferCompleted:
        {
            OnTransferCommandCompleted(event.EventParams.TransferCommand);
        } break;

        case Event::Type::NandCommandCompleted:
        {
            OnNandCommandCompleted(event.EventParams.NandCommand);
        } break;

        default:
        {
            assert(0);
        }
        }
    }
}

void SimpleFtl::OnNewCustomProtocolCommand(CustomProtocolCommand *command)
{
    // NOTE: only support for handling single command at a time
    assert(_ProcessingCommand == nullptr);

    // Set default command staus is Success
    _ProcessingCommand = command;
    command->CommandStatus = CustomProtocolCommand::Status::Success;

    switch (command->Command)
    {
    case CustomProtocolCommand::Code::Write:
    {
        // Write command to nand
        _RemainingSectorCount = command->Descriptor.SimpleFtlPayload.SectorCount;
        _CurrentLba = command->Descriptor.SimpleFtlPayload.Lba;
        _ProcessedSectorCount = 0;
        _PendingCommandCount = 0;

        WriteNextLbas();
    } break;

    case CustomProtocolCommand::Code::LoopbackWrite:
    {
        command->CommandStatus = CustomProtocolCommand::Status::Success;
        SubmitResponse();
    } break;

    case CustomProtocolCommand::Code::Read:
    {
        // Read command from nand
        _RemainingSectorCount = command->Descriptor.SimpleFtlPayload.SectorCount;
        _CurrentLba = command->Descriptor.SimpleFtlPayload.Lba;
        _ProcessedSectorCount = 0;
        _PendingCommandCount = 0;

        ReadNextLbas();
    } break;

    case CustomProtocolCommand::Code::LoopbackRead:
    {
        command->CommandStatus = CustomProtocolCommand::Status::Success;
        SubmitResponse();
    } break;

    case CustomProtocolCommand::Code::GetDeviceInfo:
    {
        command->Descriptor.DeviceInfoPayload.TotalSector = _TotalSectors;
        command->Descriptor.DeviceInfoPayload.SectorInfo = _BufferHal->GetSectorInfo();
        command->Descriptor.DeviceInfoPayload.SectorsPerPage = _SectorsPerPage;
        command->CommandStatus = CustomProtocolCommand::Status::Success;
        SubmitResponse();
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
        SubmitResponse();
    } break;
    }
}

void SimpleFtl::ReadNextLbas()
{
    Buffer buffer;
    NandHal::NandAddress nandAddress;
    U32 nextLba;
    U32 remainingSectorCount;
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
}

void SimpleFtl::TransferOut(const Buffer &buffer, const NandHal::NandAddress &nandAddress, const U32 &sectorIndex)
{
    CustomProtocolHal::TransferCommandDesc transferCommand;
    transferCommand.Buffer = buffer;
    transferCommand.Command = _ProcessingCommand;
    transferCommand.Direction = CustomProtocolHal::TransferCommandDesc::Direction::Out;
    transferCommand.SectorIndex = sectorIndex;
    transferCommand.NandAddress = nandAddress;
    transferCommand.Listener = this;
    _CustomProtocolHal->QueueCommand(transferCommand);
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
    commandDesc.Listener = this;

    _NandHal->QueueCommand(commandDesc);
}

void SimpleFtl::WriteNextLbas()
{
    Buffer buffer;
    NandHal::NandAddress nandAddress;
    U32 nextLba;
    U32 remainingSectorCount;
    while (_RemainingSectorCount > 0)
    {
        SimpleFtlTranslation::LbaToNandAddress(_CurrentLba, _RemainingSectorCount, nandAddress, nextLba, remainingSectorCount);
        if (_BufferHal->AllocateBuffer(BufferType::User, nandAddress.SectorCount._, buffer))
        {
            TransferIn(buffer, nandAddress, _ProcessedSectorCount);
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
}

void SimpleFtl::TransferIn(const Buffer &buffer, const NandHal::NandAddress &nandAddress, const U32 &sectorIndex)
{
    CustomProtocolHal::TransferCommandDesc transferCommand;
    transferCommand.Buffer = buffer;
    transferCommand.Command = _ProcessingCommand;
    transferCommand.Direction = CustomProtocolHal::TransferCommandDesc::Direction::In;
    transferCommand.SectorIndex = sectorIndex;
    transferCommand.NandAddress = nandAddress;
    transferCommand.Listener = this;
    _CustomProtocolHal->QueueCommand(transferCommand);
}

void SimpleFtl::WritePage(const NandHal::NandAddress &nandAddress, const Buffer &inBuffer)
{
    assert((nandAddress.Sector._ + nandAddress.SectorCount._) <= _SectorsPerPage);

    NandHal::CommandDesc commandDesc;
    commandDesc.Address = nandAddress;
    commandDesc.Operation = (nandAddress.SectorCount._ == _SectorsPerPage)
        ? NandHal::CommandDesc::Op::Write : NandHal::CommandDesc::Op::WritePartial;
    commandDesc.Buffer = inBuffer;
    commandDesc.Listener = this;

    _NandHal->QueueCommand(commandDesc);
}

void SimpleFtl::OnTransferCommandCompleted(const CustomProtocolHal::TransferCommandDesc &command)
{
    if (CustomProtocolCommand::Code::Read == _ProcessingCommand->Command)
    {
        _BufferHal->DeallocateBuffer(command.Buffer);
        --_PendingCommandCount;

        if (_RemainingSectorCount == 0 && _PendingCommandCount == 0)
        {
            SubmitResponse();
        }
        else
        {
            ReadNextLbas();
        }
    }
    else
    {
        WritePage(command.NandAddress, command.Buffer);
    }
}

void SimpleFtl::OnNandCommandCompleted(const NandHal::CommandDesc &command)
{
    if (CustomProtocolCommand::Code::Read == _ProcessingCommand->Command)
    {
        TransferOut(command.Buffer, command.Address, command.DescSectorIndex);

        if (NandHal::CommandDesc::Status::Success != command.CommandStatus)
        {
            _ProcessingCommand->CommandStatus = CustomProtocolCommand::Status::ReadError;
        }
    }
    else
    {
        if (NandHal::CommandDesc::Status::Success != command.CommandStatus)
        {
            _ProcessingCommand->CommandStatus = CustomProtocolCommand::Status::WriteError;
        }
        _BufferHal->DeallocateBuffer(command.Buffer);
        --_PendingCommandCount;

        if (_RemainingSectorCount == 0 && _PendingCommandCount == 0)
        {
            SubmitResponse();
        }
        else
        {
            WriteNextLbas();
        }
    }
}

void SimpleFtl::SubmitCustomProtocolCommand(CustomProtocolCommand *command)
{
    Event event;
    event.EventType = Event::Type::CustomProtocolCommand;
    event.EventParams.CustomProtocolCommand = command;
    _EventQueue->push(event);
}

void SimpleFtl::HandleCommandCompleted(const CustomProtocolHal::TransferCommandDesc &command)
{
    Event event;
    event.EventType = Event::Type::TransferCompleted;
    event.EventParams.TransferCommand = command;
    assert(_EventQueue->push(event) == true);
}

void SimpleFtl::HandleCommandCompleted(const NandHal::CommandDesc &command)
{
    Event event;
    event.EventType = Event::Type::NandCommandCompleted;
    event.EventParams.NandCommand = command;
    assert(_EventQueue->push(event) == true);
}

bool SimpleFtl::IsProcessingCommand()
{
    return (nullptr != _ProcessingCommand);
}

void SimpleFtl::SubmitResponse()
{
    assert(_ProcessingCommand != nullptr);
    _CustomProtocolHal->SubmitResponse(_ProcessingCommand);
    _ProcessingCommand = nullptr;
}