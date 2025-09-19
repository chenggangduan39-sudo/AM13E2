#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc.hpp"
extern "C" {
#include "qtk/cv/stitching/qtk_stitching_blender.h"
}

namespace qtk {

static const float WEIGHT_EPS = 1e-5f;

class MultiBandBlender {
  public:
    MultiBandBlender(cv::Rect dst_roi, int num_bands = 5,
                     int weight_type = CV_32F);
    ~MultiBandBlender() { cv::fastFree(pyr_data_); }

    int numBands() const { return actual_num_bands_; }
    void setNumBands(int val) { actual_num_bands_ = val; }

    void setMasks(const std::vector<cv::Mat> &masks,
                  const std::vector<cv::Point> &tls);
    void clearMasks(void) {
        weight_pyr_gauss_.clear();
        sizes_.clear();
        tls_.clear();
        for (int i = 0; i <= num_bands_; i++) {
            dst_band_weights_[i].setTo(0);
        }
    }
    void feed(cv::InputArray img, cv::Point tl, int idx);
    void feed(uint8_t **I);
    void feed(uint8_t *I, int idx);
    void blend(void);
    void reset(void) { memset(pyr_data_, 0, dst_pyr_total_sz_); }
    void setDst(void *dst_ptr) {
        final_dst_ =
            cv::Mat(dst_roi_.size(), CV_8UC3, dst_ptr).getUMat(cv::ACCESS_RW);
    }

  private:
    int actual_num_bands_, num_bands_;
    std::vector<cv::UMat> dst_pyr_laplace_;
    std::vector<cv::UMat> dst_band_weights_;
    cv::Rect dst_roi_;
    int weight_type_; // CV_32F or CV_16S
    cv::UMat final_dst_;
    std::vector<std::vector<cv::UMat>> weight_pyr_gauss_;
    std::vector<cv::Size> sizes_;
    std::vector<cv::Point> tls_;
    void *pyr_data_;
    size_t dst_pyr_total_sz_;
};

MultiBandBlender::MultiBandBlender(cv::Rect dst_roi, int num_bands,
                                   int weight_type) {
    num_bands_ = 0;
    setNumBands(num_bands);
    CV_Assert(weight_type == CV_32F || weight_type == CV_16S);
    weight_type_ = weight_type;
    dst_roi_ = dst_roi;

    // Crop unnecessary bands
    double max_len =
        static_cast<double>(std::max(dst_roi.width, dst_roi.height));
    num_bands_ =
        std::min(actual_num_bands_,
                 static_cast<int>(ceil(std::log(max_len) / std::log(2.0))));

    // Add border to the final image, to ensure sizes are divided by (1 <<
    // num_bands_)
    dst_roi.width += ((1 << num_bands_) - dst_roi.width % (1 << num_bands_)) %
                     (1 << num_bands_);
    dst_roi.height += ((1 << num_bands_) - dst_roi.height % (1 << num_bands_)) %
                      (1 << num_bands_);

    dst_pyr_laplace_.resize(num_bands_ + 1);
    dst_band_weights_.resize(num_bands_ + 1);

    std::vector<cv::Size> dst_pyr_sizes(num_bands_ + 1);
    std::vector<cv::Size> dst_band_sizes(num_bands_ + 1);
    size_t weight_elem_sz = weight_type_ == CV_32F ? 4 : 2;
    dst_band_sizes[0] = dst_roi.size();
    dst_pyr_sizes[0] = dst_roi_.size();
    size_t dst_pyr_total_sz = dst_pyr_sizes[0].area();
    size_t dst_band_total_sz = dst_band_sizes[0].area();
    for (int i = 1; i <= num_bands_; ++i) {
        dst_band_sizes[i] = cv::Size((dst_band_sizes[i - 1].width + 1) / 2,
                                     (dst_band_sizes[i - 1].height + 1) / 2);
        dst_pyr_sizes[i] = cv::Size((dst_pyr_sizes[i - 1].width + 1) / 2,
                                    (dst_pyr_sizes[i - 1].height + 1) / 2);
        dst_pyr_total_sz += dst_pyr_sizes[i].area();
        dst_band_total_sz += dst_band_sizes[i].area();
    }
    dst_pyr_total_sz *= 3 * sizeof(short);
    dst_band_total_sz *= weight_elem_sz;
    dst_pyr_total_sz_ = dst_pyr_total_sz;
    pyr_data_ = cv::fastMalloc(dst_pyr_total_sz + dst_band_total_sz +
                               (weight_type_ == CV_32F ? 4 : 0));
    memset(pyr_data_, 0,
           dst_pyr_total_sz + dst_band_total_sz +
               (weight_type_ == CV_32F ? 4 : 0));
    uint8_t *ptr_cursor = (uint8_t *)pyr_data_;

    for (int i = 0; i <= num_bands_; i++) {
        dst_pyr_laplace_[i] = cv::Mat(dst_pyr_sizes[i], CV_16SC3, ptr_cursor)
                                  .getUMat(cv::ACCESS_RW);
        ptr_cursor += dst_pyr_sizes[i].area() * 3 * sizeof(short);
    }

    if (weight_type_ == CV_32F) {
        ptr_cursor = cv::alignPtr(ptr_cursor, 4);
    }

    for (int i = 0; i <= num_bands_; i++) {
        dst_band_weights_[i] =
            cv::Mat(dst_band_sizes[i], weight_type_, ptr_cursor)
                .getUMat(cv::ACCESS_RW);
        ptr_cursor += dst_band_sizes[i].area() * weight_elem_sz;
    }
}

void MultiBandBlender::setMasks(const std::vector<cv::Mat> &masks,
                                const std::vector<cv::Point> &tls) {
    tls_ = tls;
    int nframe = tls.size();
    std::vector<cv::Rect> frames_rect(nframe);
    sizes_.resize(nframe);
    for (int i = 0; i < nframe; i++) {
        int gap = 3 * (1 << num_bands_);
        const cv::Point &tl = tls[i];
        int H = masks[i].rows;
        int W = masks[i].cols;
        sizes_[i] = cv::Size(W, H);
        cv::Point tl_new(std::max(dst_roi_.x, tl.x - gap),
                         std::max(dst_roi_.y, tl.y - gap));
        cv::Point br_new(std::min(dst_roi_.br().x, tl.x + W + gap),
                         std::min(dst_roi_.br().y, tl.y + H + gap));

        // Ensure coordinates of top-left, bottom-right corners are divided by
        // (1 << num_bands_). After that scale between layers is exactly 2.
        //
        // We do it to avoid interpolation problems when keeping sub-images
        // only. There is no such problem when image is bordered to have size
        // equal to the final image size, but this is too memory hungry
        // approach.
        tl_new.x = dst_roi_.x +
                   (((tl_new.x - dst_roi_.x) >> num_bands_) << num_bands_);
        tl_new.y = dst_roi_.y +
                   (((tl_new.y - dst_roi_.y) >> num_bands_) << num_bands_);
        int width = br_new.x - tl_new.x;
        int height = br_new.y - tl_new.y;
        width +=
            ((1 << num_bands_) - width % (1 << num_bands_)) % (1 << num_bands_);
        height += ((1 << num_bands_) - height % (1 << num_bands_)) %
                  (1 << num_bands_);
        br_new.x = tl_new.x + width;
        br_new.y = tl_new.y + height;
        int dy = std::max(br_new.y - dst_roi_.br().y, 0);
        int dx = std::max(br_new.x - dst_roi_.br().x, 0);
        tl_new.x -= dx;
        br_new.x -= dx;
        tl_new.y -= dy;
        br_new.y -= dy;

        int top = tl.y - tl_new.y;
        int left = tl.x - tl_new.x;
        int bottom = br_new.y - tl.y - H;
        int right = br_new.x - tl.x - W;

        std::vector<cv::UMat> weight_pyr_gauss(num_bands_ + 1);
        cv::UMat weight_map;
        if (weight_type_ == CV_32F) {
            masks[i].convertTo(weight_map, CV_32F, 1. / 255.);
        } else // weight_type_ == CV_16S
        {
            masks[i].convertTo(weight_map, CV_16S);
            cv::UMat add_mask;
            compare(masks[i], 0, add_mask, cv::CMP_NE);
            add(weight_map, cv::Scalar::all(1), weight_map, add_mask);
        }

        copyMakeBorder(weight_map, weight_pyr_gauss[0], top, bottom, left,
                       right, cv::BORDER_CONSTANT);

        for (int i = 0; i < num_bands_; ++i)
            pyrDown(weight_pyr_gauss[i], weight_pyr_gauss[i + 1]);

        weight_pyr_gauss_.emplace_back(weight_pyr_gauss);

        int y_tl = tl_new.y - dst_roi_.y;
        int y_br = br_new.y - dst_roi_.y;
        int x_tl = tl_new.x - dst_roi_.x;
        int x_br = br_new.x - dst_roi_.x;
        frames_rect[i] = cv::Rect(x_tl, y_tl, x_br - x_tl, y_br - y_tl);

        for (int i = 0; i <= num_bands_; ++i) {
            cv::Rect rc(x_tl, y_tl, x_br - x_tl, y_br - y_tl);
            cv::Mat _dst_band_weights =
                dst_band_weights_[i](rc).getMat(cv::ACCESS_RW);
            cv::Mat _weight_pyr_gauss =
                weight_pyr_gauss[i].getMat(cv::ACCESS_READ);
            if (weight_type_ == CV_32F) {
                for (int y = 0; y < rc.height; ++y) {
                    const float *weight_row = _weight_pyr_gauss.ptr<float>(y);
                    float *dst_weight_row = _dst_band_weights.ptr<float>(y);
                    for (int x = 0; x < rc.width; ++x) {
                        dst_weight_row[x] += weight_row[x];
                    }
                }
            } else // weight_type_ == CV_16S
            {
                for (int y = 0; y < y_br - y_tl; ++y) {
                    const short *weight_row = _weight_pyr_gauss.ptr<short>(y);
                    short *dst_weight_row = _dst_band_weights.ptr<short>(y);
                    for (int x = 0; x < x_br - x_tl; ++x) {
                        dst_weight_row[x] += weight_row[x];
                    }
                }
            }
            x_tl /= 2;
            y_tl /= 2;
            x_br /= 2;
            y_br /= 2;
        }
    }

    // normalize weight_pyr
    for (int i = 0; i < nframe; i++) {
        cv::Rect rc = frames_rect[i];
        for (int k = 0; k <= num_bands_; ++k) {
            cv::Mat _weight_pyr_gauss =
                weight_pyr_gauss_[i][k].getMat(cv::ACCESS_RW);
            cv::Mat _dst_band_weights =
                dst_band_weights_[k](rc).getMat(cv::ACCESS_READ);
            if (weight_type_ == CV_32F) {
                for (int y = 0; y < rc.height; ++y) {
                    float *weight_row = _weight_pyr_gauss.ptr<float>(y);
                    const float *dst_weight_row =
                        _dst_band_weights.ptr<float>(y);
                    for (int x = 0; x < rc.width; ++x) {
                        weight_row[x] /= dst_weight_row[x] + WEIGHT_EPS;
                    }
                }
            } else {
                for (int y = 0; y < rc.height; ++y) {
                    short *weight_row = _weight_pyr_gauss.ptr<short>(y);
                    const short *dst_weight_row =
                        _dst_band_weights.ptr<short>(y);
                    for (int x = 0; x < rc.width; ++x) {
                        weight_row[x] =
                            ((float)weight_row[x] / dst_weight_row[x]) * 256.0;
                    }
                }
            }
            rc.x /= 2;
            rc.y /= 2;
            rc.height /= 2;
            rc.width /= 2;
        }
    }
}

void createLaplacePyr(cv::InputArray img, int num_levels,
                      std::vector<cv::UMat> &pyr) {
    pyr.resize(num_levels + 1);

    if (img.depth() == CV_8U) {
        if (num_levels == 0) {
            img.getUMat().convertTo(pyr[0], CV_16S);
            return;
        }

        cv::UMat downNext;
        cv::UMat current = img.getUMat();
        pyrDown(img, downNext);

        for (int i = 1; i < num_levels; ++i) {
            cv::UMat lvl_up;
            cv::UMat lvl_down;

            pyrDown(downNext, lvl_down);
            pyrUp(downNext, lvl_up, current.size());
            subtract(current, lvl_up, pyr[i - 1], cv::noArray(), CV_16S);

            current = downNext;
            downNext = lvl_down;
        }

        {
            cv::UMat lvl_up;
            pyrUp(downNext, lvl_up, current.size());
            subtract(current, lvl_up, pyr[num_levels - 1], cv::noArray(),
                     CV_16S);

            downNext.convertTo(pyr[num_levels], CV_16S);
        }
    } else {
        pyr[0] = img.getUMat();
        for (int i = 0; i < num_levels; ++i)
            pyrDown(pyr[i], pyr[i + 1]);
        cv::UMat tmp;
        for (int i = 0; i < num_levels; ++i) {
            pyrUp(pyr[i + 1], tmp, pyr[i].size());
            subtract(pyr[i], tmp, pyr[i]);
        }
    }
}

void MultiBandBlender::feed(uint8_t *image, int idx) {
    feed(cv::Mat(sizes_[idx], CV_8UC3, image), tls_[idx], idx);
}

void MultiBandBlender::feed(uint8_t **image) {
    for (size_t idx = 0; idx < sizes_.size(); idx++) {
        feed(image[idx], idx);
    }
}

void MultiBandBlender::feed(cv::InputArray _img, cv::Point tl, int idx = 0) {
    cv::UMat img = _img.getUMat();
    CV_Assert(img.type() == CV_16SC3 || img.type() == CV_8UC3);

    // Keep source image in memory with small border
    int gap = 3 * (1 << num_bands_);
    cv::Point tl_new(std::max(dst_roi_.x, tl.x - gap),
                     std::max(dst_roi_.y, tl.y - gap));
    cv::Point br_new(std::min(dst_roi_.br().x, tl.x + img.cols + gap),
                     std::min(dst_roi_.br().y, tl.y + img.rows + gap));

    // Ensure coordinates of top-left, bottom-right corners are divided by (1 <<
    // num_bands_). After that scale between layers is exactly 2.
    //
    // We do it to avoid interpolation problems when keeping sub-images only.
    // There is no such problem when image is bordered to have size equal to the
    // final image size, but this is too memory hungry approach.
    tl_new.x =
        dst_roi_.x + (((tl_new.x - dst_roi_.x) >> num_bands_) << num_bands_);
    tl_new.y =
        dst_roi_.y + (((tl_new.y - dst_roi_.y) >> num_bands_) << num_bands_);
    int width = br_new.x - tl_new.x;
    int height = br_new.y - tl_new.y;
    width +=
        ((1 << num_bands_) - width % (1 << num_bands_)) % (1 << num_bands_);
    height +=
        ((1 << num_bands_) - height % (1 << num_bands_)) % (1 << num_bands_);
    br_new.x = tl_new.x + width;
    br_new.y = tl_new.y + height;
    int dy = std::max(br_new.y - dst_roi_.br().y, 0);
    int dx = std::max(br_new.x - dst_roi_.br().x, 0);
    tl_new.x -= dx;
    br_new.x -= dx;
    tl_new.y -= dy;
    br_new.y -= dy;

    int top = tl.y - tl_new.y;
    int left = tl.x - tl_new.x;
    int bottom = br_new.y - tl.y - img.rows;
    int right = br_new.x - tl.x - img.cols;

    // Create the source image Laplacian pyramid
    cv::UMat img_with_border;
    copyMakeBorder(_img, img_with_border, top, bottom, left, right,
                   cv::BORDER_REFLECT);
    std::vector<cv::UMat> src_pyr_laplace;
    createLaplacePyr(img_with_border, num_bands_, src_pyr_laplace);

    std::vector<cv::UMat> weight_pyr_gauss = weight_pyr_gauss_[idx];

    int y_tl = tl_new.y - dst_roi_.y;
    int y_br = br_new.y - dst_roi_.y;
    int x_tl = tl_new.x - dst_roi_.x;
    int x_br = br_new.x - dst_roi_.x;

    // Add weighted layer of the source image to the final Laplacian pyramid
    // layer
    for (int i = 0; i <= num_bands_; ++i) {
        cv::Rect rc(x_tl, y_tl, x_br - x_tl, y_br - y_tl);
        {
            cv::Mat _src_pyr_laplace =
                src_pyr_laplace[i].getMat(cv::ACCESS_READ);
            cv::Mat _dst_pyr_laplace =
                dst_pyr_laplace_[i](rc).getMat(cv::ACCESS_RW);
            cv::Mat _weight_pyr_gauss =
                weight_pyr_gauss[i].getMat(cv::ACCESS_READ);
            if (weight_type_ == CV_32F) {
                for (int y = 0; y < rc.height; ++y) {
                    const cv::Point3_<short> *src_row =
                        _src_pyr_laplace.ptr<cv::Point3_<short>>(y);
                    cv::Point3_<short> *dst_row =
                        _dst_pyr_laplace.ptr<cv::Point3_<short>>(y);
                    const float *weight_row = _weight_pyr_gauss.ptr<float>(y);

                    for (int x = 0; x < rc.width; ++x) {
                        dst_row[x].x +=
                            static_cast<short>(src_row[x].x * weight_row[x]);
                        dst_row[x].y +=
                            static_cast<short>(src_row[x].y * weight_row[x]);
                        dst_row[x].z +=
                            static_cast<short>(src_row[x].z * weight_row[x]);
                    }
                }
            } else // weight_type_ == CV_16S
            {
                for (int y = 0; y < y_br - y_tl; ++y) {
                    const cv::Point3_<short> *src_row =
                        _src_pyr_laplace.ptr<cv::Point3_<short>>(y);
                    cv::Point3_<short> *dst_row =
                        _dst_pyr_laplace.ptr<cv::Point3_<short>>(y);
                    const short *weight_row = _weight_pyr_gauss.ptr<short>(y);

                    for (int x = 0; x < x_br - x_tl; ++x) {
                        dst_row[x].x +=
                            short((src_row[x].x * weight_row[x]) >> 8);
                        dst_row[x].y +=
                            short((src_row[x].y * weight_row[x]) >> 8);
                        dst_row[x].z +=
                            short((src_row[x].z * weight_row[x]) >> 8);
                    }
                }
            }
        }

        x_tl /= 2;
        y_tl /= 2;
        x_br /= 2;
        y_br /= 2;
    }
}

void restoreImageFromLaplacePyr(std::vector<cv::UMat> &pyr) {
    if (pyr.empty())
        return;
    cv::UMat tmp;
    for (size_t i = pyr.size() - 1; i > 0; --i) {
        pyrUp(pyr[i], tmp, pyr[i - 1].size());
        add(tmp, pyr[i - 1], pyr[i - 1]);
    }
}

void MultiBandBlender::blend(void) {
    cv::Rect dst_rc(0, 0, dst_roi_.width, dst_roi_.height);
    restoreImageFromLaplacePyr(dst_pyr_laplace_);
    cv::convertScaleAbs(dst_pyr_laplace_[0](dst_rc), final_dst_);
}

} // namespace qtk

extern "C" {
qtk_stitching_blender_t *
qtk_stitching_blender_new(qtk_stitching_blender_cfg_t *cfg) {
    qtk_stitching_blender_t *b =
        (qtk_stitching_blender_t *)wtk_malloc(sizeof(qtk_stitching_blender_t));
    b->cfg = cfg;
    b->impl = new qtk::MultiBandBlender(
        cv::Rect(0, 0, cfg->dst_width, cfg->dst_height), cfg->nband, CV_16S);
    return b;
}

void qtk_stitching_blender_delete(qtk_stitching_blender_t *b) {
    delete (qtk::MultiBandBlender *)b->impl;
    wtk_free(b);
}

void qtk_stitching_blender_set_dst(qtk_stitching_blender_t *b, uint8_t *dst) {
    qtk::MultiBandBlender *mb = (qtk::MultiBandBlender *)b->impl;
    mb->setDst(dst);
}

int qtk_stitching_blender_set_masks(qtk_stitching_blender_t *b, int nframe,
                                    uint8_t **masks, int *tls, int *sizes) {
    qtk::MultiBandBlender *mb = (qtk::MultiBandBlender *)b->impl;
    std::vector<cv::Mat> vmasks(nframe);
    std::vector<cv::Point> vtls(nframe);
    for (int i = 0; i < nframe; i++) {
        int H = sizes[i * 2 + 0];
        int W = sizes[i * 2 + 1];
        vmasks[i] = cv::Mat(H, W, CV_8U, masks[i]);
        vtls[i] = cv::Point(tls[i * 2 + 0], tls[i * 2 + 1]);
    }
    mb->setMasks(vmasks, vtls);
    return 0;
}

int qtk_stitching_blender_clear_masks(qtk_stitching_blender_t *b) {
    qtk::MultiBandBlender *mb = (qtk::MultiBandBlender *)b->impl;
    mb->clearMasks();
    return 0;
}

void qtk_stitching_blender_blend(qtk_stitching_blender_t *b) {
    qtk::MultiBandBlender *mb = (qtk::MultiBandBlender *)b->impl;
    mb->blend();
}

int qtk_stitching_blender_feed(qtk_stitching_blender_t *b, uint8_t **image) {
    qtk::MultiBandBlender *mb = (qtk::MultiBandBlender *)b->impl;
    mb->feed(image);
    return 0;
}

int qtk_stitching_blender_feed_single(qtk_stitching_blender_t *b,
                                      uint8_t *image, int idx) {
    qtk::MultiBandBlender *mb = (qtk::MultiBandBlender *)b->impl;
    mb->feed(image, idx);
    return 0;
}

void qtk_stitching_blender_reset(qtk_stitching_blender_t *b) {
    qtk::MultiBandBlender *mb = (qtk::MultiBandBlender *)b->impl;
    mb->reset();
}
}
