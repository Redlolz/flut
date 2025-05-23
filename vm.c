#include "vm.h"
#include <stdint.h>
#include <stdlib.h>

static void stack_init(vm_stack_t *s)
{
    s->size = 0;
    s->allocated = sizeof(uint32_t) * 128;
    s->stack = malloc(sizeof(uint32_t) * s->allocated);
}

static void stack_push(vm_stack_t *s, uint32_t v)
{
    if (s->size * sizeof(uint32_t) >= s->allocated) {
        s->allocated += sizeof(uint32_t) * 128;
        s->stack = realloc(s->stack, s->allocated);
    }
    s->stack[s->size] = v;
    s->size++;
}

static uint32_t stack_pop(vm_stack_t *s)
{
    s->size--;
    return s->stack[s->size];
}

static void stack_set(vm_stack_t *s, uint32_t offset, uint32_t v)
{
    if (s->size-(1+offset) < 0) {
        return;
    }
    s->stack[s->size-(1+offset)] = v;
}

static uint32_t stack_get(vm_stack_t *s, uint32_t offset)
{
    if (s->size-(1+offset) < 0) {
        return 0;
    }
    return s->stack[s->size-(1+offset)];
}

void vm_init(vm_state *s, uint8_t *mem, uint32_t mem_size)
{
    for (unsigned int i = 0; i < REGISTER_COUNT; i++) {
        s->regs[i] = 0;
    }

    s->flag_true = false;

    s->pc = 0;
    s->mem = mem;
    s->mem_size = mem_size;

    stack_init(&s->call_stack);
    stack_init(&s->variable_stack);
}

VM_ERR vm_step(vm_state *s)
{
    if (s->pc >= s->mem_size) {
        return VM_ERR_END_OF_MEM;
    }

    switch (s->mem[s->pc]) {
        case VM_INST_NOP:
            s->pc++;
            break;
        case VM_INST_LOAD:
            if (s->pc + 1 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (((s->mem[s->pc+1] >> 4) & 0xf) >= REGISTER_COUNT || (s->mem[s->pc+1] & 0xf) >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }

            s->regs[((s->mem[s->pc+1] >> 4) & 0xf)] = s->regs[(s->mem[s->pc+1] & 0xf)];
            s->pc += 2;
            break;
        case VM_INST_LOAD_8:
            if (s->pc + 2 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (s->mem[s->pc+1] >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }

            s->regs[s->mem[s->pc+1]] = s->mem[s->pc+2];
            s->pc += 3;
            break;
        case VM_INST_LOAD_16:
            if (s->pc + 3 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (s->mem[s->pc+1] >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }

            s->regs[s->mem[s->pc+1]] = (uint32_t)s->mem[s->pc+2] | ((uint32_t)s->mem[s->pc+3] << 8);
            s->pc += 4;
            break;
        case VM_INST_LOAD_32:
            if (s->pc + 5 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (s->mem[s->pc+1] >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }

            s->regs[s->mem[s->pc+1]] = (uint32_t)s->mem[s->pc+2]
                | ((uint32_t)s->mem[s->pc+3] << 8)
                | ((uint32_t)s->mem[s->pc+4] << 16)
                | ((uint32_t)s->mem[s->pc+5] << 24);
            s->pc += 6;
            break;
        case VM_INST_LOAD_FROM_STACK:
            if (s->pc + 2 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (s->mem[s->pc+1] >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }
            s->regs[s->mem[s->pc+1]] = stack_get(&s->variable_stack, s->mem[s->pc+2]);
            s->pc += 3;
            break;
        case VM_INST_LOAD_TO_STACK:
            if (s->pc + 2 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (s->mem[s->pc+1] >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }
            stack_set(&s->variable_stack, s->mem[s->pc+2], s->regs[s->mem[s->pc+1]]);
            s->pc += 3;
            break;
        case VM_INST_PUSH:
            if (s->pc + 1 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (s->mem[s->pc+1] >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }
            stack_push(&s->variable_stack, s->regs[s->mem[s->pc+1]]);
            s->pc += 2;
            break;
        case VM_INST_POP:
            if (s->pc + 1 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (s->mem[s->pc+1] >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }
            s->regs[s->mem[s->pc+1]] = stack_pop(&s->variable_stack);
            s->pc += 2;
            break;
        case VM_INST_ADD:
            if (s->pc + 1 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (((s->mem[s->pc+1] >> 4) & 0xf) >= REGISTER_COUNT || (s->mem[s->pc+1] & 0xf) >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }

            s->regs[((s->mem[s->pc+1] >> 4) & 0xf)] += s->regs[(s->mem[s->pc+1] & 0xf)];
            s->pc += 2;
            break;
        case VM_INST_SUB:
            if (s->pc + 1 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (((s->mem[s->pc+1] >> 4) & 0xf) >= REGISTER_COUNT || (s->mem[s->pc+1] & 0xf) >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }

            s->regs[((s->mem[s->pc+1] >> 4) & 0xf)] -= s->regs[(s->mem[s->pc+1] & 0xf)];
            s->pc += 2;
            break;
        case VM_INST_MUL:
            if (s->pc + 1 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (((s->mem[s->pc+1] >> 4) & 0xf) >= REGISTER_COUNT || (s->mem[s->pc+1] & 0xf) >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }

            s->regs[((s->mem[s->pc+1] >> 4) & 0xf)] *= s->regs[(s->mem[s->pc+1] & 0xf)];
            s->pc += 2;
            break;
        case VM_INST_DIV:
            if (s->pc + 1 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (((s->mem[s->pc+1] >> 4) & 0xf) >= REGISTER_COUNT || (s->mem[s->pc+1] & 0xf) >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }

            s->regs[((s->mem[s->pc+1] >> 4) & 0xf)] /= s->regs[(s->mem[s->pc+1] & 0xf)];
            s->pc += 2;
            break;
        case VM_INST_AND:
            if (s->pc + 1 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (((s->mem[s->pc+1] >> 4) & 0xf) >= REGISTER_COUNT || (s->mem[s->pc+1] & 0xf) >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }

            s->regs[((s->mem[s->pc+1] >> 4) & 0xf)] &= s->regs[(s->mem[s->pc+1] & 0xf)];
            s->pc += 2;
            break;
        case VM_INST_OR:
            if (s->pc + 1 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (((s->mem[s->pc+1] >> 4) & 0xf) >= REGISTER_COUNT || (s->mem[s->pc+1] & 0xf) >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }

            s->regs[((s->mem[s->pc+1] >> 4) & 0xf)] |= s->regs[(s->mem[s->pc+1] & 0xf)];
            s->pc += 2;
            break;
        case VM_INST_XOR:
            if (s->pc + 1 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (((s->mem[s->pc+1] >> 4) & 0xf) >= REGISTER_COUNT || (s->mem[s->pc+1] & 0xf) >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }

            s->regs[((s->mem[s->pc+1] >> 4) & 0xf)] ^= s->regs[(s->mem[s->pc+1] & 0xf)];
            s->pc += 2;
            break;
        case VM_INST_NOT:
            if (s->pc + 1 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if ((s->mem[s->pc+1] & 0xf) >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }

            s->regs[(s->mem[s->pc+1] & 0xf)] = ~s->regs[(s->mem[s->pc+1] & 0xf)];
            s->pc += 2;
            break;
        case VM_INST_SHIFTL:
            if (s->pc + 1 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (((s->mem[s->pc+1] >> 4) & 0xf) >= REGISTER_COUNT || (s->mem[s->pc+1] & 0xf) >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }

            s->regs[((s->mem[s->pc+1] >> 4) & 0xf)] <<= s->regs[(s->mem[s->pc+1] & 0xf)];
            s->pc += 2;
            break;
        case VM_INST_SHIFTR:
            if (s->pc + 1 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (((s->mem[s->pc+1] >> 4) & 0xf) >= REGISTER_COUNT || (s->mem[s->pc+1] & 0xf) >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }

            s->regs[((s->mem[s->pc+1] >> 4) & 0xf)] >>= s->regs[(s->mem[s->pc+1] & 0xf)];
            s->pc += 2;
            break;
        case VM_INST_CMP: // compare
            if (s->pc + 1 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (((s->mem[s->pc+1] >> 4) & 0xf) >= REGISTER_COUNT || (s->mem[s->pc+1] & 0xf) >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }

            if (s->regs[((s->mem[s->pc+1] >> 4) & 0xf)] == s->regs[(s->mem[s->pc+1] & 0xf)]) {
                s->flag_true = true;
            } else {
                s->flag_true = false;
            }
            s->pc += 2;
            break;
        case VM_INST_LT: // less than
            if (s->pc + 1 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (((s->mem[s->pc+1] >> 4) & 0xf) >= REGISTER_COUNT || (s->mem[s->pc+1] & 0xf) >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }

            if (s->regs[((s->mem[s->pc+1] >> 4) & 0xf)] < s->regs[(s->mem[s->pc+1] & 0xf)]) {
                s->flag_true = true;
            } else {
                s->flag_true = false;
            }
            s->pc += 2;
            break;
        case VM_INST_LTE: // less than or equal
            if (s->pc + 1 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (((s->mem[s->pc+1] >> 4) & 0xf) >= REGISTER_COUNT || (s->mem[s->pc+1] & 0xf) >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }

            if (s->regs[((s->mem[s->pc+1] >> 4) & 0xf)] <= s->regs[(s->mem[s->pc+1] & 0xf)]) {
                s->flag_true = true;
            } else {
                s->flag_true = false;
            }
            s->pc += 2;
            break;
        case VM_INST_JP:
            if (s->pc + 4 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            s->pc = (uint32_t)s->mem[s->pc+1]
                | ((uint32_t)s->mem[s->pc+2] << 8)
                | ((uint32_t)s->mem[s->pc+3] << 16)
                | ((uint32_t)s->mem[s->pc+4] << 24);
            break;
        case VM_INST_JPC: // conditional jump
            if (s->pc + 4 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (s->flag_true) {
                s->pc = (uint32_t)s->mem[s->pc+1]
                    | ((uint32_t)s->mem[s->pc+2] << 8)
                    | ((uint32_t)s->mem[s->pc+3] << 16)
                    | ((uint32_t)s->mem[s->pc+4] << 24);
            } else {
                s->pc += 5;
            }
            break;
        case VM_INST_CALL:
            if (s->pc + 4 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            stack_push(&s->call_stack, s->pc);
            s->pc = (uint32_t)s->mem[s->pc+1]
                | ((uint32_t)s->mem[s->pc+2] << 8)
                | ((uint32_t)s->mem[s->pc+3] << 16)
                | ((uint32_t)s->mem[s->pc+4] << 24);
            break;
        case VM_INST_CALLC: // conditional call
            if (s->pc + 4 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (s->flag_true) {
                stack_push(&s->call_stack, s->pc);
                s->pc = (uint32_t)s->mem[s->pc+1]
                    | ((uint32_t)s->mem[s->pc+2] << 8)
                    | ((uint32_t)s->mem[s->pc+3] << 16)
                    | ((uint32_t)s->mem[s->pc+4] << 24);
            } else {
                s->pc += 5;
            }
            break;
        case VM_INST_RET:
            s->pc = stack_pop(&s->call_stack);
            break;
        case VM_INST_EX_CALL:
            break;
        case VM_INST_INV_FTRUE:
            s->flag_true = !s->flag_true;
            s->pc += 1;
            break;
        case VM_INST_EXIT:
            if (s->pc + 1 >= s->mem_size) { return VM_ERR_ILLEGAL_INST; }
            if (s->mem[s->pc+1] >= REGISTER_COUNT) {
                return VM_ERR_ILLEGAL_INST;
            }
            s->exit_code = s->regs[s->mem[s->pc+1]];
            return VM_ERR_EXIT;
        default:
            return VM_ERR_ILLEGAL_INST;
    }

    return VM_ERR_NONE;
}
