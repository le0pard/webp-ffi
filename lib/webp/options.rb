module WebP
  class Options

    # integer options; -1 tells C to keep the libwebp default
    ENCODE_INT_OPTIONS = [:lossless, :near_lossless, :method, :target_size, :segments,
      :sns_strength, :filter_strength, :filter_sharpness,
      :filter_type, :autofilter, :alpha_compression, :alpha_filtering,
      :alpha_quality, :pass, :show_compressed, :preprocessing, :partitions,
      :partition_limit, :width, :height,
      :exact, :use_sharp_yuv, :thread_level, :low_memory, :emulate_jpeg_size].freeze
    ENCODE_FLOAT_OPTIONS = [:quality, :target_PSNR].freeze
    CROP_RESIZE_OPTIONS = [:crop_x, :crop_y, :crop_w, :crop_h, :resize_w, :resize_h].freeze
    # options that take a name or a list of names
    ENCODE_NAMED_OPTIONS = [:preset, :image_hint, :level, :metadata].freeze
    ENCODE_OPTIONS = (ENCODE_INT_OPTIONS + ENCODE_FLOAT_OPTIONS + CROP_RESIZE_OPTIONS + ENCODE_NAMED_OPTIONS).freeze
    # raw pixels come with their own size and have no metadata
    ENCODE_RGBA_OPTIONS = (ENCODE_OPTIONS - [:width, :height, :metadata]).freeze

    DECODE_FLAGS = [:bypass_filtering, :no_fancy_upsampling, :use_threads].freeze
    DECODE_OPTIONS = ([:output_format] + DECODE_FLAGS + CROP_RESIZE_OPTIONS).freeze
    DECODE_RGBA_OPTIONS = (DECODE_OPTIONS - [:output_format]).freeze
    OUTPUT_FORMATS = [:png, :pam, :ppm, :pgm, :bmp, :tiff, :yuv, :alpha_plane_only].freeze

    # libwebp's WebPPreset, WebPImageHint and the METADATA_* flags in metadata.h
    PRESETS = { default: 0, picture: 1, photo: 2, drawing: 3, icon: 4, text: 5 }.freeze
    IMAGE_HINTS = { default: 0, picture: 1, photo: 2, graph: 3 }.freeze
    METADATA = { exif: 1, icc: 2, xmp: 4 }.freeze

    def initialize(options)
      @user_options = options
    end

    def encode_pointer(known = ENCODE_OPTIONS)
      check_unknown_options(known, "encode")
      options_pointer = FFI::MemoryPointer.new :char, C::FfiWebpEncodeConfig.size, true
      options_struct = C::FfiWebpEncodeConfig.new options_pointer
      (ENCODE_INT_OPTIONS + [:target_PSNR]).each do |key|
        value = number_option(key)
        options_struct[key] = value.nil? ? -1 : value
      end
      encode_default(options_struct)
      CROP_RESIZE_OPTIONS.each do |key|
        value = number_option(key)
        options_struct[key] = value unless value.nil?
      end
      level = level_option
      quality = number_option(:quality)
      # a lossless level picks its own quality, so only override it when asked
      options_struct[:quality] = quality.nil? ? (level >= 0 ? -1 : 100) : quality
      options_struct[:lossless_level] = level
      options_struct[:preset] = named_option(:preset, PRESETS)
      options_struct[:image_hint] = named_option(:image_hint, IMAGE_HINTS)
      options_struct[:metadata] = metadata_option
      options_pointer
    end

    def decode_pointer(known = DECODE_OPTIONS)
      check_unknown_options(known, "decode")
      options_pointer = FFI::MemoryPointer.new :char, C::FfiWebpDecodeConfig.size, true
      options_struct = C::FfiWebpDecodeConfig.new options_pointer
      decode_default(options_struct)
      options_struct[:output_format] = C::OutputFileFormat[output_format] if @user_options[:output_format]
      # set every flag explicitly, so C never reads a field we did not write
      DECODE_FLAGS.each do |key|
        options_struct[key] = flag_option(key)
      end
      CROP_RESIZE_OPTIONS.each do |key|
        value = number_option(key)
        options_struct[key] = value unless value.nil?
      end
      options_pointer
    end

    private

    # A typo like :cropt_y used to be ignored silently, so unknown options raise.
    def check_unknown_options(known, operation)
      unknown = @user_options.keys - known
      return if unknown.empty?
      details = unknown.map { |key| "#{key.inspect}#{suggestion(key, known)}" }
      raise ArgumentError, "webp-ffi: unknown #{operation} option#{'s' if unknown.size > 1} #{details.join(', ')}"
    end

    def suggestion(key, known)
      return "" unless defined?(DidYouMean::SpellChecker)
      match = DidYouMean::SpellChecker.new(dictionary: known).correct(key).first
      match ? " (did you mean #{match.inspect}?)" : ""
    end

    # numbers pass through, true and false become 1 and 0, nil keeps the default
    def number_option(key)
      value = @user_options[key]
      case value
      when nil then nil
      when true then 1
      when false then 0
      when Numeric then value
      else
        raise ArgumentError, "webp-ffi: option #{key.inspect} must be a number or true/false, got #{value.inspect}"
      end
    end

    def flag_option(key)
      value = @user_options[key]
      case value
      when nil, false, 0 then 0
      when true, 1 then 1
      else
        raise ArgumentError, "webp-ffi: option #{key.inspect} must be true or false, got #{value.inspect}"
      end
    end

    def output_format
      value = @user_options[:output_format]
      format = value.respond_to?(:to_sym) ? value.to_sym : value
      return format if OUTPUT_FORMATS.include?(format)
      raise ArgumentError, "webp-ffi: unknown output_format #{value.inspect}, use one of #{OUTPUT_FORMATS.map(&:inspect).join(', ')}"
    end

    # a Symbol or String from 'names', or -1 for the default
    def named_option(key, names)
      value = @user_options[key]
      return -1 if value.nil?
      name = value.respond_to?(:to_sym) ? value.to_sym : value
      names.fetch(name) do
        raise ArgumentError, "webp-ffi: unknown #{key} #{value.inspect}, use one of #{names.keys.map(&:inspect).join(', ')}"
      end
    end

    # lossless compression effort from 0 (fast) to 9 (small), or -1
    def level_option
      value = @user_options[:level]
      return -1 if value.nil?
      return value if value.is_a?(Integer) && value.between?(0, 9)
      raise ArgumentError, "webp-ffi: option :level must be an integer from 0 to 9, got #{value.inspect}"
    end

    # :all, :none, or one or more of :exif, :icc and :xmp
    def metadata_option
      value = @user_options[:metadata]
      case value
      when nil, false, :none, "none" then 0
      when true, :all, "all" then METADATA.values.sum
      else
        Array(value).sum do |name|
          METADATA.fetch(name.respond_to?(:to_sym) ? name.to_sym : name) do
            raise ArgumentError, "webp-ffi: unknown metadata #{name.inspect}, use :all, :none or some of :exif, :icc, :xmp"
          end
        end
      end
    end

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
