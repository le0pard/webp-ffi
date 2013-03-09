module WebpFfi
  module C
    # enum
    WebpImageHint = enum(:webp_hint_default, 0,
      :webp_hint_picture,
      :webp_hint_photo,
      :webp_hint_graph,
      :webp_hint_last)
    Vp8StatusCodeEnum = enum(:vp8_status_ok, 0,
      :vp8_status_out_of_memory,
      :vp8_status_invalid_params,
      :vp8_status_bitstream_error,
      :vp8_status_unsupported_feature,
      :vp8_status_suspended,
      :vp8_status_user_abort,
      :vp8_status_not_enought_data)
    WebpEncCsp = enum(:webp_yuv420, 0,
      :webp_yuv422, 1,
      :webp_yuv444, 2,
      :webp_yuv400, 3,
      :webp_csp_uv_mask, 3,
      :webp_yuv420a, 4,
      :webp_yuv422a, 5,
      :webp_yuv444a, 6,
      :webp_yuv400a, 7,
      :webp_csp_alpha_bit, 4)
    WebpEncodingError = enum(:vp8_enc_ok, 0,
      :vp8_enc_error_out_of_memory,
      :vp8_enc_error_bitstream_out_of_memory,
      :vp8_enc_error_null_parameter,
      :vp8_enc_error_invalid_configuration,
      :vp8_enc_error_bad_dimension,
      :vp8_enc_error_partition0_overflow,
      :vp8_enc_error_partition_overflow,
      :vp8_enc_error_bad_write,
      :vp8_enc_error_file_too_big,
      :vp8_enc_error_user_abort,
      :vp8_enc_error_last)
    # struct
    class WebPBitstreamFeatures < FFI::Struct
      layout  :width, :int,
        :height, :int,
        :has_alpha, :int
    end
    
    class WebPConfig < FFI::Struct
      layout  :lossless, :int,
        :quality, :float,
        :method, :int,
        :image_hint, WebpImageHint,
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
        :partition_limit, :int
    end
    
    class WebPPicture < FFI::Struct
      layout  :use_argb, :int,
        :colorspace, WebpEncCsp,
        :width, :int,
        :height, :int,
        :y, :pointer,
        :u, :pointer,
        :v, :pointer,
        :y_stride, :int,
        :uv_stride, :int,
        :a, :pointer,
        :argb, :pointer,
        :argb_stride, :int,
        :error_code, WebpEncodingError
    end
   
    # webp lib functions
    attach_function :decoder_version, [:pointer], :void
    attach_function :encoder_version, [:pointer], :void
    attach_function :webp_get_info, [:pointer, :size_t, :pointer, :pointer], :int
    attach_function :webp_encode_rgb, [:pointer, :int, :int, :int, :float, :pointer], :size_t
    attach_function :webp_encode, [:pointer, :size_t, WebPConfig, :pointer], :int

    attach_function :test, [:int], :int
  end
end
