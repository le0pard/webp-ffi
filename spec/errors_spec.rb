require 'spec_helper'

describe "WebP errors" do
  around do |example|
    Dir.mktmpdir("webp-ffi") do |dir|
      @dir = dir
      example.run
    end
  end

  # libtiff before 4.5.0 can only print through process-wide handlers
  def self.libtiff_version
    tiff = Module.new do
      extend FFI::Library
      ffi_lib FFI::Compiler::Loader.find('webp_ffi')
      attach_function :TIFFGetVersion, [], :string
    end
    Gem::Version.new(tiff.TIFFGetVersion[/Version (\d+\.\d+\.\d+)/, 1])
  end

  describe ".encode" do
    it "names the input file that cannot be read" do
      input = File.join(@dir, "missing.png")
      expect { WebP.encode(input, File.join(@dir, "out.webp")) }
        .to raise_error(WebP::EncoderError, "Cannot read input picture file: #{input}")
    end

    it "names the output file that cannot be opened" do
      output = File.join(@dir, "missing", "out.webp")
      expect { WebP.encode(factory_path("1.png"), output) }
        .to raise_error(WebP::EncoderError, "Cannot open output file: #{output}")
    end

    it "reports error codes it does not know" do
      allow(WebP::C).to receive(:webp_encode).and_return(42)
      expect { WebP.encode(factory_path("1.png"), File.join(@dir, "out.webp")) }
        .to raise_error(WebP::EncoderError, "Unknown error (code 42)")
    end

    # a JPEG cut after its header still decodes, so cut it inside the header
    { "1.png" => 60_000, "3.jpg" => 200, "5.tif" => 200 }.each do |name, length|
      it "rejects a truncated #{File.extname(name)} file" do
        input = truncated_copy(name, length, @dir)
        expect { WebP.encode(input, File.join(@dir, "out.webp")) }
          .to raise_error(WebP::EncoderError, "Cannot read input picture file: #{input}")
      end
    end

    it "prints nothing to stderr for broken PNG and JPEG files" do
      expect {
        ["1.png", "3.jpg"].each do |name|
          input = truncated_copy(name, 200, @dir)
          expect { WebP.encode(input, File.join(@dir, "out.webp")) }.to raise_error(WebP::EncoderError)
        end
      }.not_to output.to_stderr_from_any_process
    end

    it "prints nothing to stderr for broken TIFF files", if: libtiff_version >= Gem::Version.new("4.5.0") do
      expect {
        input = truncated_copy("5.tif", 1_000_000, @dir)
        expect { WebP.encode(input, File.join(@dir, "out.webp")) }.to raise_error(WebP::EncoderError)
      }.not_to output.to_stderr_from_any_process
    end
  end

  describe ".decode" do
    it "names the input file that cannot be read" do
      input = File.join(@dir, "missing.webp")
      expect { WebP.decode(input, File.join(@dir, "out.png")) }
        .to raise_error(WebP::DecoderError, "Cannot read input file: #{input}")
    end

    it "fails when the output file cannot be written" do
      output = File.join(@dir, "missing", "out.png")
      expect { WebP.decode(factory_path("1.webp"), output) }
        .to raise_error(WebP::DecoderError, "Cannot write output file: #{output}")
      expect(File.exist?(output)).to be false
    end

    it "names the input file that is not a WebP image" do
      input = factory_path("1.png")
      expect { WebP.decode(input, File.join(@dir, "out.png")) }
        .to raise_error(WebP::DecoderError, "Invalid webp image: #{input}")
    end

    it "reports error codes it does not know" do
      allow(WebP::C).to receive(:webp_decode).and_return(-1)
      expect { WebP.decode(factory_path("1.webp"), File.join(@dir, "out.png")) }
        .to raise_error(WebP::DecoderError, "Unknown error (code -1)")
    end

    it "prints nothing to stderr when files cannot be read or written" do
      expect {
        expect { WebP.decode(File.join(@dir, "missing.webp"), File.join(@dir, "out.png")) }
          .to raise_error(WebP::DecoderError)
        expect { WebP.decode(factory_path("1.webp"), File.join(@dir, "missing", "out.png")) }
          .to raise_error(WebP::DecoderError)
      }.not_to output.to_stderr_from_any_process
    end
  end
end
