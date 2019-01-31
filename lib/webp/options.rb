module WebP
  class Options

    def initialize(options)
      @user_options = options
    end

    def encode_pointer
      options_pointer = FFI::MemoryPointer.new :char, C::FfiWebpEncodeConfig.size, false
      options_struct = C::FfiWebpEncodeConfig.new options_pointer
      [:lossless, :near_lossless, :method, :target_size, :target_PSNR, :segments,
        :sns_strength, :filter_strength, :filter_sharpness,
        :filter_type, :autofilter, :alpha_compression, :alpha_filtering,
        :alpha_quality, :pass, :show_compressed, :preprocessing, :partitions,
        :partition_limit, :width, :height].each do |key|
          options_struct[key] = @user_options[key] ? @user_options[key] : -1
      end
      encode_default(options_struct)
      [:quality, :crop_x, :crop_y, :crop_w,
        :crop_h, :resize_w, :resize_h].each do |key|
          options_struct[key] = @user_options[key] if @user_options[key]
      end
      options_pointer
    end

    def decode_pointer
      options_pointer = FFI::MemoryPointer.new :char, C::FfiWebpDecodeConfig.size, false
      options_struct = C::FfiWebpDecodeConfig.new options_pointer
      decode_default(options_struct)
      # options
      if @user_options[:output_format] && [:png, :pam, :ppm, :pgm, :bmp, :tiff, :yuv, :alpha_plane_only].include?(@user_options[:output_format])
        options_struct[:output_format] = C::OutputFileFormat[@user_options[:output_format]]
      end
      [:bypass_filtering, :no_fancy_upsampling, :use_threads].each do |key|
        options_struct[key] = 1 if @user_options[key] && true == @user_options[key]
      end
      [:crop_x, :crop_y, :crop_w,
        :crop_h, :resize_w, :resize_h].each do |key|
          options_struct[key] = @user_options[key] if @user_options[key]
      end
      options_pointer
    end

    private

    def encode_default(options_struct)
      options_struct[:quality] = 100
      similar_default(options_struct)
    end

    def decode_default(options_struct)
      # default format is png
      options_struct[:output_format] = C::OutputFileFormat[:png]
      similar_default(options_struct)
    end

    def similar_default(options_struct)
      options_struct[:crop_x] = options_struct[:crop_y] = 0
      options_struct[:crop_w] = options_struct[:crop_h] = 0
      options_struct[:resize_w] = options_struct[:resize_h] = 0
    end

  end
end
