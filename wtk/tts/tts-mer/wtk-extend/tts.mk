_path_tts := ./wtk
lex_dir = $(shell find ${_path_tts}/lex -maxdepth 4 -type d)
lex_file = $(foreach lex_dirs,$(lex_dir),$(wildcard $(lex_dirs)/*.c))
obj_lex = $(patsubst %.c,%.o, ${lex_file})
json_dir = $(shell find ${_path_tts}/core/json -maxdepth 4 -type d)
json_file = $(foreach json_dirs,$(json_dir),$(wildcard $(json_dirs)/*.c))
obj_json = $(patsubst %.c,%.o, ${json_file}) 
segmenter_dir = $(shell find ${_path_tts}/core/segmenter -maxdepth 4 -type d)
segmenter_file = $(foreach segmenter_dirs,$(segmenter_dir),$(wildcard $(segmenter_dirs)/*.c))
obj_segmenter = $(patsubst %.c,%.o, ${segmenter_file})

obj_tts = \
${_path_tts}/core/wavehdr.o \
${_path_tts}/core/math/wtk_matrix.o \
${_path_tts}/core/math/wtk_math.o \
${_path_tts}/core/math/wtk_mat.o \
${_path_tts}/core/math/wtk_vector.o \
${_path_tts}/core/fft/wtk_rfft.o \
${_path_tts}/core/wtk_kcls.o \
${_path_tts}/core/wtk_bit_heap.o \
${_path_tts}/core/wtk_heap.o \
${_path_tts}/core/wtk_hoard.o \
${_path_tts}/core/wtk_queue.o \
${_path_tts}/core/wtk_queue2.o \
${_path_tts}/core/wtk_queue3.o \
${_path_tts}/core/wtk_array.o \
${_path_tts}/core/wtk_str.o \
${_path_tts}/core/wtk_str_encode.o \
${_path_tts}/core/wtk_stridx.o \
${_path_tts}/core/wtk_str_hash.o \
${_path_tts}/core/wtk_strlist.o \
${_path_tts}/core/wtk_strbuf.o \
${_path_tts}/core/wtk_strpool.o \
${_path_tts}/core/wtk_larray.o \
${_path_tts}/core/wtk_os.o \
${_path_tts}/core/wtk_kdict.o \
${_path_tts}/core/wtk_treenode.o \
${_path_tts}/core/wtk_rbtree.o \
${_path_tts}/core/wtk_arg.o \
${_path_tts}/core/cfg/wtk_source.o\
\
${_path_tts}/core/wtk_fkv.o\
${_path_tts}/core/wtk_fkv2.o\
${_path_tts}/core/wtk_robin.o\
${_path_tts}/core/wtk_vpool.o\
${_path_tts}/core/wtk_sort.o \
${_path_tts}/core/wtk_if.o \
${_path_tts}/core/parse/wtk_number_parse.o\
${_path_tts}/core/parse/wtk_str_parse.o\
${_path_tts}/core/rbin/wtk_rbin2.o\
${_path_tts}/core/rbin/wtk_rbin.o\
${_path_tts}/core/rbin/wtk_flist.o\
${_path_tts}/core/wtk_hash.o\
${_path_tts}/core/cfg/wtk_cfg_queue.o\
${_path_tts}/core/cfg/wtk_cfg_file.o\
${_path_tts}/core/cfg/wtk_local_cfg.o\
${_path_tts}/core/cfg/wtk_main_cfg.o\
${_path_tts}/core/cfg/wtk_mbin_cfg.o\
${_path_tts}/core/math/wtk_sparsem.o \
${_path_tts}/core/wtk_str_parser.o \
${_path_tts}/tts/parser/wtk_tts_parser.o\
${_path_tts}/tts/parser/wtk_tts_parser_cfg.o\
${_path_tts}/tts/parser/wtk_tts_norm.o\
${_path_tts}/tts/parser/wtk_tts_def.o\
${_path_tts}/tts/parser/wtk_defpron.o\
${_path_tts}/tts/parser/wtk_tts_phn.o\
${_path_tts}/tts/parser/wtk_tts_phn_cfg.o\
${_path_tts}/tts/parser/wtk_tts_norm_cfg.o\
${_path_tts}/tts/parser/wtk_tts_segsnt_cfg.o\
${_path_tts}/tts/parser/wtk_tts_segsnt.o\
${_path_tts}/tts/parser/wtk_tts_segwrd_cfg.o\
${_path_tts}/tts/parser/wtk_tts_segwrd.o\
${_path_tts}/tts/parser/wtk_maxseg.o\
${_path_tts}/tts/parser/wtk_polyphn_lex.o\
${_path_tts}/tts/parser/pos/wtk_tts_pos_cfg.o\
${_path_tts}/tts/parser/pos/wtk_tts_pos.o\
${_path_tts}/tts/parser/pos/wtk_tts_poshmm.o \
${_path_tts}/tts/parser/wtk_polyphn_cfg.o \
${_path_tts}/tts/parser/wtk_polyphn.o \
${obj_segmenter} \
${obj_lex} \
${obj_json}