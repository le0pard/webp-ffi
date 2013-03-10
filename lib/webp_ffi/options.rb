module WebpFfi
  class Options
    
    def initialize(options)
      @user_options = options
    end
    
    def encode_pointer
      options_pointer = FFI::MemoryPointer.new :char, C::FfiWebpEncodeConfig.size, false
      options_struct = C::FfiWebpEncodeConfig.new options_pointer
      options_struct[:lossless] = @user_options[:lossless] if @user_options[:lossless]
      options_struct[:quality] = @user_options[:quality] if @user_options[:quality]
      options_struct[:method] = @user_options[:method] if @user_options[:method]
      options_struct[:segments] = @user_options[:segments] if @user_options[:segments]
      options_struct[:sns_strength] = @user_options[:sns_strength] if @user_options[:sns_strength]
      options_struct[:alpha_quality] = @user_options[:alpha_quality] if @user_options[:alpha_quality]
      options_pointer
    end
    
  end
end
