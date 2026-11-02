require 'spec_helper'

describe "WebP metadata" do
  around do |example|
    Dir.mktmpdir("webp-ffi") do |dir|
      @dir = dir
      example.run
    end
  end

  let(:icc) { icc_profile("test profile") }
  let(:png_with_metadata) do
    png(2, 2, rows: ["\xFF\x00\x00\x00\xFF\x00".b] * 2, chunks: [png_iccp(icc), png_exif(exif), png_xmp(xmp)])
  end
  let(:jpg_with_metadata) do
    jpeg_with_segments(File.binread(factory_path("3.jpg")),
                       [jpeg_exif_segment(exif), jpeg_xmp_segment(xmp)] + jpeg_icc_segments(icc))
  end
  let(:tiff_with_metadata) { tiff(2, 2, pixels: "\xFF\x00\x00".b * 4, icc: icc, xmp: xmp) }

  def chunks(webp)
    webp_chunks(webp).to_h
  end

  it "copies nothing by default, like cwebp" do
    [png_with_metadata, jpg_with_metadata, tiff_with_metadata].each do |data|
      expect(chunks(WebP.encode_data(data)).keys).to eq(["VP8 "])
    end
  end

  it "copies ICC, EXIF and XMP from PNG" do
    result = chunks(WebP.encode_data(png_with_metadata, metadata: :all))
    expect(result.keys).to eq(["VP8X", "ICCP", "VP8 ", "EXIF", "XMP "])
    expect(result.values_at("ICCP", "EXIF", "XMP ")).to eq([icc, exif, xmp.b])
  end

  it "copies ICC, EXIF and XMP from JPEG" do
    result = chunks(WebP.encode_data(jpg_with_metadata, metadata: :all))
    expect(result.values_at("ICCP", "EXIF", "XMP ")).to eq([icc, exif, xmp.b])
  end

  it "copies ICC and XMP from TIFF, like cwebp (no EXIF)" do
    result = chunks(WebP.encode_data(tiff_with_metadata, metadata: :all))
    expect(result.keys).to eq(["VP8X", "ICCP", "VP8 ", "XMP "])
    expect(result.values_at("ICCP", "XMP ")).to eq([icc, xmp.b])
  end

  it "copies only the selected kinds" do
    expect(WebP.info(WebP.encode_data(png_with_metadata, metadata: :icc)).metadata).to eq([:icc])
    expect(WebP.info(WebP.encode_data(png_with_metadata, metadata: [:exif, "xmp"])).metadata).to eq([:exif, :xmp])
    expect(WebP.info(WebP.encode_data(png_with_metadata, metadata: "all")).metadata).to eq([:icc, :exif, :xmp])
    expect(chunks(WebP.encode_data(png_with_metadata, metadata: :none)).keys).to eq(["VP8 "])
  end

  it "keeps the alpha flag of lossless images" do
    data = png(2, 2, color_type: 6, rows: ["\xFF\x00\x00\x80\x00\xFF\x00\xFF".b] * 2, chunks: [png_iccp(icc)])
    info = WebP.info(WebP.encode_data(data, metadata: :icc, lossless: true))
    expect([info.alpha?, info.lossless?, info.metadata]).to eq([true, true, [:icc]])
  end

  it "adds ICC before the alpha chunk of lossy images with alpha" do
    data = png(2, 2, color_type: 6, rows: ["\xFF\x00\x00\x80\x00\xFF\x00\xFF".b] * 2, chunks: [png_iccp(icc)])
    expect(chunks(WebP.encode_data(data, metadata: :icc)).keys).to eq(["VP8X", "ICCP", "ALPH", "VP8 "])
  end

  it "pads odd-sized chunks" do
    odd = xmp("odd!")
    expect(odd.bytesize).to be_odd
    data = png(2, 2, rows: ["\xFF\x00\x00\x00\xFF\x00".b] * 2, chunks: [png_xmp(odd)])
    webp = WebP.encode_data(data, metadata: :xmp)
    expect(chunks(webp)["XMP "]).to eq(odd.b)
    expect(webp.bytesize).to eq(webp.byteslice(4, 4).unpack1("V") + 8)
    expect(webp.bytesize).to be_even
  end

  it "joins a JPEG ICC profile split over segments in any order" do
    data = jpeg_with_segments(File.binread(factory_path("3.jpg")), jpeg_icc_segments(icc, parts: 3, order: [2, 0, 1]))
    expect(chunks(WebP.encode_data(data, metadata: :icc))["ICCP"]).to eq(icc)
  end

  it "rejects a JPEG ICC profile with a missing segment, like cwebp" do
    data = jpeg_with_segments(File.binread(factory_path("3.jpg")), jpeg_icc_segments(icc, parts: 3).first(2))
    expect { WebP.encode_data(data, metadata: :icc) }.to raise_error(WebP::EncoderError, "Cannot read input picture data")
    # without metadata the profile is not read at all
    expect(WebP.webp_size(WebP.encode_data(data))).to eq([550, 368])
  end

  it "works with files" do
    input = File.join(@dir, "in.jpg")
    output = File.join(@dir, "out.webp")
    File.binwrite(input, jpg_with_metadata)
    WebP.encode(input, output, metadata: :all)
    expect(WebP.info(File.binread(output)).metadata).to eq([:icc, :exif, :xmp])
  end

  it "has nothing to copy from raw YUV" do
    yuv = ("\x80" * 12).b
    expect(WebP.info(WebP.encode_data(yuv, width: 4, height: 2, metadata: :all)).metadata).to eq([])
  end

  describe "ImageMagick and ExifTool raw profiles in PNG" do
    let(:pixels) { ["\xFF\x00\x00\x00\xFF\x00".b] * 2 }

    it "reads EXIF and XMP from text chunks" do
      data = png(2, 2, rows: pixels, chunks: [png_text("Raw profile type exif", raw_profile("exif", exif)),
                                              png_text("Raw profile type xmp", raw_profile("xmp", xmp))])
      expect(chunks(WebP.encode_data(data, metadata: :all)).values_at("EXIF", "XMP ")).to eq([exif, xmp.b])
    end

    it "reads compressed text chunks" do
      data = png(2, 2, rows: pixels, chunks: [png_text("Raw profile type exif", raw_profile("exif", exif), compressed: true)])
      expect(chunks(WebP.encode_data(data, metadata: :exif))["EXIF"]).to eq(exif)
    end

    it "reads EXIF from an APP1 profile" do
      data = png(2, 2, rows: pixels, chunks: [png_text("Raw profile type APP1", raw_profile("app1", exif))])
      expect(chunks(WebP.encode_data(data, metadata: :exif))["EXIF"]).to eq(exif)
    end

    it "prefers an eXIf chunk, like libwebp" do
      other = exif + "\0\0".b
      data = png(2, 2, rows: pixels, chunks: [png_exif(exif), png_text("Raw profile type exif", raw_profile("exif", other))])
      expect(chunks(WebP.encode_data(data, metadata: :exif))["EXIF"]).to eq(exif)
    end

    it "rejects a broken profile when metadata is requested, like cwebp" do
      data = png(2, 2, rows: pixels, chunks: [png_text("Raw profile type exif", "\nexif\n      99\nnot hex\n")])
      expect { WebP.encode_data(data, metadata: :exif) }.to raise_error(WebP::EncoderError, "Cannot read input picture data")
      expect(WebP.webp_size(WebP.encode_data(data))).to eq([2, 2])
    end
  end

  it "rejects unknown kinds" do
    expect { WebP.encode_data(png_with_metadata, metadata: [:icc, :gps]) }
      .to raise_error(ArgumentError, "webp-ffi: unknown metadata :gps, use :all, :none or some of :exif, :icc, :xmp")
  end
end
