module WebpFfi
  module C
    # enum
    vp8_status_code_enum = enum(:vp8_status_ok, 0,
      :vp8_status_out_of_memory,
      :vp8_status_invalid_params,
      :vp8_status_bitstream_error,
      :vp8_status_unsupported_feature,
      :vp8_status_suspended,
      :vp8_status_user_abort,
      :vp8_status_not_enought_data)
    # struct
    class WebPBitstreamFeatures < FFI::Struct
      layout  :width, :int,
        :height, :int,
        :has_alpha, :int
    end
    
    # webp lib functions
    attach_function :WebPGetInfo, [:pointer, :size_t, :pointer, :pointer], :int
    
    attach_function :webp_get_features, [:pointer, :size_t, :pointer], vp8_status_code_enum

    attach_function :test, [:int], :int
  end
end
