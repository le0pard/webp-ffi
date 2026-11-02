module WebP
  module C
    # enum
    OutputFileFormat = enum( :png, 0,
                  :pam,
                  :ppm,
                  :pgm,
                  :bmp,
                  :tiff,
                  :yuv,
                  :alpha_plane_only )
    # struct
    class FfiWebpEncodeConfig < FFI::Struct
      layout  :lossless, :int,
        :near_lossless, :int,
        :quality, :float,
        :method, :int,
        :target_size, :int,
        :target_PSNR, :float,
        :segments, :int,
        :sns_strength, :int,
        :filter_strength, :int,
        :filter_sharpness, :int,
        :filter_type, :int,
        :autofilter, :int,
        :alpha_compression, :int,
        :alpha_filtering, :int,
        :alpha_quality, :int,
        :pass, :int,
        :show_compressed, :int,
        :preprocessing, :int,
        :partitions, :int,
        :partition_limit, :int,
        # image
        :width, :int,
        :height, :int,
        :crop_x, :int,
        :crop_y, :int,
        :crop_w, :int,
        :crop_h, :int,
        :resize_w, :int,
        :resize_h, :int,
        # -1 keeps the libwebp default
        :preset, :int,
        :image_hint, :int,
        :exact, :int,
        :use_sharp_yuv, :int,
        :thread_level, :int,
        :low_memory, :int,
        :emulate_jpeg_size, :int,
        :lossless_level, :int,
        :metadata, :int
    end

    class FfiWebpFeatures < FFI::Struct
      layout :width, :int,
        :height, :int,
        :has_alpha, :int,
        :has_animation, :int,
        :format, :int
    end

    class FfiWebpDecodeConfig < FFI::Struct
      layout  :output_format, OutputFileFormat,
        :bypass_filtering, :int,
        :no_fancy_upsampling, :int,
        :crop_x, :int,
        :crop_y, :int,
        :crop_w, :int,
        :crop_h, :int,
        :resize_w, :int,
        :resize_h, :int,
        :use_threads, :int
    end

    # webp lib functions
    # libwebp versions packed as 0xMMmmpp
    attach_function :decoder_version, [], :int
    attach_function :encoder_version, [], :int
    attach_function :webp_get_info, [:buffer_in, :size_t, :pointer, :pointer], :int
    attach_function :webp_get_features, [:buffer_in, :size_t, :pointer], :int
    # blocking: true releases Ruby's Global VM Lock while C works, so other
    # threads keep running. The C code never calls back into Ruby.
    # The last argument receives libwebp's reason when a call fails.
    attach_function :webp_encode, [:string, :string, :pointer, :pointer], :int, blocking: true
    attach_function :webp_decode, [:string, :string, :pointer, :pointer], :int, blocking: true
    # in-memory versions: (data, data size, output pointer, output size, options, reason)
    attach_function :webp_encode_data, [:pointer, :size_t, :pointer, :pointer, :pointer, :pointer], :int, blocking: true
    attach_function :webp_decode_data, [:pointer, :size_t, :pointer, :pointer, :pointer, :pointer], :int, blocking: true
    # raw RGBA pixels: (pixels, width, height, output pointer, output size, options, reason)
    attach_function :webp_encode_rgba, [:pointer, :int, :int, :pointer, :pointer, :pointer, :pointer], :int, blocking: true
    # (data, data size, output pointer, output size, width, height, options, reason)
    attach_function :webp_decode_rgba, [:pointer, :size_t, :pointer, :pointer, :pointer, :pointer, :pointer, :pointer], :int, blocking: true
    attach_function :webp_free_data, [:pointer], :void

    # internal: image size check shared by the PNG, JPEG and TIFF readers
    attach_function :check_image_size, :UtilCheckImageSize, [:uint64, :uint64, :uint64, :pointer, :pointer], :int
    # internal: version string of the libtiff in use
    attach_function :tiff_version, :UtilTIFFVersion, [], :string
  end
end
