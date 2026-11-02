module WebP
  # Returned by WebP.info.
  Info = Data.define(:width, :height, :format, :alpha, :animated, :metadata) do
    # :lossy, :lossless or :mixed (animations can mix both)
    def lossless? = format == :lossless
    def alpha? = alpha
    def animated? = animated
  end

  # Returned by WebP.decode_rgba: 'rgba' holds width * height * 4 bytes, row
  # after row, as a binary String.
  RGBAImage = Data.define(:rgba, :width, :height)
end
