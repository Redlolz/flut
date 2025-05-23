#include "vm.h"
#include <stddef.h>
#include <stdio.h>

int main()
{
    uint8_t mem[2048] = {
        VM_INST_LOAD_8, 0, 100,
        VM_INST_PUSH, 0x00,
        VM_INST_LOAD_8, 1, 0,
        VM_INST_LOAD_8, 2, 1,
        VM_INST_LOAD_8, 3, 2,
        VM_INST_ADD, 0x13,
        VM_INST_CMP, 0x01,
        VM_INST_INV_FTRUE,
        VM_INST_JPC, 14, 0, 0, 0,
        VM_INST_POP, 0x00,
        VM_INST_ADD, 0x10,
        VM_INST_SUB, 0x13,
        VM_INST_DIV, 0x13,
        VM_INST_ADD, 0x32,
        VM_INST_MUL, 0x13,
        VM_INST_EXIT, 0x1,
    };

    size_t mem_size = sizeof(mem);
    vm_state state;

    vm_init(&state, mem, mem_size);

    VM_ERR err = VM_ERR_NONE;
    while (err == VM_ERR_NONE) {
        printf("%u: %u\n", state.pc, mem[state.pc]);
        printf("\t0: %u 1: %u 2: %u 3: %u\n", state.regs[0], state.regs[1], state.regs[2], state.regs[3]);
        err = vm_step(&state);
    }

    switch (err) {
        case VM_ERR_NONE:
            printf("VM_ERR_NONE\n");
            break;
        case VM_ERR_EXIT:
            printf("VM_ERR_EXIT\n");
            printf("%i\n", state.exit_code);
            break;
        case VM_ERR_END_OF_MEM:
            printf("VM_ERR_END_OF_MEM\n");
            break;
        case VM_ERR_ILLEGAL_INST:
            printf("VM_ERR_ILLEGAL_INST\n");
            break;
    }
}
