#include "SimpleFtl.h"

SimpleFtl::SimpleFtl() : _ProcessingCommand(nullptr)
{
    _EventQueue = std::unique_ptr<boost::lockfree::queue<Event>>(new boost::lockfree::queue<Event>{ 1024 });
}

void SimpleFtl::SetProtocol(CustomProtocolHal* customProtocolHal)
{
    SimpleFtl::_CustomProtocolHal = customProtocolHal;
}

void SimpleFtl::SetNandHal(NandHal* nandHal)
{
    _NandHal = nandHal;
    NandHal::Geometry geometry = _NandHal->GetGeometry();

    SimpleFtlTranslation::SetGeometry(geometry);
}

void SimpleFtl::SetBufferHal(BufferHal* bufferHal)
{
    _BufferHal = bufferHal;
    SetSectorInfo(DefaultSectorInfo);
}

bool SimpleFtl::SetSectorInfo(const SectorInfo& sectorInfo)
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

    // The buffer size must be larger than maxBlocksPerCommand blocks' size to support read-modify-write
    constexpr U8 maxBlocksPerCommand = 8;
    U32 blockSizeInBytes = geometry.PagesPerBlock * geometry.BytesPerPage;
    assert(_BufferHal->GetBufferMaxSizeInBytes() >= blockSizeInBytes * maxBlocksPerCommand);
    _BlockBuffers = new Buffer[geometry.PagesPerBlock * maxBlocksPerCommand];
    _ProcessingBlocks = new NandHal::NandAddress[maxBlocksPerCommand];

    _SectorsPerSegment = _SectorsPerPage;
    _PagesPerBlock = geometry.PagesPerBlock;
    _BufferHal->SetImplicitAllocationSectorCount(_SectorsPerSegment);

    return true;
}

void SimpleFtl::Shutdown()
{
    delete[] _BlockBuffers;
    _BlockBuffers = nullptr;
    delete[] _ProcessingBlocks;
    _ProcessingBlocks = nullptr;
}

void SimpleFtl::operator()()
{
    while (_EventQueue->empty() == false)
    {
        ProcessEvent();
    }
}

void SimpleFtl::ProcessEvent()
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

void SimpleFtl::OnNewCustomProtocolCommand(CustomProtocolCommand* command)
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
        _RemainingSectorCount = command->Descriptor.SimpleFtlPayload.SectorCount;
        _CurrentLba = command->Descriptor.SimpleFtlPayload.Lba;
        _ProcessedSectorOffset = 0;
        _PendingTransferCommandCount = 0;
        _PendingNandCommandCount = 0;
        _ProcessingBlockCount = 0;
        _BlockBufferCount = 0;

        // Write command to nand
        HandleWriteCommand();
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
        _ProcessedSectorOffset = 0;
        _PendingTransferCommandCount = 0;
        _PendingNandCommandCount = 0;

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
        if (_BufferHal->AllocateBuffer(BufferType::User, buffer))
        {
            ReadPage(nandAddress, buffer, _ProcessedSectorOffset);
            _ProcessedSectorOffset += nandAddress.SectorCount;
            _CurrentLba = nextLba;
            _RemainingSectorCount = remainingSectorCount;
        }
        else
        {
            break;
        }
    }
}

void SimpleFtl::TransferOut(const Buffer& buffer, const tSectorOffset& bufferOffset, const tSectorOffset& commandOffset, const tSectorCount& sectorCount)
{
    CustomProtocolHal::TransferCommandDesc transferCommand;
    transferCommand.Buffer = buffer;
    transferCommand.BufferOffset = bufferOffset;  //NOTE: if NAND sector and buffer sector ever differ, need a conversion
    transferCommand.Command = _ProcessingCommand;
    transferCommand.Direction = CustomProtocolHal::TransferCommandDesc::Direction::Out;
    transferCommand.CommandOffset = commandOffset;
    transferCommand.SectorCount = sectorCount;
    transferCommand.Listener = this;
    _CustomProtocolHal->QueueCommand(transferCommand);
    ++_PendingTransferCommandCount;
}

void SimpleFtl::ReadPage(const NandHal::NandAddress& nandAddress, const Buffer& outBuffer, const tSectorOffset& descSectorIndex)
{
    assert((nandAddress.Sector + nandAddress.SectorCount) <= _SectorsPerPage);

    NandHal::CommandDesc commandDesc;
    commandDesc.Address = nandAddress;
    commandDesc.Operation = (nandAddress.SectorCount == _SectorsPerPage)
        ? NandHal::CommandDesc::Op::Read : NandHal::CommandDesc::Op::ReadPartial;
    commandDesc.Buffer = outBuffer;
    commandDesc.BufferOffset = nandAddress.Sector;  //NOTE: if NAND sector and buffer sector ever differ, need a conversion
    commandDesc.DescSectorIndex = descSectorIndex;
    commandDesc.Listener = this;

    _NandHal->QueueCommand(commandDesc);
    ++_PendingNandCommandCount;
}

void SimpleFtl::HandleWriteCommand()
{
    // Calculate starting block
    Buffer buffer;
    U32 nextLba, remainingSectorCount, startingPage;
    SimpleFtlTranslation::LbaToNandAddress(_CurrentLba, 1,
        _ProcessingPage, nextLba, remainingSectorCount);
    startingPage = _ProcessingPage.Page;

    // Read head pages of the starting block
    _ProcessingPage.Sector = 0;
    _ProcessingPage.SectorCount = _SectorsPerPage;
    for (_ProcessingPage.Page = 0; _ProcessingPage.Page < startingPage; ++_ProcessingPage.Page)
    {
        AllocateNextBuffer(buffer);
        ReadPage(_ProcessingPage, buffer, tSectorOffset{ 0 });
    }

    // Transfer data from host
    // First writing page
    // Read head sectors of the page
    SimpleFtlTranslation::LbaToNandAddress(_CurrentLba, _RemainingSectorCount,
        _ProcessingPage, _CurrentLba, _RemainingSectorCount);
    AllocateNextBuffer(buffer);
    if (_ProcessingPage.Sector > 0)
    {
        // Starting LBA doesn't align to a page. Starting page has head sectors.
        // Read head sectors of the page
        NandHal::NandAddress temp = _ProcessingPage;
        temp.Sector = 0;
        temp.SectorCount = _ProcessingPage.Sector;
        ReadPage(temp, buffer, tSectorOffset{ 0 });
    }
    // Transfer in sectors of the starting page
    TransferIn(buffer, tSectorOffset{ _ProcessingPage.Sector }, _ProcessedSectorOffset, _ProcessingPage.SectorCount);
    _ProcessingBlocks[0] = _ProcessingPage;
    ++_ProcessingBlockCount;

    // Transfer in the middle pages
    NandHal::NandAddress previousPage;
    _ProcessingPage.Sector = 0;
    _ProcessingPage.SectorCount = _SectorsPerPage;
    while (_RemainingSectorCount > 0)
    {
        previousPage = _ProcessingPage;
        SimpleFtlTranslation::LbaToNandAddress(_CurrentLba, _RemainingSectorCount,
            _ProcessingPage, _CurrentLba, _RemainingSectorCount);
        AllocateNextBuffer(buffer);
        TransferIn(buffer, tSectorOffset{ _ProcessingPage.Sector }, _ProcessedSectorOffset, _ProcessingPage.SectorCount);

        if (false == IsSameBlock(previousPage, _ProcessingPage))
        {
            // Move to new block
            _ProcessingBlocks[_ProcessingBlockCount] = _ProcessingPage;
            ++_ProcessingBlockCount;
        }
    }

    U32 tailStartingSector = _ProcessingPage.Sector + _ProcessingPage.SectorCount;
    if (_RemainingSectorCount == 0 && tailStartingSector < _SectorsPerPage)
    {
        // Ending LBA doesn't align to a page. Ending page has tail sectors.
        // Read tail sectors of the page
        _ProcessingPage.Sector = tailStartingSector;
        _ProcessingPage.SectorCount = _SectorsPerPage - tailStartingSector;
        ReadPage(_ProcessingPage, buffer, tSectorOffset{ tailStartingSector });
    }

    // Read tail pages of the ending block
    ++_ProcessingPage.Page;
    _ProcessingPage.Sector = 0;
    _ProcessingPage.SectorCount = _SectorsPerPage;
    for (; _ProcessingPage.Page < _PagesPerBlock; ++_ProcessingPage.Page)
    {
        AllocateNextBuffer(buffer);
        ReadPage(_ProcessingPage, buffer, tSectorOffset{ 0 });
    }
}

void SimpleFtl::AllocateNextBuffer(Buffer &buffer)
{
    bool success = _BufferHal->AllocateBuffer(BufferType::User, _BlockBuffers[_BlockBufferCount]);
    buffer = _BlockBuffers[_BlockBufferCount];
    assert(success == true);        // MUST success
    ++_BlockBufferCount;
}

void SimpleFtl::HandleNandReadAndTransferCompleted()
{
    if (0 == _PendingNandCommandCount && 0 == _PendingTransferCommandCount)
    {
        // All read commands and transfer command are completed
        // All data are in buffers
        // Submit erase command
        for (U32 i = 0; i < _ProcessingBlockCount; ++i)
        {
            EraseBlock(_ProcessingBlocks[i]);
        }
    }
}

void SimpleFtl::HandleNandEraseCompleted()
{
    if (0 == _PendingNandCommandCount)
    {
        // All blocks are erased
        // Submit write data from buffers to erased blocks
        U32 index = 0;
        NandHal::NandAddress address;
        for (U32 i = 0; i < _ProcessingBlockCount; ++i)
        {
            address = _ProcessingBlocks[i];
            address.Sector = 0;
            address.SectorCount = _SectorsPerPage;
            for (address.Page = 0; address.Page < _PagesPerBlock; ++address.Page, ++index)
            {
                assert(index < _BlockBufferCount);
                WritePage(address, _BlockBuffers[index]);
            }
        }
    }
}

bool SimpleFtl::IsSameBlock(const NandHal::NandAddress& nandAddress1, const NandHal::NandAddress& nandAddress2)
{
    return (nandAddress1.Channel == nandAddress2.Channel)
        && (nandAddress1.Device == nandAddress2.Device)
        && (nandAddress1.Block == nandAddress2.Block);
}

void SimpleFtl::TransferIn(const Buffer& buffer, const tSectorOffset& bufferOffset, const tSectorOffset& commandOffset, const tSectorCount& sectorCount)
{
    CustomProtocolHal::TransferCommandDesc transferCommand;
    transferCommand.Buffer = buffer;
    transferCommand.BufferOffset = bufferOffset;  //NOTE: if NAND sector and buffer sector ever differ, need a conversion
    transferCommand.Command = _ProcessingCommand;
    transferCommand.Direction = CustomProtocolHal::TransferCommandDesc::Direction::In;
    transferCommand.CommandOffset = commandOffset;
    transferCommand.SectorCount = sectorCount;
    transferCommand.Listener = this;
    _CustomProtocolHal->QueueCommand(transferCommand);

    _ProcessedSectorOffset += sectorCount;
    ++_PendingTransferCommandCount;
}

void SimpleFtl::WritePage(const NandHal::NandAddress& nandAddress, const Buffer& inBuffer)
{
    assert((nandAddress.Sector + nandAddress.SectorCount) <= _SectorsPerPage);

    NandHal::CommandDesc commandDesc;
    commandDesc.Address = nandAddress;
    commandDesc.Operation = (nandAddress.SectorCount == _SectorsPerPage)
        ? NandHal::CommandDesc::Op::Write : NandHal::CommandDesc::Op::WritePartial;
    commandDesc.Buffer = inBuffer;
    commandDesc.BufferOffset = nandAddress.Sector;  //NOTE: if NAND sector and buffer sector ever differ, need a conversion
    commandDesc.Listener = this;

    _NandHal->QueueCommand(commandDesc);
    ++_PendingNandCommandCount;
}

void SimpleFtl::EraseBlock(const NandHal::NandAddress& nandAddress)
{
    NandHal::CommandDesc commandDesc;
    commandDesc.Address = nandAddress;
    commandDesc.Operation = NandHal::CommandDesc::Op::Erase;
    commandDesc.Listener = this;

    _NandHal->QueueCommand(commandDesc);
    ++_PendingNandCommandCount;
}

void SimpleFtl::OnTransferCommandCompleted(const CustomProtocolHal::TransferCommandDesc& command)
{
    if (CustomProtocolCommand::Code::Read == _ProcessingCommand->Command)
    {
        _BufferHal->DeallocateBuffer(command.Buffer);
        --_PendingTransferCommandCount;

        if (_RemainingSectorCount == 0 && _PendingTransferCommandCount == 0 && _PendingNandCommandCount == 0)
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
        --_PendingTransferCommandCount;
        HandleNandReadAndTransferCompleted();
    }
}

void SimpleFtl::OnNandCommandCompleted(const NandHal::CommandDesc& command)
{
    if (CustomProtocolCommand::Code::Read == _ProcessingCommand->Command)
    {
        TransferOut(command.Buffer, tSectorOffset{ command.Address.Sector }, command.DescSectorIndex, command.Address.SectorCount);
        --_PendingNandCommandCount;

        if (NandHal::CommandDesc::Status::Success != command.CommandStatus)
        {
            _ProcessingCommand->CommandStatus = CustomProtocolCommand::Status::ReadError;
        }
    }
    else
    {
        if (NandHal::CommandDesc::Op::Read == command.Operation 
            || NandHal::CommandDesc::Op::ReadPartial == command.Operation)
        {
            --_PendingNandCommandCount;
            HandleNandReadAndTransferCompleted();
        }
        else if (NandHal::CommandDesc::Op::Erase == command.Operation)
        {
            --_PendingNandCommandCount;
            HandleNandEraseCompleted();
        }
        else if (NandHal::CommandDesc::Op::Write == command.Operation)
        {
            if (NandHal::CommandDesc::Status::Success != command.CommandStatus)
            {
                _ProcessingCommand->CommandStatus = CustomProtocolCommand::Status::WriteError;
            }
            _BufferHal->DeallocateBuffer(command.Buffer);
            --_PendingNandCommandCount;

            if (_RemainingSectorCount == 0 && _PendingNandCommandCount == 0 && _PendingTransferCommandCount == 0)
            {
                SubmitResponse();
            }
        }
    }
}

void SimpleFtl::SubmitCustomProtocolCommand(CustomProtocolCommand* command)
{
    Event event;
    event.EventType = Event::Type::CustomProtocolCommand;
    event.EventParams.CustomProtocolCommand = command;
    _EventQueue->push(event);
}

void SimpleFtl::HandleCommandCompleted(const CustomProtocolHal::TransferCommandDesc& command)
{
    Event event;
    event.EventType = Event::Type::TransferCompleted;
    event.EventParams.TransferCommand = command;
    assert(_EventQueue->push(event) == true);
}

void SimpleFtl::HandleCommandCompleted(const NandHal::CommandDesc& command)
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