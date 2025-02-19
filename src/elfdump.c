#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define BOOTROM_MEM_ADDR    0x80000000
#define BOOTROM_MEM_END     0x80002000

#ifndef BOOTROM_MEM_ALT
// If the application is linked to be loaded at BOOTROM_MEM_ADDR:
//   BOOTROM_MEM_ALT is used as temporary storage
//   system RAM size must be >= 128MB
//   the application size must be <= (128MB - 8KB)
#define BOOTROM_MEM_ALT     0x87ffe000

#endif

 static uintptr_t read_num(FILE* fd, unsigned size) {
     uint8_t buf[0x10];
     uintptr_t v = 0;
     unsigned n = 0;
     uint8_t rd;

     if (fread(buf,1,size, fd) != size) return 0;

     for (n = 0; n < size; n++) {
        v |= (uintptr_t)(buf[n]) << (n * 8);
      }
     return v;

}

#define read_uint16(a) (uint16_t)read_num(a, 2)
#define read_uint32(a) (uint32_t)read_num(a, 4)
#define read_addr(a) (uintptr_t)read_num(a, 64 >> 3)

#define PT_LOAD 1

static int download(void) {
    const char *fnm = "boot.elf";
    char phent_fn[] = "phent_x\0";
    uint8_t buf[0x10];
    uint8_t mem[0x10000];
    uintptr_t entry_addr = 0;
    uint64_t phoff = 0;
    uint16_t phentsize = 0;
    uint16_t phnum = 0;
    unsigned i = 0;
    FILE* fd;
    fd = fopen(fnm, "r");

    if (fd == NULL) return -1;

    unsigned rd = 0;
    if (fgets(buf,7,fd) == NULL) {
       printf("ERROR: read\n");
       return -1;
    }

    if (buf[0] != 0x7f || buf[1] != 'E' || buf[2] != 'L' || buf[3] != 'F') {

        printf("ERROR: Read Magic number\n");
        printf("%x, %x, %x, %x\n",buf[0], buf[1], buf[2], buf[3]);        //errno = ERR_NOT_ELF;
        return -1;
    }
    printf("ELF MAGIC OK\n");

    if (buf[4] == 1 )
        printf("32bit\n");
    else if (buf[4]== 2)
        printf("64bit\n");
    else
        printf("INVALID bit size\n");

   if (buf[5] == 1)
        printf("LITTLE ENDIAN\n");
    else if (buf[5] == 2)
      printf("BIF ENDIAN\n");
    else {
        printf("INVALID endianess\n");
         printf("%x\n",buf[5]);
    }

    fseek(fd,24, SEEK_SET);
    entry_addr = read_addr(fd);
    printf("entry_addr %#0x\n", entry_addr);

    phoff = read_addr(fd);
    printf("phoff %16x\n", phoff);
    //printf("offset %08x\n", ftell(fd));

    int  tmp = ftell(fd) + (64>>3) + 6;
    fseek(fd,tmp, SEEK_SET);
    phentsize = read_uint16(fd);
    printf("phentsize %08x\n", phentsize);
    phnum = read_uint16(fd);
    printf("phnum %08x\n", phnum);

    for (i = 0; i < phnum; i++) {
        printf("\nPhysical entry %d\n",i);
        uint32_t p_type = 0;
        uint64_t p_offset = 0;
        uint64_t p_vaddr = 0;
        uint64_t p_filesz = 0;
        uint64_t p_memsz = 0;
        uintptr_t addr = 0;
        size_t pos = 0;
        fseek(fd, phoff + i * phentsize, SEEK_SET);

        printf("Phent offset %#10x\n", ftell(fd));

        p_type = read_uint32(fd);
        printf("p_type %08x\n", p_type);

        if (p_type != 1) continue;

        FILE* phent_fd;
        phent_fn[6] = i+48;
        phent_fd = fopen(phent_fn, "w");
        if (phent_fd == NULL) return -1;

        fseek(fd, ftell(fd) + (64 == 32 ? 0 : 4), SEEK_SET);
        p_offset = read_addr(fd);
        printf("p_offset %#0x\n", p_offset);
        p_vaddr = read_addr(fd);
        printf("p_vaddrr %#0x\n", p_vaddr);

        //not part of code
        long int tmp1 = read_addr(fd); /* p_paddr */
        printf("p_addrr %#0x\n", tmp1);

        p_filesz = read_addr(fd);
        printf("p_filesz %#0x\n", p_filesz);
        p_memsz = read_addr(fd);
        printf("p_memsz %#0x\n", p_memsz);


        fseek(fd, p_offset, SEEK_SET);
        addr = p_vaddr;
        size_t size = 0x10000;
        while (pos < p_filesz && pos < p_memsz) {
            printf("-> begin reading file");
            printf("-> pos %x\n", pos);
            printf("-> p_filesz %x\n", p_filesz);
            printf("-> p_filesz %x\n", p_memsz);
            if (size > p_filesz - pos) {
                size = (size_t)(p_filesz - pos);
                printf("-> size =  p_filezsz = %x\n", size);
            }
            if (size > p_memsz - pos) {
                size = (size_t)(p_memsz - pos);
                printf("-> size = pmemsz - pos =  %x\n", size);
            }
            printf("-> size  %x\n", size);

            size_t rn = fread(mem, 1, size, fd);
            printf("read %x\n", rn );
            size_t wn = fwrite(mem, 1,size,  phent_fd);
            printf("write %x\n", wn );

            pos += size;

        };
        // break;
        //
        // while (pos < p_memsz) {
        //     uint8_t * mem = (void *)addr;
        //     size_t size = 0x10000;
        //     if (size > p_memsz - pos) size = (size_t)p_memsz - pos;
        //     if (addr + size > BOOTROM_MEM_ADDR && addr < BOOTROM_MEM_END) {
        //         mem = (uint8_t *)(addr - BOOTROM_MEM_ADDR + BOOTROM_MEM_ALT);
        //         if (addr + size > BOOTROM_MEM_END) size = BOOTROM_MEM_END - addr;
        //        // alt_mem = 1;
        //     }
        //     size_t s = size;
        //     while (s) {
        //         *mem++ = 0;
        //         s--;
        //     }
        //     addr += size;
        //     pos += size;
        //}
        fclose(phent_fd);
   }
   fclose(fd);
   return 0;
}

int main(void) {

     printf("elf-dump\n");
     if (download() != 0) {
            printf("Cannot read BOOT.ELF: %s\n", "ME");
    }
    return 0;
}
