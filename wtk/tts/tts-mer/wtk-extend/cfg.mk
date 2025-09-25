use_go2blas = 1

ifeq (${use_dev}, 0)
else
	use_debug = 1
	CFLAGS += -DUSE_DEV
endif

ifeq (${use_mat_fix}, 1)
	CFLAGS += -DUSE_MAT_FIX
else ifeq (${use_blas}, 0)
else
	CFLAGS += -DUSE_BLAS
	ifeq (${use_mkl}, 1)
		blas = mkl
		lib_blas ?= ${libmkl}
		CFLAGS += -DUSE_MKL
		ifeq (${use_mkl_pack}, 1)
			CFLAGS += -DUSE_MKL_PACK
		endif
	else ifeq (${use_go2blas}, 1)
		blas = gotoblas
		lib_blas ?= ${libgoto}
		CFLAGS += -DUSE_GO2BLAS
	endif
endif

ifeq (${use_neon}, 1)
	CFLAGS += -DUSE_NEON
endif

ifeq (${use_debug}, 1)
	CFLAGS += -DUSE_DEBUG
endif

libmkl = -m64 -Wl,--start-group \
/opt/intel/mkl/lib/intel64/libmkl_core.a \
/opt/intel/mkl/lib/intel64/libmkl_intel_lp64.a \
/opt/intel/mkl/lib/intel64/libmkl_intel_thread.a  -Wl,--end-group \
-L/opt/intel/lib/intel64 -liomp5 -ldl

libgoto=./third/GotoBLAS2/libgoto2.a