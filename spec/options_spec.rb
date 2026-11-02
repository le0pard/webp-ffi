require 'spec_helper'

describe WebP::Options do
  # FFI only zeroes memory when asked to, and malloc can hand back reused
  # memory. Fill every new pointer with junk, so a field that Options does not
  # write explicitly shows up in these specs.
  before do
    allow(FFI::MemoryPointer).to receive(:new).and_wrap_original do |original, *args|
      original.call(*args).tap { |pointer| pointer.put_bytes(0, "\xFF".b * pointer.size) }
    end
  end

  def fields(struct)
    struct.members.to_h { |member| [member, struct[member]] }
  end

  def decode_config(options)
    WebP::C::FfiWebpDecodeConfig.new(described_class.new(options).decode_pointer)
  end

  def encode_config(options)
    WebP::C::FfiWebpEncodeConfig.new(described_class.new(options).encode_pointer)
  end

  describe "#decode_pointer" do

    it "writes every field when no options are given" do
      expect(fields(decode_config({}))).to eq(
        output_format: :png, bypass_filtering: 0, no_fancy_upsampling: 0,
        crop_x: 0, crop_y: 0, crop_w: 0, crop_h: 0,
        resize_w: 0, resize_h: 0, use_threads: 0)
    end

    it "turns on the flags that are set to true" do
      config = decode_config(bypass_filtering: true, no_fancy_upsampling: true, use_threads: true)
      expect(fields(config)).to include(bypass_filtering: 1, no_fancy_upsampling: 1, use_threads: 1)
    end
  end

  describe "#encode_pointer" do
    it "writes every field when no options are given" do
      config = WebP::C::FfiWebpEncodeConfig.new(described_class.new({}).encode_pointer)
      zero_fields = [:crop_x, :crop_y, :crop_w, :crop_h, :resize_w, :resize_h]
      expected = config.members.to_h do |member|
        value = if member == :quality then 100
                elsif zero_fields.include?(member) then 0
                else -1
                end
        [member, value]
      end
      expect(fields(config)).to eq(expected)
    end
  end

  describe "unknown options" do
    it "warns about a typo and suggests the right name" do
      expect { encode_config(cropt_y: 0) }
        .to output(/ignoring unknown encode option :cropt_y \(did you mean :crop_y\?\)/).to_stderr
    end

    it "warns about encode options passed to decode" do
      expect { decode_config(quality: 50) }.to output(/ignoring unknown decode option :quality/).to_stderr
    end

    it "stays quiet for known options" do
      expect { encode_config(quality: 50, crop_x: 1, lossless: 1) }.not_to output.to_stderr
      expect { decode_config(output_format: :tiff, use_threads: true, resize_w: 10) }.not_to output.to_stderr
    end
  end

  describe "option values" do
    it "accepts true and false for integer encode options" do
      expect(fields(encode_config(lossless: true, autofilter: false))).to include(lossless: 1, autofilter: 0)
    end

    it "accepts floats" do
      expect(fields(encode_config(quality: 75.5, target_PSNR: 42.0))).to include(quality: 75.5, target_PSNR: 42.0)
    end

    it "rejects encode values that are not numbers" do
      expect { encode_config(quality: "high") }
        .to raise_error(ArgumentError, /option :quality must be a number or true\/false, got "high"/)
    end

    it "accepts 1 and 0 for decode flags" do
      expect(fields(decode_config(use_threads: 1, bypass_filtering: 0))).to include(use_threads: 1, bypass_filtering: 0)
    end

    it "rejects decode flags that are not true or false" do
      expect { decode_config(use_threads: "yes") }
        .to raise_error(ArgumentError, /option :use_threads must be true or false, got "yes"/)
    end

    it "accepts the output format as a string" do
      expect(decode_config(output_format: "tiff")[:output_format]).to eq(:tiff)
    end

    it "rejects unknown output formats instead of writing PNG" do
      expect { decode_config(output_format: :jpg) }
        .to raise_error(ArgumentError, /unknown output_format :jpg, use one of :png, :pam/)
    end
  end
end
