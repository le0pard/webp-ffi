module WebpFfi
  module C
    # struct
    class FfiWebpEncodeConfig < FFI::Struct
      layout  :lossless, :int,
        :quality, :float,
        :method, :int,
        :segments, :int,
        :sns_strength, :int,
        :alpha_quality, :int
    end
   
    # webp lib functions
    attach_function :decoder_version, [:pointer], :void
    attach_function :encoder_version, [:pointer], :void
    attach_function :webp_get_info, [:pointer, :size_t, :pointer, :pointer], :int
    attach_function :webp_decode, [:string, :string], :int
    attach_function :webp_encode, [:string, :string, :pointer], :int

    attach_function :test, [:int], :int
  end
end
