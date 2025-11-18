#include "qtk/cv/utils/qtk_lip_cropper.h"
#include "qtk/cv/geometric/qtk_geometric_transform.h"
#include "qtk/image/qtk_image.h"
#include "qtk/math/qtk_math.h"

static void float_swap(float *x, float *y) {
  float tmp = *x;
  *x = *y;
  *y = tmp;
}

static float float_clip(float x, float l, float h) {
  return x < l ? l : x > h ? h : x;
}

static float default_ref_pts[10] = {
    38.29459953 * 2, 51.69630051 * 2, 73.53179932 * 2, 51.50139999 * 2,
    56.02519989 * 2, 71.73660278 * 2, 41.54930115 * 2, 92.3655014 * 2,
    70.72990036 * 2, 92.20410156 * 2};

static uint8_t get_pixel_(const uint8_t *src, int y, int x, int k, int h, int w,
                          float *mat, int channel) {
#define M(x, y) mat[(x) * 3 + (y)]
#define GETP(x, y) src[(y) * (w) * channel + (x) * channel + k]
    float fx = x * M(0, 0) + y * M(0, 1) + M(0, 2);
    float fy = x * M(1, 0) + y * M(1, 1) + M(1, 2);
    int low_x = floorf(fx);
    int high_x = ceilf(fx);
    int low_y = floorf(fy);
    int high_y = ceilf(fy);
    float pos_x = fx - low_x;
    float pos_y = fy - low_y;
    if (low_x >= 0 && high_x < w && low_y >= 0 && high_y < h) {
        float p0_area = (1 - pos_x) * (1 - pos_y);
        float p1_area = (pos_x) * (1 - pos_y);
        float p2_area = (1 - pos_x) * (pos_y);
        float p3_area = (pos_x) * (pos_y);
        // p0        p1
        //       p
        // p2        p3
        float pixel =
            GETP(low_x, low_y) * p0_area + GETP(high_x, low_y) * p1_area +
            GETP(low_x, high_y) * p2_area + GETP(high_x, high_y) * p3_area;
        return QBL_USAT8(pixel);
    }
#undef M
#undef GETP
    return 0;
}

void qtk_lip_cropper_process(const uint8_t *I, uint8_t *dst, int H, int W,
                             float *landmark, int nborder) {
    float landmark_transformed[10];
    float central_y;
    int i, j;
    int from_H = 224, from_W = 224;
    float tf_mat[3 * 3] = {0.0};
    float inv_mat[6];

    qtk_geometric_similarity_transform(landmark, default_ref_pts, 5, 2, tf_mat);
    for (i = 0; i < 5; i++) {
        float x = landmark[i * 2 + 0];
        float y = landmark[i * 2 + 1];
        landmark_transformed[i * 2 + 0] =
            tf_mat[0] * x + tf_mat[1] * y + tf_mat[2];
        landmark_transformed[i * 2 + 1] =
            tf_mat[3] * x + tf_mat[4] * y + tf_mat[5];
    }

    // float X_left = landmark_transformed[6];
    // float Y_left = landmark_transformed[7];
    // float X_right = landmark_transformed[8];
    // float Y_right = landmark_transformed[9];
    // float X_center = (X_left + X_right) / 2;
    // float Y_center = (Y_left + Y_right) / 2;
    // float X_left_new = X_left - border;
    // float Y_left_new = Y_left - border;
    // float X_right_new = X_right + border;
    // float Y_right_new = Y_right + border;
    // float width_new = X_right_new - X_left_new;
    // float height_new = Y_right_new - Y_left_new;
    // int X_left_crop = max(0, (int)(X_center - width_new / 2));
    // int X_right_crop = min(from_W, (int)(X_center + width_new / 2));
    // int Y_left_crop = max(0, (int)(Y_center - height_new / 2));
    // int Y_right_crop = min(from_H, (int)(Y_center + height_new / 2));
    // int new_H = Y_right_crop - Y_left_crop;
    // int new_W = X_right_crop - X_left_crop;

    float X_left = float_clip(landmark_transformed[6], 0, from_W);
    float Y_left = float_clip(landmark_transformed[7], 0, from_H);
    float X_right = float_clip(landmark_transformed[8], 0, from_W);
    float Y_right = float_clip(landmark_transformed[9], 0, from_H);
    if (X_right < X_left) {
        float_swap(&X_left, &X_right);
    }
    central_y = (Y_left + Y_right) / 2;
    float border = (X_right - X_left) * 0.2;
    if (border < 10) {
        border = 10;
    }
    int X_left_new = max(0, (int)(X_left - border));
    int X_right_new = min((int)(X_right + border), from_W);
    float Y_border = (X_left_new + X_right_new) / 2.0 - X_left_new;
    if (Y_border < 10) {
        Y_border = 10;
    }
    int Y_left_new = max(0, (int)(central_y - Y_border));
    int Y_right_new = min((int)(central_y + Y_border), from_H);
    int new_H = Y_right_new - Y_left_new;
    int new_W = X_right_new - X_left_new;

    qtk_geometric_invert_affine_transform(tf_mat, inv_mat);

    int tmp_patch_alloc = 0;
    int tmp_patch_size = new_H * new_W * sizeof(uint8_t);
    uint8_t tmp_patch_buf[8192];
    uint8_t *tmp_patch = tmp_patch_buf;
    if (tmp_patch_size) {
        tmp_patch_alloc = 1;
        tmp_patch = wtk_malloc(tmp_patch_size);
    }

    uint8_t *tmp_patch_ptr = tmp_patch;
    for (i = Y_left_new; i < Y_right_new; i++) {
        for (j = X_left_new; j < X_right_new; j++) {
            uint8_t R = get_pixel_(I, i, j, 0, H, W, inv_mat, 3);
            uint8_t G = get_pixel_(I, i, j, 1, H, W, inv_mat, 3);
            uint8_t B = get_pixel_(I, i, j, 2, H, W, inv_mat, 3);
            *tmp_patch_ptr++ =
                cast(uint8_t, (R * 6969 + G * 23434 + B * 2365) >> 15);
        }
    }
    qtk_image_desc_t desc = {
        .fmt = QBL_IMAGE_GRAY8,
        .height = new_H,
        .width = new_W,
        .channel = 1,
    };
    qtk_image_resize(&desc, tmp_patch, 88, 88, dst);
    if (tmp_patch_alloc) {
        wtk_free(tmp_patch);
    }
}
