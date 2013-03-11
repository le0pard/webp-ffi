module WebpFfi
  module C
    # struct
    class FfiWebpEncodeConfig < FFI::Struct
      layout  :lossless, :int,
        :quality, :float,
        :method, :int,
        :segments, :int,
        :sns_strength, :int,
        :alpha_quality, :int,
        :alpha_compression, :int,
        :alpha_filtering, :int,
        :width, :int,
        :height, :int,
        :crop_x, :int,
        :crop_y, :int,
        :crop_w, :int,
        :crop_h, :int,
        :resize_w, :int,
        :resize_h, :int
    end

    # webp lib functions
    attach_function :decoder_version, [:pointer], :void
    attach_function :encoder_version, [:pointer], :void
    attach_function :webp_get_info, [:pointer, :size_t, :pointer, :pointer], :int
    attach_function :webp_encode, [:string, :string, :pointer], :int
    attach_function :webp_decode, [:string, :string], :int

    attach_function :test_c, [:int], :int
  end
end
