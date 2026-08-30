ENTRY(entry)

PHDRS
{
    text PT_LOAD FLAGS(5);
    data PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = 0x1000;
    __kernel_start = .;

    /*
     * FLAGS(5) = leitura + execução.
     * Código e constantes não podem ser modificados.
     */
    .text : ALIGN(16)
    {
        *(.text*)
    } :text

    .rodata : ALIGN(16)
    {
        *(.rodata*)
    } :text

    /*
     * Começa o segmento gravável em outra página.
     * Isso permitirá aplicar permissões reais quando
     * a paginação for implementada.
     */
    . = ALIGN(4096);

    /*
     * FLAGS(6) = leitura + escrita, sem execução.
     */
    .data : ALIGN(16)
    {
        *(.data*)
    } :data

    .bss (NOLOAD) : ALIGN(16)
    {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } :data

    __kernel_end = .;

    /*
     * A IDT utiliza a região física 0xF000.
     * O primeiro 1 MiB permanece reservado pelo PMM.
     */
    __idt_start__ = ABSOLUTE(0xF000);
}