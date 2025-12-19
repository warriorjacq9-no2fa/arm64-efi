#!/usr/bin/env bash
set -euo pipefail

if [ $# -ne 2 ]; then
    echo "Usage: $0 <arch> <efi_application>"
    echo "Arch: i386 | x86_64 | aarch64"
    exit 1
fi

ARCH="$2"
EFI_APP="$(realpath "$1")"

if [ ! -f "$EFI_APP" ]; then
    echo "Error: EFI application not found: $EFI_APP"
    exit 1
fi

case "$ARCH" in
    x86_64)
        QEMU="qemu-system-x86_64"
        BOOT_NAME="BOOTX64.EFI"
        CODE="/usr/share/OVMF/OVMF_CODE_4M.fd"
        VARS="/usr/share/OVMF/OVMF_VARS_4M.fd"
        MACHINE_OPTS="-machine q35"
        CPU_OPTS="-cpu qemu64"
        ;;
    i386)
        QEMU="qemu-system-i386"
        BOOT_NAME="BOOTIA32.EFI"
        CODE="/usr/share/OVMF/OVMF_CODE.fd"
        VARS="/usr/share/OVMF/OVMF_VARS.fd"
        MACHINE_OPTS="-machine q35"
        CPU_OPTS="-cpu qemu32"
        ;;
    aarch64)
        QEMU="qemu-system-aarch64"
        BOOT_NAME="BOOTAA64.EFI"
        CODE="/usr/share/AAVMF/AAVMF_CODE.fd"
        VARS="/usr/share/AAVMF/AAVMF_VARS.fd"
        MACHINE_OPTS="-machine virt"
        CPU_OPTS="-cpu cortex-a57"
        ;;
    *)
        echo "Unsupported architecture: $ARCH"
        exit 1
        ;;
esac

for f in "$CODE" "$VARS"; do
    if [ ! -f "$f" ]; then
        echo "Missing UEFI firmware: $f"
        exit 1
    fi
done

TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT

mkdir -p "$TMPDIR/EFI/BOOT"
cp "$EFI_APP" "$TMPDIR/EFI/BOOT/$BOOT_NAME"
cp kernel "$TMPDIR/kernel.img"
cp "$VARS" "$TMPDIR/vars.fd"

echo "Running EFI application"
echo "  Arch : $ARCH"
echo "  QEMU : $QEMU"
echo "  Boot : EFI/BOOT/$BOOT_NAME"

exec "$QEMU" \
    $MACHINE_OPTS \
    $CPU_OPTS \
    -m 512M \
    -drive if=pflash,format=raw,readonly=on,file="$CODE" \
    -drive if=pflash,format=raw,file="$TMPDIR/vars.fd" \
    -drive format=raw,file=fat:rw:"$TMPDIR" \
    -nographic
