require 'spec_helper'
require 'stringio'

describe "WebP.info" do
  let(:png_file) { File.binread(factory_path("1.png")) }
  let(:jpg) { File.binread(factory_path("3.jpg")) }

  it "describes a lossy image" do
    expect(WebP.info(WebP.encode_data(jpg))).to eq(
      WebP::Info.new(width: 550, height: 368, format: :lossy, alpha: false, animated: false, metadata: []))
  end

  it "describes a lossy image with alpha" do
    info = WebP.info(WebP.encode_data(png_file))
    expect([info.format, info.alpha?, info.lossless?]).to eq([:lossy, true, false])
  end

  it "describes a lossless image" do
    info = WebP.info(File.binread(factory_path("1.webp")))
    expect([info.width, info.height, info.format, info.lossless?, info.alpha?]).to eq([400, 301, :lossless, true, true])
  end

  it "describes an animation" do
    still = WebP.encode_data(png_file, lossless: true, resize_w: 20, resize_h: 15)
    info = WebP.info(animated_webp(still))
    expect([info.width, info.height, info.animated?, info.format]).to eq([20, 15, true, :mixed])
  end

  it "lists the metadata in the file" do
    data = png(2, 2, rows: ["\xFF\x00\x00\x00\xFF\x00".b] * 2, chunks: [png_iccp(icc_profile), png_xmp(xmp)])
    expect(WebP.info(WebP.encode_data(data, metadata: :all)).metadata).to eq([:icc, :xmp])
  end

  it "reads from an IO" do
    File.open(factory_path("1.webp"), "rb") { |file| expect(WebP.info(file).width).to eq(400) }
    expect(WebP.info(StringIO.new(File.binread(factory_path("1.webp")))).height).to eq(301)
  end

  it "returns nil for nil" do
    expect(WebP.info(nil)).to be_nil
  end

  it "raises for data that is not WebP" do
    expect { WebP.info(png_file) }.to raise_error(
      an_instance_of(WebP::InvalidImageFormatError).and(having_attributes(
        message: "invalid WebP image data: the data is not a valid WebP bitstream", reason: :bitstream_error)))
  end

  it "raises for truncated data" do
    expect { WebP.info(File.binread(factory_path("1.webp"), 20)) }
      .to raise_error(WebP::InvalidImageFormatError, /not enough data/)
  end
end
