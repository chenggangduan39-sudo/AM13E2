target('qtk')
    on_load(function(target)
        if not has_config('with-onnxruntime') then
            target:remove("files",
                'cv/detection/qtk_cv_detection_onnxruntime.c'
            )
        end
    end)
    add_files(
      'qtk_global.c',
      'core/**.c',
      'camera/**.c',
      'cv/**.c',
      'executor/**.c',
      'image/**.c',
      'linalg/**.c',
      'numeric/**.c',
      'sci/**.c',
      'dns/**.c',
      'os/**.c',
      'ult/**.c',
      'mdl/**.c',
      'actor/**.c',
      'serde/**.c',
      'nnrt/**.c',
      'avspeech/**.c'
    )
    
    if not has_config('with-onnxruntime') then
    	remove_files( 
            'cv/detection/qtk_cv_detection_onnxruntime.c',
            'cv/api/**.c'
    	)
    end
    
    if has_config('with-opencv') then
      add_files('cv/**.cpp', 'cv/**.cc')
      remove_files("cv/qtk_cv_image.cpp")
    else
      remove_files('cv/app/**.c')
    end

    if has_config('with-libtorch') then
        add_files('nnrt/qtk_nnrt_libtorch.cc')
    end

    if has_config('with-recorder') then
        add_files('rcd/**.c')
        add_links('asound')
    end

    if has_config('with-3308rcd') then
	add_files('record/**.c')
        add_links('asound')
    end

    if has_config('with-3308ply') then
	add_files('play/**.c')
    end

    -- TODO remove hptt deps
    if has_config('with-nn') then
        add_files('nn/**.c')
    end

    if has_config('with-asr') then
        add_files('vprint/**.c')
    end

    if has_config('with-tts_device') then
        add_files('tts/acoustic/**.c')
        add_files('tts/dfsmn/**.c')
        add_files('tts/parse/**.c')
        add_files('layer/**.c')
    end
    
    if has_config('with-stitch') then
        add_files("stitch/**.c")
        add_files("stitch/**.cpp")
        -- 先这样  到时要改成包
        add_includedirs("../third/opencv-4.9.0/install/include/opencv4")
        add_linkdirs("../third/opencv-4.9.0/install/lib")
        add_rpathdirs("./third/opencv-4.9.0/install/lib")
        add_links("opencv_core","opencv_features2d","opencv_imgproc","opencv_photo","opencv_imgcodecs","opencv_dnn",
                    "opencv_flann","opencv_videoio","opencv_stitching",
                    "opencv_ml","opencv_objdetect","opencv_calib3d","opencv_highgui")

    end

    if not is_plat('linux') and not is_plat('android') then
        remove_files('camera/qtk_camera_linux.c')
        remove_files('executor/qtk_thread_pool_executor.c')
    end
target_end()
