#include <efi.h>
#include <efilib.h>
EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);

    Print(L"GNU-EFI Loader starting...\n");

    EFI_STATUS Status;
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

    while (1);
}
