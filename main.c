#include <efi.h>
#include <efilib.h>

static VOID
PrintDevicePath(EFI_DEVICE_PATH *Path)
{
    CHAR16 *Str = DevicePathToStr(Path);
    if (Str) {
        Print(L"    Path: %s\n", Str);
        FreePool(Str);
    }
}

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    InitializeLib(ImageHandle, SystemTable);

    EFI_STATUS Status;
    EFI_HANDLE *Handles;
    UINTN HandleCount;

    Status = uefi_call_wrapper(
        BS->LocateHandleBuffer,
        5,
        AllHandles,
        NULL,
        NULL,
        &HandleCount,
        &Handles
    );
    if (EFI_ERROR(Status)) {
        Print(L"LocateHandleBuffer failed\n");
        return Status;
    }

    Print(L"Found %d handles\n\n", HandleCount);

    for (UINTN i = 0; i < HandleCount; i++) {

        EFI_DEVICE_PATH *Path;
        BS->HandleProtocol(
            Handles[i],
            &gEfiDevicePathProtocolGuid,
            (VOID **)&Path
        );

        BOOLEAN Printed = FALSE;

        /* ---------- STORAGE ---------- */
        EFI_BLOCK_IO_PROTOCOL *BlockIo;
        if (!EFI_ERROR(BS->HandleProtocol(
            Handles[i],
            &gEfiBlockIoProtocolGuid,
            (VOID **)&BlockIo)))
        {
            if (BlockIo->Media->MediaPresent) {
                Print(L"[STORAGE]\n");
                if(BlockIo->Media->LogicalPartition) {
                    Print(L"  Logical Partition\n");
                } else {
                    Print(L"  Physical Drive\n");
                }
                if(BlockIo->Media->RemovableMedia) {
                    Print(L"  Removable Media\n");
                } else {
                    Print(L"  Fixed Media\n");
                }
                Printed = TRUE;
            }
        }

        /* ---------- NETWORK ---------- */
        EFI_SIMPLE_NETWORK_PROTOCOL *Net;
        if (!EFI_ERROR(BS->HandleProtocol(
            Handles[i],
            &gEfiSimpleNetworkProtocolGuid,
            (VOID **)&Net)))
        {
            Print(L"[NETWORK]\n");
            Printed = TRUE;
        }

        /* ---------- DISPLAY ---------- */
        EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;
        if (!EFI_ERROR(BS->HandleProtocol(
            Handles[i],
            &gEfiGraphicsOutputProtocolGuid,
            (VOID **)&Gop)))
        {
            Print(L"[DISPLAY]\n");
            if(Gop->Mode->Info) {
                Print(L"  Mode: %dx%d\n", Gop->Mode->Info->HorizontalResolution, Gop->Mode->Info->VerticalResolution);
            }
            if(Gop->Mode->FrameBufferBase) {
                Print(L"  Framebuffer: %p\n", Gop->Mode->FrameBufferBase);
            }
            if(Gop->Mode->FrameBufferSize) {
                Print(L"  Framebuffer Size: %d bytes\n", Gop->Mode->FrameBufferSize);
            }
            if(Gop->Mode->Info->PixelFormat) {
                Print(L"  Pixel Format: %d\n", Gop->Mode->Info->PixelFormat);
            }
            Printed = TRUE;
        }

        /* ---------- UART / SERIAL ---------- */
        EFI_SERIAL_IO_PROTOCOL *Serial;
        if (!EFI_ERROR(BS->HandleProtocol(
            Handles[i],
            &gEfiSerialIoProtocolGuid,
            (VOID **)&Serial)))
        {
            Print(L"[UART]\n");
            if(Serial->Mode->BaudRate) {
                Print(L"  Baud Rate: %d\n", Serial->Mode->BaudRate);
            }
            if(Serial->Mode->Parity) {
                Print(L"  Parity: %d\n", Serial->Mode->Parity);
            }
            if(Serial->Mode->DataBits) {
                Print(L"  Data Bits: %d\n", Serial->Mode->DataBits);
            }
            if(Serial->Mode->Timeout) {
                Print(L"  Timeout: %d\n", Serial->Mode->Timeout);
            }
            Printed = TRUE;
        }

        if (Printed) {
            Print(L"  Handle: %p\n", Handles[i]);
            if (Path) {
                PrintDevicePath(Path);
            }
            Print(L"\n");
        }
    }

    while(1);
}
