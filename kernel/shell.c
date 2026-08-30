#include "shell.h"
#include "vga.h"
#include "utypes.h"
#include "io.h"
#include "drivers/storage/ata.h"
#include "kernel/memory/pmm.h"
#include "kernel/memory/e820.h"

extern char __kernel_start;
extern char __kernel_end;

#define KERNEL_MAX_SIZE 24576
#define KERNEL_WARNING_SIZE 20480
#define ATA_TEST_LBA 100

static int string_equals(const char *a, const char *b) {
    uint16_t i = 0;

    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return 0;
        }

        i++;
    }

    return a[i] == b[i];
}

static int string_starts_with(const char *text, const char *prefix) {
    uint16_t i = 0;

    while (prefix[i] != '\0') {
        if (text[i] != prefix[i]) {
            return 0;
        }

        i++;
    }

    return 1;
}

static void command_reboot(void) {
    __asm__ volatile ("cli");

    while (inb(0x64) & 0x02) {
    }

    outb(0x64, 0xFE);

    for (;;) {
        __asm__ volatile ("hlt");
    }
}

static void command_atatest(void) {
    uint8_t boot_sector[512];
    uint8_t write_buffer[512];
    uint8_t read_buffer[512];

    if (ata_init() != 0) {
        return;
    }

    vga_print("ATA detectado: ");
    vga_print_uint(ata_get_sector_count());
    vga_print(" setores");
    vga_newline(1);

    if (ata_read_sector(0, boot_sector) != 0) {
        return;
    }

    if (boot_sector[510] != 0x55 || boot_sector[511] != 0xAA) {
        vga_print("ATA: assinatura do setor de boot invalida");
        vga_newline(1);
        return;
    }

    vga_print("ATA: leitura do setor de boot OK");
    vga_newline(1);

    if (ata_get_sector_count() <= ATA_TEST_LBA) {
        vga_print("ATA: imagem pequena demais para o setor de teste");
        vga_newline(1);
        return;
    }

    for (uint16_t i = 0; i < 512; i++) {
        write_buffer[i] = (uint8_t)((i * 37u + 0x5Au) & 0xFFu);
        read_buffer[i] = 0;
    }

    if (ata_write_sector(ATA_TEST_LBA, write_buffer) != 0) {
        return;
    }

    if (ata_read_sector(ATA_TEST_LBA, read_buffer) != 0) {
        return;
    }

    for (uint16_t i = 0; i < 512; i++) {
        if (read_buffer[i] != write_buffer[i]) {
            vga_print("ATA: verificacao de escrita falhou no byte ");
            vga_print_uint(i);
            vga_newline(1);
            return;
        }
    }

    vga_print("ATA: escrita, flush e leitura no LBA 100 OK");
    vga_newline(1);
}

static void command_pmmtest(void) {
    uint32_t free_before = pmm_get_free_frames();
    uint32_t used_before = pmm_get_used_frames();
    uint32_t first_frame = pmm_alloc_frame();
    uint32_t second_frame = pmm_alloc_frame();

    vga_print("PMM total: ");
    vga_print_uint(pmm_get_total_frames());
    vga_print(" paginas");
    vga_newline(1);

    vga_print("PMM livres: ");
    vga_print_uint(pmm_get_free_frames());
    vga_print(" paginas");
    vga_newline(1);

    vga_print("PMM ocupadas: ");
    vga_print_uint(pmm_get_used_frames());
    vga_print(" paginas");
    vga_newline(1);
    
    if (first_frame == 0) {
        vga_print("PMM: falha na primeira alocacao");
        vga_newline(1);
        return;
    }

    if (second_frame == 0) {
        pmm_free_frame(first_frame);

        vga_print("PMM: falha na segunda alocacao");
        vga_newline(1);
        return;
    }

    if (first_frame == second_frame) {
        vga_print("PMM: paginas repetidas");
        vga_newline(1);
        return;
    }

    vga_print("PMM pagina 1: 0x");
    vga_print_hex(first_frame);
    vga_newline(1);

    vga_print("PMM pagina 2: 0x");
    vga_print_hex(second_frame);
    vga_newline(1);

    int first_result = pmm_free_frame(first_frame);
    int second_result = pmm_free_frame(second_frame);

    if (first_result != 0 || second_result != 0) {
        vga_print("PMM: Teste falhou");
        vga_newline(1);
        return;
    }

    if (pmm_get_free_frames() != free_before || pmm_get_used_frames() != used_before) {

        vga_print("PMM: contadores nao foram restaurados");
        vga_newline(1);
        return;
    }

    vga_print("PMM: Teste concluido");
    vga_newline(1);
}

static void command_e820test(void) {
    uint16_t count = e820_get_entry_count();

    const volatile e820_entry_t *entries =
        e820_get_entries();

    vga_print("E820 entradas: ");
    vga_print_uint(count);
    vga_newline(1);

    if (count == 0) {
        vga_print("E820: mapa indisponivel");
        vga_newline(1);
        return;
    }

    for (uint16_t i = 0; i < count; i++) {
        vga_print("#");
        vga_print_uint(i);

        vga_print(" base=0x");
        vga_print_hex((uint32_t)entries[i].base);

        vga_print(" tamanho=0x");
        vga_print_hex((uint32_t)entries[i].length);

        vga_print(" tipo=");
        vga_print_uint(entries[i].type);

        vga_newline(1);
    }
}

static void command_color(const char *color) {
    int color_found = 0;

    if (string_equals(color, "red")) {
        vga_set_color(VGA_RED, VGA_BLACK);
        color_found = 1;
    }

    if (string_equals(color, "green")) {
        vga_set_color(VGA_GREEN, VGA_BLACK);
        color_found = 1;
    }

    if (string_equals(color, "blue")) {
        vga_set_color(VGA_BLUE, VGA_BLACK);
        color_found = 1;
    }

    if (string_equals(color, "cyan")) {
        vga_set_color(VGA_CYAN, VGA_BLACK);
        color_found = 1;
    }

    if (string_equals(color, "magenta")) {
        vga_set_color(VGA_MAGENTA, VGA_BLACK);
        color_found = 1;
    }

    if (string_equals(color, "yellow")) {
        vga_set_color(VGA_YELLOW, VGA_BLACK);
        color_found = 1;
    }

    if (string_equals(color, "brown")) {
        vga_set_color(VGA_BROWN, VGA_BLACK);
        color_found = 1;
    }

    if (string_equals(color, "reset")) {
        vga_set_color(VGA_WHITE, VGA_BLACK);
        color_found = 1;
    }

    if (string_equals(color, "help")) {
        vga_print("Cores disponiveis: cyan, blue, green, red, brown, yellow, magenta e reset");
        vga_newline(1);
        color_found = 1;
    }

    if (!color_found) {
        vga_print("Cor desconhecida");
        vga_newline(1);
        vga_print("Digite \"color help\" para ver as cores disponiveis.");
        vga_newline(1);
    }
}

static void command_ascii(void) {
    vga_newline(1);
    vga_print(
    "                     *#\n"
    "                 +*+              *\n"
    "               +*         #     +###\n"
    "              #          #       *\n"
    "             #          +#+\n"
    "            #           #@#            +\n"
    "           *#          :#####:          #*\n"
    "           *#      +#############+      +*\n"
    "           *#      :++#######@#*+:      **\n"
    "           +#           #####:          #*\n"
    "            #           *##             *#\n"
    "             **          :#+           *#\n"
    "              +#          #            #*\n"
    "                +##       #           *#\n"
    "                  +*#           **+\n"
    "                        ++++\n"
    );
}

static void command_meminfo(void) {
    uint32_t kernel_size =
        (uint32_t)&__kernel_end -
        (uint32_t)&__kernel_start;
    
    if (kernel_size >= KERNEL_WARNING_SIZE) {
        vga_print("Aviso: kernel proximo do limite do bootloader");
        vga_newline(1);
    }

    vga_print("Tamanho do kernel: ");
    vga_print_uint(kernel_size);
    vga_print(" bytes");
    vga_newline(1);

    vga_print("Limite do bootloader: ");
    vga_print_uint(KERNEL_MAX_SIZE);
    vga_print(" bytes");
    vga_newline(1);
}

void shell_execute(const char *command) {
    int command_found = 0;

    if (command[0] == '\0') return;

    if (string_equals(command, "help")) {
        command_found = 1;
        vga_print("Comandos: help, about, clear, echo, reboot, sysinfo, ascii, color, meminfo, version, atatest, pmmtest, e820test");
        vga_newline(1);
    }

    if (string_equals(command, "about")) {
        command_found = 1;
        vga_print("Hoshi OS - kernel experimental");
        vga_newline(1);
    }

    if (string_equals(command, "clear")) {
        command_found = 1;
        vga_clear();
    }

    if (string_starts_with(command, "echo ")) {
        command_found = 1;
        vga_print(command + 5);
        vga_newline(1);
    }

    if (string_equals(command, "reboot")) {
        command_found = 1;
        command_reboot();
    }

    if (string_equals(command, "sysinfo")) {
        command_found = 1;
        vga_print("Hoshi OS\nArquitetura: x86 32-bit\nVideo: VGA text mode\nTeclado: PS/2 por polling");
        vga_newline(1);
    }

    if (string_equals(command, "ascii")) {
        command_found = 1;
        command_ascii();
    }

    if (string_equals(command, "version")) {
        command_found = 1;
        vga_print("HoshiOS version 0.1\n");
    }

    if (string_starts_with(command, "color ")) {
        command_found = 1;
        command_color(command + 6);
    }

    if (string_equals(command, "meminfo")) {
        command_found = 1;
        command_meminfo();
    }

    if (string_equals(command, "atatest")) {
       command_found = 1;
       command_atatest();
    }

    if (string_equals(command, "pmmtest")) {
       command_found = 1;
       command_pmmtest();
    }

    if (string_equals(command, "e820test")) {
       command_found = 1;
       command_e820test();
    }
    
    if (!command_found) {
        vga_print("Comando desconhecido, Digite: \"help\" para ver os comandos disponiveis");
        vga_newline(1);
    }
}
