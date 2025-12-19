CC=aarch64-linux-gnu-gcc
LD=aarch64-linux-gnu-ld
OBJCOPY=aarch64-linux-gnu-objcopy
ARCH=aarch64

.PHONY: test
test: main.efi
	./runefi.sh main.efi $(ARCH)

main.efi: main.c
	$(CC) -I../gnu-efi/inc -I../gnu-efi/inc/$(ARCH) -fpic -ffreestanding -fno-stack-protector \
		-fno-stack-check -fshort-wchar \
		-c main.c -o main.o
	$(LD) -shared -Bsymbolic -L../gnu-efi/$(ARCH)/lib -L../gnu-efi/$(ARCH)/gnuefi \
		-T../gnu-efi/gnuefi/elf_$(ARCH)_efi.lds ../gnu-efi/$(ARCH)/gnuefi/crt0-efi-$(ARCH).o \
		main.o -o main.so -lgnuefi -lefi
	$(OBJCOPY) -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym \
		-j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-$(ARCH) \
		--subsystem=10 main.so main.efi

kernel: kernel.c
	aarch64-linux-gnu-gcc -nostdlib -ffreestanding -fno-pie -fPIC -T linker.ld kernel.c -o kernel.elf
	aarch64-linux-gnu-objcopy -O binary kernel.elf kernel

clean:
	rm -f *.img *.efi *.so *.o