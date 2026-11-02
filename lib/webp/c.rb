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
    # blocking: true releases Ruby's Global VM Lock while C works, so other
    # threads keep running. The C code never calls back into Ruby.
    attach_function :webp_encode, [:string, :string, :pointer], :int, blocking: true
    attach_function :webp_decode, [:string, :string, :pointer], :int, blocking: true

    # internal: image size check shared by the PNG, JPEG and TIFF readers
    attach_function :check_image_size, :UtilCheckImageSize, [:uint64, :uint64, :uint64, :pointer, :pointer], :int
    # internal: version string of the libtiff in use
    attach_function :tiff_version, :UtilTIFFVersion, [], :string

    attach_function :test_c, [:int], :int
  end
end
