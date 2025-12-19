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

    EFI_LOADED_IMAGE *LoadedImage;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FS;
    EFI_FILE_HANDLE Root, KernelFile;

    /* Loaded Image */
    Status = uefi_call_wrapper(
        SystemTable->BootServices->HandleProtocol,
        3,
        ImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (void **)&LoadedImage
    );
    if (EFI_ERROR(Status)) return Status;

    /* Filesystem */
    Status = uefi_call_wrapper(
        SystemTable->BootServices->HandleProtocol,
        3,
        LoadedImage->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (void **)&FS
    );
    if (EFI_ERROR(Status)) return Status;

    Status = uefi_call_wrapper(FS->OpenVolume, 2, FS, &Root);
    if (EFI_ERROR(Status)) return Status;

    /* Open kernel.img */
    Status = uefi_call_wrapper(
        Root->Open,
        5,
        Root,
        &KernelFile,
        L"kernel.img",
        EFI_FILE_MODE_READ,
        0
    );
    if (EFI_ERROR(Status)) {
        Print(L"Cannot open kernel.img\n");
        return Status;
    }

    /* File size */
    EFI_FILE_INFO *Info;
    UINTN InfoSize = SIZE_OF_EFI_FILE_INFO + 200;

    Info = AllocatePool(InfoSize);
    Status = uefi_call_wrapper(
        KernelFile->GetInfo,
        4,
        KernelFile,
        &gEfiFileInfoGuid,
        &InfoSize,
        Info
    );
    if (EFI_ERROR(Status)) return Status;

    UINTN KernelSize = Info->FileSize;

    /* Allocate kernel memory */
    VOID *KernelBuffer;
    Status = uefi_call_wrapper(
        SystemTable->BootServices->AllocatePages,
        4,
        AllocateAnyPages,
        EfiLoaderData,
        EFI_SIZE_TO_PAGES(KernelSize),
        (EFI_PHYSICAL_ADDRESS *)&KernelBuffer
    );
    if (EFI_ERROR(Status)) return Status;

    /* Read kernel */
    Status = uefi_call_wrapper(
        KernelFile->Read,
        3,
        KernelFile,
        &KernelSize,
        KernelBuffer
    );
    if (EFI_ERROR(Status)) return Status;

    Print(L"Kernel loaded at %p (%lu bytes)\n",
          KernelBuffer, KernelSize);

    /* Exit boot services */
    UINTN MapSize = 0, MapKey, DescSize;
    UINT32 DescVersion;
    EFI_MEMORY_DESCRIPTOR *Map = NULL;

    uefi_call_wrapper(
        SystemTable->BootServices->GetMemoryMap,
        5,
        &MapSize, Map, &MapKey, &DescSize, &DescVersion
    );

    Map = AllocatePool(MapSize);

    Status = uefi_call_wrapper(
        SystemTable->BootServices->GetMemoryMap,
        5,
        &MapSize, Map, &MapKey, &DescSize, &DescVersion
    );
    if (EFI_ERROR(Status)) return Status;

    Status = uefi_call_wrapper(
        SystemTable->BootServices->ExitBootServices,
        2,
        ImageHandle,
        MapKey
    );
    if (EFI_ERROR(Status)) return Status;

    /* Jump to kernel */
    void (*kernel_entry)(void) =
        (void (*)(void))KernelBuffer;

    kernel_entry();


    while(1);
}
