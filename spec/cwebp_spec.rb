require 'spec_helper'
require 'digest'
require 'open3'

# The gem should produce exactly what libwebp's own cwebp tool produces. Runs
# when a cwebp of the same libwebp version is on the PATH (Homebrew installs
# one, and CI builds one with libwebp).
describe "WebP compared with cwebp" do
  def self.cwebp_version
    output, status = Open3.capture2e("cwebp", "-version")
    status.success? ? output[/\A\d+\.\d+\.\d+/] : nil
  rescue SystemCallError
    nil
  end

  around do |example|
    Dir.mktmpdir("webp-ffi") do |dir|
      @dir = dir
      example.run
    end
  end

  def cwebp(input_data, *flags)
    input = File.join(@dir, "input")
    output = File.join(@dir, "output.webp")
    File.binwrite(input, input_data)
    # without -quiet, so a failure shows cwebp's own reason
    out, status = Open3.capture2e("cwebp", *flags, input, "-o", output)
    unless status.success?
      # some builds lack a format, for example the cwebp on GitHub's macOS
      # runners has no TIFF support
      missing = out[/^(\w+) support not compiled/, 1]
      skip("this cwebp was built without #{missing} support") if missing
      raise "cwebp #{flags.join(' ')} failed:\n#{out}"
    end
    File.binread(output)
  end

  # compares sizes and digests, so a failure does not print binary data
  def summary(webp)
    "#{webp.bytesize} bytes, sha256 #{Digest::SHA256.hexdigest(webp)[0, 16]}"
  end

  context "with the same libwebp version", if: cwebp_version == WebP.encoder_version do
    let(:jpg) { File.binread(factory_path("3.jpg")) }

    # the gem's default quality is 100; cwebp's is 75
    {
      %w[-q 100] => {}, %w[-q 100 -size 20000] => { target_size: 20_000 },
      %w[-q 100 -psnr 30] => { target_PSNR: 30 }, %w[-q 100 -sharp_yuv] => { use_sharp_yuv: true },
      %w[-q 100 -lossless] => { lossless: true }, %w[-q 75 -m 6 -segments 2] => { quality: 75, method: 6, segments: 2 },
      %w[-q 100 -crop 10 10 200 100] => { crop_x: 10, crop_y: 10, crop_w: 200, crop_h: 100 },
      %w[-q 100 -resize 100 80] => { resize_w: 100, resize_h: 80 },
      %w[-preset photo -q 100] => { preset: :photo },
      # any -q turns off cwebp's -z, so the level picks its own quality
      %w[-z 6] => { level: 6 }
    }.each do |flags, options|
      it "matches cwebp #{flags.join(' ')} for JPEG input" do
        expect(summary(WebP.encode_data(jpg, options))).to eq(summary(cwebp(jpg, *flags)))
      end
    end

    {
      # every cwebp reads raw profiles; only cwebp 1.6.0 and newer read eXIf chunks
      "PNG" => -> { png(2, 2, rows: ["\xFF\x00\x00\x00\xFF\x00".b] * 2, chunks: [png_iccp(icc_profile("png")), png_text("Raw profile type exif", raw_profile("exif", exif)), png_xmp(xmp)]) },
      "PNG with alpha" => -> { png(2, 2, color_type: 6, rows: ["\xFF\x00\x00\x80\x00\xFF\x00\xFF".b] * 2, chunks: [png_iccp(icc_profile("alpha")), png_xmp(xmp)]) },
      "JPEG" => -> { jpeg_with_segments(File.binread(factory_path("3.jpg")), [jpeg_exif_segment(exif), jpeg_xmp_segment(xmp)] + jpeg_icc_segments(icc_profile("jpeg"), parts: 3)) },
      "TIFF" => -> { tiff(2, 2, pixels: "\xFF\x00\x00".b * 4, icc: icc_profile("tiff"), xmp: xmp) }
    }.each do |name, input|
      it "matches cwebp -metadata all for #{name} input" do
        data = instance_exec(&input)
        expect(summary(WebP.encode_data(data, metadata: :all)))
          .to eq(summary(cwebp(data, "-q", "100", "-metadata", "all")))
        expect(summary(WebP.encode_data(data, metadata: :all, lossless: true)))
          .to eq(summary(cwebp(data, "-q", "100", "-metadata", "all", "-lossless")))
      end
    end

    it "matches cwebp -metadata all for a PNG eXIf chunk", if: Gem::Version.new(cwebp_version.to_s) >= Gem::Version.new("1.6.0") do
      data = png(2, 2, rows: ["\xFF\x00\x00\x00\xFF\x00".b] * 2, chunks: [png_exif(exif)])
      expect(summary(WebP.encode_data(data, metadata: :all))).to eq(summary(cwebp(data, "-q", "100", "-metadata", "all")))
    end
  end
end
