#ifndef VM_H
#define VM_H
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    VM_ERR_NONE,
    VM_ERR_EXIT,
    VM_ERR_END_OF_MEM,
    VM_ERR_ILLEGAL_INST,
} VM_ERR;

typedef enum {
    VM_INST_NOP,
    VM_INST_LOAD,
    VM_INST_LOAD_8,
    VM_INST_LOAD_16,
    VM_INST_LOAD_32,
    VM_INST_LOAD_FROM_STACK,
    VM_INST_LOAD_TO_STACK,
    VM_INST_PUSH,
    VM_INST_POP,
    VM_INST_ADD,
    VM_INST_SUB,
    VM_INST_MUL,
    VM_INST_DIV,
    VM_INST_AND,
    VM_INST_OR,
    VM_INST_XOR,
    VM_INST_NOT,
    VM_INST_SHIFTL,
    VM_INST_SHIFTR,
    VM_INST_CMP, // compare
    VM_INST_LT, // less than
    VM_INST_LTE, // less than or equal
    VM_INST_JP,
    VM_INST_JPC, // conditional jump
    VM_INST_CALL,
    VM_INST_CALLC, // conditional call
    VM_INST_RET,
    VM_INST_EX_CALL, // external call
    VM_INST_INV_FTRUE,
    VM_INST_EXIT,
} VM_INST;

typedef struct {
    uint32_t *stack;
    uint32_t size;
    uint32_t allocated;
} vm_stack_t;

#define REGISTER_COUNT 4

#if REGISTER_COUNT > 16
#  error "Can not use more than 16 registers"
#endif

typedef struct {
    uint32_t regs[REGISTER_COUNT];

    bool flag_true;

    uint32_t pc;
    uint8_t *mem;
    uint32_t mem_size;

    vm_stack_t call_stack;
    vm_stack_t variable_stack;

    uint8_t exit_code;
} vm_state;

void vm_init(vm_state *s, uint8_t *mem, uint32_t mem_size);
VM_ERR vm_step(vm_state *s);

#endif
