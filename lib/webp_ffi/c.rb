module WebpFfi
  module C
    # enum
    WebpImageHint = enum(:webp_hint_default, 0,
      :webp_hint_picture,
      :webp_hint_photo,
      :webp_hint_graph,
      :webp_hint_last)
    # struct
    class FfiWebpConfig < FFI::Struct
      layout  :lossless, :int,
        :quality, :float,
        :method, :int
    end
   
    # webp lib functions
    attach_function :decoder_version, [:pointer], :void
    attach_function :encoder_version, [:pointer], :void
    attach_function :webp_get_info, [:pointer, :size_t, :pointer, :pointer], :int
    attach_function :webp_decode, [:pointer, :size_t, :pointer, :pointer], :int
    attach_function :webp_encode, [:pointer, :size_t, FfiWebpConfig, :pointer], :int

    attach_function :test, [:int], :int
  end
end
