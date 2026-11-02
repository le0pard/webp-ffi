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

  describe "#decode_pointer" do
    def decode_config(options)
      WebP::C::FfiWebpDecodeConfig.new(described_class.new(options).decode_pointer)
    end

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
end
