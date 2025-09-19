#ifndef QBL_NN_QBL_NN_DEBUG_H
#define QBL_NN_QBL_NN_DEBUG_H
#pragma once
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif
void qtk_nn_vm_print_symbol(qtk_nn_vm_t *nv, uint16_t repr);
void qtk_nn_vm_save_symbol(qtk_nn_vm_t *nv, uint16_t x);
const char *qtk_nn_vm_get_opname(qtk_nn_vm_op_t op);
wtk_string_t qtk_nn_vm_get_symbol(qtk_nn_vm_t *nv, uint16_t repr);
int qtk_nn_vm_load_debug_info(qtk_nn_vm_t *nv, qtk_io_reader reader,
                              void *upval);
void qtk_nn_vm_dump_debug_info(qtk_nn_vm_t *nv, uint8_t *cur_instruction);
void qtk_nn_vm_dump_input_debug_info(qtk_nn_vm_t *nv);
#ifdef __cplusplus
};
#endif
#endif
