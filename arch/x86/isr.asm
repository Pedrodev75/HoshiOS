extern division_error_handler
extern invalid_opcode_handler
extern general_protection_fault_handler
extern page_fault_handler
extern double_fault_handler
global ir0
global ir6
global ir13
global ir14
extern ir8

ir0:
    pushad

    call division_error_handler

    popad
    iret

ir6:
    pushad

    call invalid_opcode_handler

    popad
    iret

ir13:
    pop eax
    pushad

    call general_protection_fault_handler

    popad
    iret

ir14:
    pop eax
    pushad

    call page_fault_handler

    popad
    iret

ir8:
   pop eax
   pushad

   call double_fault_handler

   popad
   iret