require 'spec_helper'
require 'pathname'

describe "WebP file arguments" do
  around do |example|
    Dir.mktmpdir("webp-ffi") do |dir|
      @dir = Pathname(dir)
      example.run
    end
  end

  it "encodes from and to a Pathname" do
    output = @dir.join("out.webp")
    expect(WebP.encode(Pathname(factory_path("1.png")), output)).to be true
    expect(WebP.webp_size(output.binread)).to eq([400, 301])
  end

  it "decodes from and to a Pathname" do
    output = @dir.join("out.png")
    expect(WebP.decode(Pathname(factory_path("1.webp")), output)).to be true
    expect(output.binread(8)).to eq("\x89PNG\r\n\x1A\n".b)
  end

  it "accepts any object with #to_path" do
    path_like = Struct.new(:to_path).new(factory_path("1.png"))
    expect(WebP.encode(path_like, @dir.join("out.webp"))).to be true
  end

  it "names a Pathname in error messages" do
    input = @dir.join("missing.png")
    expect { WebP.encode(input, @dir.join("out.webp")) }
      .to raise_error(WebP::EncoderError, "Cannot read input picture file: #{input}")
  end

  it "rejects nil instead of passing it to C", :outside_type_check do
    expect { WebP.encode(nil, @dir.join("out.webp")) }.to raise_error(TypeError)
    expect { WebP.decode(factory_path("1.webp"), nil) }.to raise_error(TypeError)
  end

  it "encodes a file onto itself" do
    path = @dir.join("same.png")
    path.binwrite(File.binread(factory_path("1.png")))
    expect(WebP.encode(path, path)).to be true
    expect(WebP.webp_size(path.binread)).to eq([400, 301])
  end

  it "leaves no output file when encoding fails" do
    output = @dir.join("failed.webp")
    expect { WebP.encode(factory_path("1.png"), output, crop_w: 30_000) }.to raise_error(WebP::EncoderError)
    expect(output).not_to exist
  end

  it "leaves an existing output file alone when encoding fails" do
    output = @dir.join("old.webp")
    output.binwrite("old contents")
    expect { WebP.encode(factory_path("1.png"), output, crop_w: 30_000) }.to raise_error(WebP::EncoderError)
    expect(output.binread).to eq("old contents")
  end

  it "leaves no output file when decoding fails" do
    output = @dir.join("failed.png")
    expect { WebP.decode(factory_path("1.webp"), output, crop_w: 30_000) }.to raise_error(WebP::DecoderError)
    expect(output).not_to exist
  end

  it "uses true for lossless encoding" do
    output = @dir.join("lossless.webp")
    WebP.encode(factory_path("1.png"), output, lossless: true)
    expect(output.binread(4, 12)).to eq("VP8L")
  end

  it "does not write a file for an unknown output format" do
    output = @dir.join("out.jpg")
    expect { WebP.decode(factory_path("1.webp"), output, output_format: :jpg) }.to raise_error(ArgumentError)
    expect(output).not_to exist
  end
end
