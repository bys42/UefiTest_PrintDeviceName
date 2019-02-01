#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/ComponentName2.h>

EFI_HANDLE
FindAgent (
    IN EFI_HANDLE Handle,
    IN EFI_HANDLE Opener,
    IN EFI_GUID   *Guid
    )
{
    EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *Info;
    EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *InfoStart;
    UINTN                               Count;
    EFI_HANDLE                          Agent = NULL;

    if (Handle == NULL || Opener == NULL || Guid == NULL) {
        return NULL;
    }

    if (gBS->OpenProtocolInformation (Handle, Guid, &InfoStart, &Count) != EFI_SUCCESS) {
        return NULL;
    }

    for (Info = InfoStart; Count-- > 0; Info++) {
        if (Info->ControllerHandle == Opener) {
            Agent = Info->AgentHandle;
            break;
        }
    }
    gBS->FreePool (InfoStart);
    return Agent;
}

VOID
PrintDeviceName(
    IN EFI_HANDLE AgentHandle,
    IN EFI_HANDLE Controller,
    IN EFI_HANDLE Child,
    IN CHAR8      *Lang
    )
{
    EFI_COMPONENT_NAME2_PROTOCOL *CompName2;
    CHAR16                       *ControllerName;
    EFI_STATUS                   Status;

    if (AgentHandle == NULL || Controller == NULL) {
        return;
    }

    Status = gBS->OpenProtocol(
        AgentHandle,
        &gEfiComponentName2ProtocolGuid,
        (VOID**)&CompName2,
        NULL,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(Status)) {
        Print(L"Open CompName2: %r\n", Status);
        return;
    }

    Status = CompName2->GetControllerName(CompName2, Controller, Child, Lang, &ControllerName);
    if (Status == EFI_SUCCESS) {
        Print(L"ControllerName: %s\n", ControllerName);
    } else {
        Print(L"ControllerName: Unkown, %r\n", Status);
    }

    return;
}

EFI_STATUS
TestEntry(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
    )
{
    EFI_HANDLE *FsHandles = NULL;
    EFI_HANDLE *UsbIoHandles = NULL;
    EFI_HANDLE *UsbHcHandles = NULL;
    EFI_HANDLE *ParentUsbIo;
    EFI_HANDLE *ParentUsbHc;
    EFI_HANDLE AgentHandle;
    UINTN      FsCount;
    UINTN      UsbHcCount;
    UINTN      UsbIoCount;
    EFI_STATUS Status;

    // only 1 usb fs in test env.
    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &FsCount, &FsHandles);
    if (EFI_ERROR(Status) || FsCount > 1) {
        goto Exit;
    }

    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiUsbIoProtocolGuid, NULL, &UsbIoCount, &UsbIoHandles);
    if (EFI_ERROR(Status)) {
        goto Exit;
    }

    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiUsb2HcProtocolGuid, NULL, &UsbHcCount, &UsbHcHandles);
    if (EFI_ERROR(Status)) {
        goto Exit;
    }

    AgentHandle = FindAgent(*FsHandles, *FsHandles, &gEfiDiskIoProtocolGuid);
    if (AgentHandle == NULL) {
        goto Exit;
    }
    PrintDeviceName(AgentHandle, *FsHandles, NULL, "en");

    for (ParentUsbIo = UsbIoHandles; UsbIoCount-- > 0; ParentUsbIo++) {
        AgentHandle = FindAgent(*ParentUsbIo, *FsHandles, &gEfiDiskIoProtocolGuid);
        if (AgentHandle != NULL) {
            break;
        }
    }
    if (AgentHandle == NULL) {
        goto Exit;
    }

    for (ParentUsbHc = UsbHcHandles; UsbHcCount-- > 0; ParentUsbHc++) {
        AgentHandle = FindAgent(*ParentUsbHc, *ParentUsbIo, &gEfiUsb2HcProtocolGuid);
        if (AgentHandle != NULL) {
            PrintDeviceName(AgentHandle, *ParentUsbHc, *ParentUsbIo, "en-US");
            break;
        }
    }

Exit:
    gBS->FreePool(FsHandles);
    gBS->FreePool(UsbIoHandles);
    gBS->FreePool(UsbHcHandles);
    return EFI_SUCCESS;
}
