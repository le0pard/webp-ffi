module WebpFfi
  module C
    attach_function :webp_get_info, [:pointer, :size_t, :pointer, :pointer], :int

    attach_function :test, [:int], :int
  end
end
