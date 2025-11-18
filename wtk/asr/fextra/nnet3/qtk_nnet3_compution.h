#ifndef QTK_NNET3_COMPUTION_H_
#define QTK_NNET3_COMPUTION_H_
#include "wtk/core/math/wtk_matrix.h"
#include "wtk/core/cfg/wtk_source.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_nnet3_command qtk_nnet3_command_t;
typedef struct qtk_nnet3_matrix_info qtk_nnet3_matrix_info_t;
typedef struct qtk_nnet3_precomputed_indexes qtk_nnet3_precomputed_indexes_t;
typedef struct qtk_nnet3_compution qtk_nnet3_compution_t;
typedef struct qtk_nnet3_backprop_precomputed_indexes qtk_nnet3_backprop_precomputed_indexes_t;
typedef struct qtk_nnet3_convolution_step qtk_nnet3_convolution_step_t;
typedef struct qtk_nnet3_convolution_precomputed_indexes qtk_nnet3_convolution_precomputed_indexes_t;
typedef struct  qtk_nnet3_generaldropout_precomputed_indexes qtk_nnet3_generaldropout_precomputed_indexes_t;
typedef struct qtk_nnet3_fsmn_precomputed_indexes qtk_nnet3_fsmn_precomputed_indexes_t;
typedef enum
{
  kAllocMatrix, kDeallocMatrix, kSwapMatrix, kSetConst,
  kPropagate, kBackprop, kBackpropNoModelUpdate,
  kMatrixCopy, kMatrixAdd, kCopyRows, kAddRows,
  kCopyRowsMulti, kCopyToRowsMulti, kAddRowsMulti, kAddToRowsMulti,
  kAddRowRanges, kCompressMatrix, kDecompressMatrix,
  kAcceptInput, kProvideOutput,
  kNoOperation, kNoOperationPermanent, kNoOperationMarker, kNoOperationLabel,
  kGotoLabel,
}qtk_nnet3_command_type_t;

typedef enum
{
      kDefaultStride,
      kStrideEqualNumCols,
}qtk_nnet3_matrix_type_t;

typedef enum
{
	QTK_BackpropTruncationComponentPrecomputedIndexes,
	QTK_TimeHeightConvolutionComponentPrecomputedIndexes,
	QTK_GeneralDropoutComponentPrecomputedIndexes,
	QTK_FsmnComponentPrecomputedIndexes
}qtk_nnet3_precompute_index_type;

struct qtk_nnet3_command
{
    qtk_nnet3_command_type_t type;
    float alpha;
    int arg1;
    int arg2;
    int arg3;
    int arg4;
    int arg5;
    int arg6;
    int arg7;
    wtk_queue_node_t qn;
};

struct qtk_nnet3_matrix_info
{
    int num_rows;
    int num_cols;
    qtk_nnet3_matrix_type_t stride_type;
    wtk_queue_node_t qn;
};

struct qtk_nnet3_backprop_precomputed_indexes
{
	qtk_blas_matrix_t* zeroing;
	float zeroing_sum;
};

struct qtk_nnet3_convolution_step
{
//TODO
//	std::vector<int32> height_map;
  	int *columns;
	int **backward_columns;
	int *height_map;
	int input_time_shift;
	int params_start_col;
	int columns_are_contiguous;
	int first_column;
	qtk_sub_matrix_t *input_part;
	qtk_sub_matrix_t *params_part;
	qtk_sub_matrix_t *output_reshaped;
	qtk_sub_matrix_t *temp_mat_part;
	qtk_sub_matrix_t *temp_mat_part_reshaped;
	qtk_sub_matrix_t *input_reshaped;
	qtk_sub_matrix_t *newtemp;
	int max_overlap;
};

struct qtk_nnet3_convolution_precomputed_indexes
{
	qtk_nnet3_convolution_step_t **steps;
	int num_filters_in;
	int num_filters_out;
	int height_in;
	int height_out;
	int num_t_in;
	int num_t_out;
	int num_images;
	int temp_rows;
	int temp_cols;
	int num_steps;
	qtk_sub_matrix_t *temp;
	qtk_sub_matrix_t *input_part;
	qtk_sub_matrix_t *output_part;
	qtk_sub_matrix_t *temp_part;
	qtk_sub_matrix_t *input_reshaped;
};

struct qtk_nnet3_generaldropout_precomputed_indexes
{
	int num_mask_rows;
	int *indexes;//0 for length 1,2,3... values
};

struct qtk_nnet3_fsmn_precomputed_indexes
{
	int row_stride;
	int *row_offsets;//0 for length 1,2,3... values
};

struct qtk_nnet3_precomputed_indexes
{
	void *index;
	qtk_nnet3_precompute_index_type type;
};

struct qtk_nnet3_compution
{
    qtk_nnet3_command_t** command_q;
    qtk_nnet3_matrix_info_t** matrix_info_q;
    qtk_nnet3_submatrix_info_t** submatrix_info_q;
	qtk_nnet3_precomputed_indexes_t** pre_computed;
	int** indexes;
	int** indexes_multi;
	int** indexes_ranges;
	int* input_indexes;
	int num[7]; 
	int cmd_cnt;
};

qtk_nnet3_compution_t* qtk_nnet3_compution_new(void);
int qtk_nnet3_compution_load(qtk_nnet3_compution_t *compution,wtk_source_t* src);
void qtk_nnet3_compution_delete(qtk_nnet3_compution_t* nnet3);

#ifdef __cplusplus
};
#endif
#endif
