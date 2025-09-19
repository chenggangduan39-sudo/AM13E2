import os
from string import Template
import uuid

mod_h_tpl = Template('''#ifndef ${guard}
#define ${guard}

#include "${cfg_hdr_fn}"

typedef struct qtk_${mod_name} qtk_${mod_name}_t;

struct qtk_${mod_name} {
    qtk_${mod_name}_cfg_t *cfg;
};

qtk_${mod_name}_t *qtk_${mod_name}_new(qtk_${mod_name}_cfg_t *cfg);
void qtk_${mod_name}_delete(qtk_${mod_name}_t *m);

#endif
''')

mod_tpl = Template('''#include "${hdr_fn}"

qtk_${mod_name}_t *qtk_${mod_name}_new(qtk_${mod_name}_cfg_t *cfg) {

}

void qtk_${mod_name}_delete(qtk_${mod_name}_t *m) {

}
''')


mod_cfg_h_tpl = Template('''#ifndef ${guard}
#define ${guard}
#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_${mod_name}_cfg qtk_${mod_name}_cfg_t;

struct qtk_${mod_name}_cfg {
};

int qtk_${mod_name}_cfg_init(qtk_${mod_name}_cfg_t *cfg);
int qtk_${mod_name}_cfg_clean(qtk_${mod_name}_cfg_t *cfg);
int qtk_${mod_name}_cfg_update(qtk_${mod_name}_cfg_t *cfg);
int qtk_${mod_name}_cfg_update_local(qtk_${mod_name}_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_${mod_name}_cfg_update2(qtk_${mod_name}_cfg_t *cfg, wtk_source_loader_t *sl);

#endif
''')

mod_cfg_tpl = Template('''#include "${cfg_hdr_fn}"

int qtk_${mod_name}_cfg_init(qtk_${mod_name}_cfg_t *cfg) {

}

int qtk_${mod_name}_cfg_clean(qtk_${mod_name}_cfg_t *cfg) {

}

int qtk_${mod_name}_cfg_update(qtk_${mod_name}_cfg_t *cfg) {

}

int qtk_${mod_name}_cfg_update_local(qtk_${mod_name}_cfg_t *cfg, wtk_local_cfg_t *lc) {

}
int qtk_${mod_name}_cfg_update2(qtk_${mod_name}_cfg_t *cfg, wtk_source_loader_t *sl) {

}
''')

def _gen_mod(fn, mod_name):
    hdr_fn = fn[:-2] + '.h'
    with open(fn, 'x') as f:
        f.write(mod_tpl.substitute({
            'hdr_fn': hdr_fn,
            'mod_name': mod_name
            }))

def _gen_mod_cfg(fn, mod_name):
    cfg_hdr_fn = fn[:-2] + '.h'
    with open(fn, 'x') as f:
        f.write(mod_cfg_tpl.substitute({
            'cfg_hdr_fn': cfg_hdr_fn,
            'mod_name': mod_name
            }))

def _gen_mod_hdr(fn, mod_name):
    guard = f'G_{uuid.uuid4().hex.upper()}'
    cfg_hdr_fn = fn[:-2] + '_cfg.h'
    with open(fn, 'x') as f:
        f.write(mod_h_tpl.substitute({
            'mod_name': mod_name,
            'cfg_hdr_fn': cfg_hdr_fn,
            'guard': guard
            }))
    pass

def _gen_mod_cfg_hdr(fn, mod_name):
    guard = f'G_{uuid.uuid4().hex.upper()}'
    with open(fn, 'x') as f:
        f.write(mod_cfg_h_tpl.substitute({
            'mod_name': mod_name,
            'guard': guard
            }))

def _main(args):
    sl = args.mod.split('/')
    mod_name = sl[-1]
    dir_list = sl[:-1]
    dir_name = '.'
    if dir_list:
        dir_name = os.path.join(*dir_list)
        if not os.path.exists(dir_name):
            os.system(f'mkdir -p {dir_name}')
    _gen_mod(os.path.join(dir_name, f'qtk_{mod_name}.c'), mod_name)
    _gen_mod_hdr(os.path.join(dir_name, f'qtk_{mod_name}.h'), mod_name)
    _gen_mod_cfg(os.path.join(dir_name, f'qtk_{mod_name}_cfg.c'), mod_name)
    _gen_mod_cfg_hdr(os.path.join(dir_name, f'qtk_{mod_name}_cfg.h'), mod_name)

if __name__ == '__main__':
    import argparse
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument('mod')
    args = arg_parser.parse_args()
    _main(args)
