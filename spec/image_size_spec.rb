require 'spec_helper'

# The PNG, JPEG and TIFF readers size their pixel buffers with this check.
describe "WebP::C.check_image_size" do
  def check(width, height, bytes_per_px)
    stride = FFI::MemoryPointer.new(:int)
    size = FFI::MemoryPointer.new(:size_t)
    return nil unless 1 == WebP::C.check_image_size(width, height, bytes_per_px, stride, size)
    { stride: stride.read_int, size: size.read(:size_t) }
  end

  it "returns the row stride and buffer size" do
    expect(check(400, 301, 4)).to eq(stride: 1_600, size: 481_600)
    expect(check(550, 368, 3)).to eq(stride: 1_650, size: 607_200)
  end

  it "computes buffer sizes above 4 GB without wrapping around", if: FFI::Platform::ADDRESS_SIZE == 64 do
    expect(check(50_000, 30_000, 4)).to eq(stride: 200_000, size: 6_000_000_000)
  end

  it "rejects empty images" do
    expect(check(0, 10, 4)).to be_nil
    expect(check(10, 0, 4)).to be_nil
  end

  it "rejects invalid bytes per pixel" do
    expect(check(10, 10, 0)).to be_nil
    expect(check(10, 10, 9)).to be_nil
  end

  it "rejects dimensions that do not fit in an int" do
    expect(check(2**31, 1, 1)).to be_nil
    expect(check(1, 2**31, 1)).to be_nil
  end

  it "rejects rows longer than an int" do
    expect(check(600_000_000, 1, 4)).to be_nil
  end

  it "rejects buffers above the memory limit" do
    expect(check(100_000, 100_000, 4)).to be_nil
  end
end
