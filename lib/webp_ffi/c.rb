module WebpFfi
  module C
    attach_function :WebPGetInfo, [:pointer, :size_t, :pointer, :pointer], :int

    attach_function :test, [:int], :int
  end
end
