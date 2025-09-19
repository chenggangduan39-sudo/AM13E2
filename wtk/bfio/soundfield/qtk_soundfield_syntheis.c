#include "qtk_soundfield_syntheis.h"
#include "qtk_turkey_window.h"
#include "wtk/core/math/wtk_math.h"

#define DIM 3         // 三维向量
#define SOUNDSPEED 343.0  // 声速常量
#define SELECTION_TOL 1e-6 // 选择容差

static float degrees_to_radians(float degrees) {
    return degrees * (M_PI / 180.0);  // 核心公式
}

static void direction_vector(float alpha, float beta, float* result) {
    result[0] = cos(alpha) * sin(beta);  // x
    result[1] = sin(alpha) * sin(beta);  // y
    result[2] = cos(beta);               // z  [1,3](@ref)
}

double dot(const Vector3D a, const Vector3D b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

// 向量归一化 (原地修改)
static void normalize_vector(Vector3D v) {
    float norm = sqrt(dot(v, v));
    if (norm > 0) {
        v.x /= norm;
        v.y /= norm;
        v.z /= norm;
    }
}

// 主函数：平面波驱动计算
static void plane2d(
    float omega,        // 角频率
    Vector3D *x0,     // 声源位置数组 [N][3]
    Vector3D *n0,     // 法向量数组 [N][3]
    int N,               // 声源数量
    Vector3D *n,         // 平面波方向 (传入后归一化)
    wtk_complex_t* d,             // 输出：驱动复数数组
    int* selection       // 输出：选择标记数组
) {
    // 归一化平面波方向
    normalize_vector(*n);
    
    // 计算波数 k = ω/c
    float k = omega / SOUNDSPEED;
    wtk_complex_t a,b;
    // 遍历每个声源
    for (int i = 0; i < N; i++) {
        // 计算 n·n0 和 n·x0
        float dot_n_n0 = dot(*n, n0[i]);
        float dot_n_x0 = dot(*n, x0[i]);
        
        // 计算驱动函数：d = 2j·k·(n·n0)·exp(-j·k·(n·x0))
        a.a = 0;
        a.b = 2 * k * dot_n_n0;
        b.a = 0;
        b.b = -1.0 * k * dot_n_x0;
        wtk_complex_exp(&b);
        (d + i)->a = a.a * b.a - a.b * b.b;
        (d + i)->b = a.a * b.b + a.b * b.a;
        // 计算选择条件：n·n0 ≥ 容差
        selection[i] = (dot_n_n0 >= SELECTION_TOL) ? 1 : 0;
    }
}

void normalize(Vector3D* v) {
    double len = sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
    if (len > 1e-8) {
        v->x /= len;
        v->y /= len;
        v->z /= len;
    }
}

// 向量叉积
Vector3D cross(const Vector3D a, const Vector3D b) {
    return (Vector3D){
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    };
}


void rotation_matrix(const Vector3D n1, const Vector3D n2, float R[3][3]) {
    Vector3D v1 = n1, v2 = n2;
    normalize(&v1);
    normalize(&v2);
    
    // 单位矩阵
    float I[3][3] = {{1,0,0},{0,1,0},{0,0,1}};
    
    // 相同或相反方向
    if(fabs(dot(v1, v2)) > 0.999) {
        memcpy(R, I, sizeof(float)*9);
        if(dot(v1, v2) < 0) {
            for(int i=0; i<3; i++) for(int j=0; j<3; j++) 
                R[i][j] *= -1;
        }
        return;
    }
    
    // 计算旋转轴和角度
    Vector3D axis = cross(v1, v2);
    float sin_theta = sqrt(dot(axis, axis));
    float cos_theta = dot(v1, v2);
    normalize(&axis);
    
    // 叉积矩阵
    float K[3][3] = {
        {0, -axis.z, axis.y},
        {axis.z, 0, -axis.x},
        {-axis.y, axis.x, 0}
    };
    
    // 罗德里格斯公式
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            R[i][j] = I[i][j] 
                     + sin_theta * K[i][j] 
                     + (1 - cos_theta) * (K[i][0]*K[0][j] 
                                        + K[i][1]*K[1][j] 
                                        + K[i][2]*K[2][j]);
        }
    }
}

void linear_array(LinearArray* array, int N, float spacing, Vector3D center, Vector3D orientation) {
    // 1. 初始化输出结构
    array->count = N;
    array->positions = malloc(N * sizeof(Vector3D));
    array->normals = malloc(N * sizeof(Vector3D));
    array->weights = malloc(N * sizeof(float));
    
    // 2. 生成y坐标
    float* ycoords = malloc(N * sizeof(float));
    for(int i=0; i<N; i++) {
        ycoords[i] = i * spacing;
    }
    
    // 3. 计算中心偏移
    float y_center = (ycoords[0] + ycoords[N-1]) / 2.0;
    
    // 4. 初始化位置和法向量
    for(int i=0; i<N; i++) {
        array->positions[i] = (Vector3D){0, ycoords[i] - y_center, 0};
        array->normals[i] = (Vector3D){1, 0, 0};  // 初始法向量
    }
    
    // 5. 计算旋转矩阵
    float R[3][3];
    Vector3D base_orientation = {1, 0, 0};
    rotation_matrix(base_orientation, orientation, R);
    
    // 6. 应用旋转
    for(int i=0; i<N; i++) {
        // 旋转位置
        Vector3D p = array->positions[i];
        array->positions[i] = (Vector3D){
            R[0][0]*p.x + R[0][1]*p.y + R[0][2]*p.z,
            R[1][0]*p.x + R[1][1]*p.y + R[1][2]*p.z,
            R[2][0]*p.x + R[2][1]*p.y + R[2][2]*p.z
        };
        
        // 旋转法向量
        Vector3D n = array->normals[i];
        array->normals[i] = (Vector3D){
            R[0][0]*n.x + R[0][1]*n.y + R[0][2]*n.z,
            R[1][0]*n.x + R[1][1]*n.y + R[1][2]*n.z,
            R[2][0]*n.x + R[2][1]*n.y + R[2][2]*n.z
        };
        
        // 添加中心偏移
        array->positions[i].x += center.x;
        array->positions[i].y += center.y;
        array->positions[i].z += center.z;
    }
    
    // 7. 计算权重（中点法则）
    // 扩展位置数组（镜像边界）
    Vector3D* ext_positions = malloc((N+2) * sizeof(Vector3D));
    ext_positions[0] = array->positions[0];  // 镜像处理
    ext_positions[0].x = 2*array->positions[0].x - array->positions[1].x;
    ext_positions[0].y = 2*array->positions[0].y - array->positions[1].y;
    ext_positions[0].z = 2*array->positions[0].z - array->positions[1].z;
    
    memcpy(&ext_positions[1], array->positions, N * sizeof(Vector3D));
    
    ext_positions[N+1] = array->positions[N-1];  // 镜像处理
    ext_positions[N+1].x = 2*array->positions[N-1].x - array->positions[N-2].x;
    ext_positions[N+1].y = 2*array->positions[N-1].y - array->positions[N-2].y;
    ext_positions[N+1].z = 2*array->positions[N-1].z - array->positions[N-2].z;
    
    // 计算距离
    float* distances = malloc((N+1) * sizeof(float));
    for(int i=0; i<=N; i++) {
        Vector3D d = {
            ext_positions[i+1].x - ext_positions[i].x,
            ext_positions[i+1].y - ext_positions[i].y,
            ext_positions[i+1].z - ext_positions[i].z
        };
        distances[i] = sqrt(d.x*d.x + d.y*d.y + d.z*d.z);
    }
    
    // 计算权重
    for(int i=0; i<N; i++) {
        array->weights[i] = (distances[i] + distances[i+1]) / 2.0;
    }
    
    // 清理临时内存
    free(ycoords);
    free(ext_positions);
    free(distances);
}
static void complex_dump(wtk_complex_t *c, int len){
    int i;
    for(i = 0; i < len; i++){
        printf("%.10f %.10f\n", c[i].a, c[i].b);
    }
}
qtk_soundfield_syntheis_t *qtk_soundfield_syntheis_new(qtk_soundfield_syntheis_cfg_t *cfg){
	qtk_soundfield_syntheis_t *sos = wtk_malloc(sizeof(qtk_soundfield_syntheis_t));
    sos->cfg = cfg;

    float npw[3];
    direction_vector(degrees_to_radians(cfg->pw_angle), M_PI/2, npw);
    LinearArray array;
    Vector3D npw3D = (Vector3D){npw[0], npw[1], npw[2]};
    linear_array(&array, cfg->N, cfg->array_spacing, 
        (Vector3D){cfg->center[0], cfg->center[1], cfg->center[2]}, (Vector3D){1.0, 0.0, 0.0});

    int i,j;
    float *turkey_win = wtk_malloc(cfg->N * sizeof(float));
    int *selection = wtk_malloc(cfg->N * sizeof(int));
    float omega;
    float omg = 2.0 * cfg->fs / (2 * cfg->hop_size) * M_PI;
    sos->driving_func = wtk_malloc(cfg->N * (cfg->hop_size + 1) * sizeof(wtk_complex_t));
    for(i = 0; i < cfg->hop_size + 1; i++) {
        omega = omg * i;
        plane2d(omega, array.positions, array.normals, cfg->N, &npw3D, sos->driving_func + i * cfg->N, selection);
        //print_int(selection, cfg->N);
        //complex_dump(sos->driving_func + i * cfg->N, cfg->N);
        tukey(selection, cfg->N, 0.3, turkey_win);
        //print_float(turkey_win, cfg->N);
        for(j = 0; j < cfg->N; j++) {
            sos->driving_func[i * cfg->N + j].a *= turkey_win[j];
            sos->driving_func[i * cfg->N + j].b *= turkey_win[j];
        }
    }
    memset(sos->driving_func, 0, sizeof(wtk_complex_t) * cfg->N);
    sos->prev_half_win = wtk_calloc(cfg->N * cfg->hop_size, sizeof(float));
    sos->drft = wtk_drft_new2(cfg->hop_size * 2);
    sos->fft_in = (float*)wtk_malloc(cfg->hop_size * 2 * sizeof(float));
    sos->fft_buf = (wtk_complex_t*)wtk_malloc((cfg->hop_size + 1) * 2 * sizeof(wtk_complex_t));
    sos->ifft_in = (wtk_complex_t*)wtk_malloc((cfg->hop_size + 1) * cfg->N * sizeof(wtk_complex_t));
    sos->ifft_buf = (float*)wtk_malloc(cfg->hop_size * cfg->N * 2 * sizeof(float));

    sos->input = wtk_strbuf_new(1024,1);
    sos->output_buf = wtk_calloc(cfg->N * cfg->hop_size, sizeof(float));
	sos->output = (short **)wtk_malloc(sizeof(short*) * cfg->N);
    for(i = 0; i < cfg->N; i++){
        sos->output[i] = (short *)wtk_malloc(sizeof(short) * cfg->hop_size);
    }

    wtk_free(selection);
    wtk_free(turkey_win);
    wtk_free(array.positions);
    wtk_free(array.normals);
    wtk_free(array.weights);
	return sos;
}


void qtk_soundfield_syntheis_delete(qtk_soundfield_syntheis_t *sos){
    int i;
    wtk_free(sos->driving_func);
    wtk_free(sos->prev_half_win);
    wtk_free(sos->fft_in);
    wtk_free(sos->fft_buf);
    wtk_free(sos->ifft_in);
    wtk_free(sos->ifft_buf);
    wtk_drft_delete2(sos->drft);
    wtk_strbuf_delete(sos->input);
    wtk_free(sos->output_buf);
    for(i = 0; i < sos->cfg->N; i++){
        wtk_free(sos->output[i]);
    }
    wtk_free(sos->output);
    wtk_free(sos);
}

void qtk_soundfield_syntheis_reset(qtk_soundfield_syntheis_t *sos){
    wtk_strbuf_reset(sos->input);
    memset(sos->prev_half_win, 0, sizeof(float) * sos->cfg->N * sos->cfg->hop_size);
}

static void process_frame_(qtk_soundfield_syntheis_t *sos){
    int N = sos->cfg->N;
    int hop_size = sos->cfg->hop_size;
    int fsize = hop_size * 2;
    int nbin = hop_size + 1;
    int i,j;
    float *data = (float*)sos->input->data;
    wtk_complex_t *ifft_in;
    wtk_complex_t *driving_func;
    float *ifft_buf;
    float *output;
    short *o;
    float *prev_half_win;
    qtk_soundfield_syntheis_cfg_t *cfg = sos->cfg;
    float scale = 32768.0 / cfg->scale;
    for(i = 0; i < fsize; i++){
        sos->fft_in[i] = data[i] * cfg->window[i];
    }

    wtk_drft_fft22(sos->drft, sos->fft_in, sos->fft_buf);
    for(i = 0; i < nbin; i++){
        sos->fft_buf[i].a *= fsize;
        sos->fft_buf[i].b *= fsize;
    }
    driving_func = sos->driving_func;
    for(i = 0; i < N; i++){
        ifft_in = sos->ifft_in + i * nbin;
        //driving_func = sos->driving_func + i * nbin;
        ifft_buf = sos->ifft_buf + i * hop_size * 2;
        output = sos->output_buf + i * hop_size;
        o = sos->output[i];
        prev_half_win = sos->prev_half_win + i * hop_size;
        for(int j = 0; j < nbin; j++){
            ifft_in[j].a = sos->fft_buf[j].a * driving_func[j * N + i].a - sos->fft_buf[j].b * driving_func[j * N + i].b;
            ifft_in[j].b = sos->fft_buf[j].a * driving_func[j * N + i].b + sos->fft_buf[j].b * driving_func[j * N + i].a;
        }
        //complex_dump(ifft_in, nbin);
        wtk_drft_ifft22(sos->drft, ifft_in, ifft_buf);

        for(j = 0; j < fsize; j++){
            ifft_buf[j] /= fsize;
        }
        //print_float(ifft_buf,cfg->hop_size * 2);
        for(j = 0; j < hop_size; j++){
            output[j] = (ifft_buf[j] * cfg->window[j] + prev_half_win[j] * cfg->window[j + cfg->hop_size]) / cfg->win_gain[j];
            o[j] = output[j] * scale;
        }
        //print_float(output,cfg->hop_size);
        memcpy(prev_half_win, ifft_buf + cfg->hop_size, sizeof(float) * cfg->hop_size);
    }
}

void qtk_soundfield_syntheis_feed(qtk_soundfield_syntheis_t *sos, short *data, int len){

    wtk_strbuf_t *input = sos->input;
    int i;
    float fv;
    for(i = 0;i < len;++i)
    {
        fv = data[i]/32768.0;
        wtk_strbuf_push(input,(char *)(&fv),sizeof(float));
    }

    int wav_len = input->pos/sizeof(float);
	int fsize = sos->cfg->hop_size * 2;
	while(wav_len > fsize){
        process_frame_(sos);
		// for(i = 0; i < sspot->cfg->hop_size; i++){
		// 	sspot->output[0][i] = sspot->left_frame_delayed[i] * 32768.0;
		// 	sspot->output[1][i] = sspot->right_frame_delayed[i] * 32768.0;
		// }

		if(sos->notify){
			sos->notify(sos->upval, sos->output, sos->cfg->hop_size);
		}

        wtk_strbuf_pop(input, NULL, sos->cfg->hop_size * sizeof(float));
		wav_len = input->pos/sizeof(float);
	}
}

void qtk_soundfield_syntheis_set_notify(qtk_soundfield_syntheis_t *sos, void *upval, qtk_soundfield_syntheis_notify_f notify){
    sos->upval = upval;
    sos->notify = notify;
}