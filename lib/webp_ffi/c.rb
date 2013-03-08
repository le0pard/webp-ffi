module WebpFfi
  module C
    # struct
    class WebPBitstreamFeatures < FFI::Struct
       layout  :width, :int,
              :height, :int,
              :has_alpha, :int
    end
    
    
    # webp lib functions
    attach_function :WebPGetInfo, [:pointer, :size_t, :pointer, :pointer], :int
    
    attach_function :webp_get_features, [:pointer, :size_t, :pointer], :int

    attach_function :test, [:int], :int
  end
end
