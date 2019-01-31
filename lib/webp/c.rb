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
        :resize_h, :int
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
    attach_function :decoder_version, [:pointer], :void
    attach_function :encoder_version, [:pointer], :void
    attach_function :webp_get_info, [:pointer, :size_t, :pointer, :pointer], :int
    attach_function :webp_encode, [:string, :string, :pointer], :int
    attach_function :webp_decode, [:string, :string, :pointer], :int

    attach_function :test_c, [:int], :int
  end
end
