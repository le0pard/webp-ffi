require 'spec_helper'
require 'fileutils'
require_relative '../rakelib/native_gem'

describe WebPFFI::NativeGem, :source_tree do
  around do |example|
    Dir.mktmpdir("webp-ffi-native") do |dir|
      @dir = dir
      example.run
    end
  end

  def elf(machine)
    "\x7FELF\x02\x01\x01".b + ("\0" * 9) + [3, machine].pack("vv")
  end

  def macho(cpu_type)
    "\xCF\xFA\xED\xFE".b + [cpu_type].pack("V") + ("\0" * 12)
  end

  # fake output of script/build-native.sh
  def native_build(library_name, header)
    File.binwrite(File.join(@dir, library_name), header + ("\0" * 64))
    File.write(File.join(@dir, "versions.txt"), "libwebp 1.6.0\n")
    FileUtils.mkdir_p(File.join(@dir, "licenses"))
    File.write(File.join(@dir, "licenses", "libwebp-COPYING"), "license text")
  end

  def native_gem(platform)
    described_class.new(platform, native_dir: @dir)
  end

  def quietly(&block)
    Gem::DefaultUserInteraction.use_ui(Gem::SilentUI.new, &block)
  end

  it "maps each platform to the folder the runtime loader searches" do
    expect(described_class::PLATFORMS).to eq(
      "x86_64-linux-gnu" => "x86_64-linux", "aarch64-linux-gnu" => "aarch64-linux",
      "x86_64-linux-musl" => "x86_64-linux", "aarch64-linux-musl" => "aarch64-linux",
      "x86_64-darwin" => "x86_64-darwin", "arm64-darwin" => "aarch64-darwin")
  end

  it "rejects platforms it cannot build" do
    expect { described_class.new("x64-mingw-ucrt") }.to raise_error(described_class::Error, /unknown platform "x64-mingw-ucrt"/)
  end

  describe "#spec" do
    before { native_build("libwebp_ffi.so", elf(0x3E)) }
    let(:spec) { native_gem("x86_64-linux-gnu").spec }

    it "targets the platform" do
      expect(spec.platform.to_s).to eq("x86_64-linux-gnu")
    end

    it "compiles nothing at install time" do
      expect(spec.extensions).to be_empty
      expect(spec.runtime_dependencies.map(&:name)).to contain_exactly("ffi", "ffi-compiler")
    end

    it "adds the library, its versions and licenses next to the source files" do
      expect(spec.files).to include(
        "lib/webp_ffi.rb",
        "ext/webp_ffi/x86_64-linux/libwebp_ffi.so",
        "ext/webp_ffi/x86_64-linux/versions.txt",
        "ext/webp_ffi/x86_64-linux/licenses/libwebp-COPYING")
    end

    it "leaves the source gemspec unchanged" do
      spec
      source = Gem::Specification.load(File.expand_path("../webp-ffi.gemspec", __dir__))
      expect(source.platform.to_s).to eq("ruby")
      expect(source.extensions).to eq(["ext/webp_ffi/Rakefile"])
      expect(source.runtime_dependencies.map(&:name)).to include("rake")
    end
  end

  describe "#build" do
    it "packages the library where FFI::Compiler::Loader finds it" do
      native_build("libwebp_ffi.so", elf(0x3E))
      path = quietly { native_gem("x86_64-linux-gnu").build(File.join(@dir, "pkg")) }
      expect(File.basename(path)).to eq("webp-ffi-#{WebP::VERSION}-x86_64-linux-gnu.gem")
      package = Gem::Package.new(path)
      expect(package.spec.platform.to_s).to eq("x86_64-linux-gnu")
      expect(package.contents).to include("lib/webp_ffi.rb", "ext/webp_ffi/x86_64-linux/libwebp_ffi.so",
                                          "ext/webp_ffi/x86_64-linux/licenses/libwebp-COPYING")
    end

    it "uses the folder of the Apple Silicon loader for arm64-darwin" do
      native_build("libwebp_ffi.bundle", macho(0x0100000C))
      path = quietly { native_gem("arm64-darwin").build(File.join(@dir, "pkg")) }
      expect(Gem::Package.new(path).contents).to include("ext/webp_ffi/aarch64-darwin/libwebp_ffi.bundle")
    end

    it "refuses a library built for another CPU" do
      native_build("libwebp_ffi.so", elf(0xB7))
      expect { native_gem("x86_64-linux-gnu").build(File.join(@dir, "pkg")) }
        .to raise_error(described_class::Error, /built for elf aarch64, not x86_64-linux-gnu/)
    end

    it "refuses a Linux library for macOS" do
      native_build("libwebp_ffi.bundle", elf(0x3E))
      expect { native_gem("x86_64-darwin").build(File.join(@dir, "pkg")) }
        .to raise_error(described_class::Error, /built for elf x86_64, not x86_64-darwin/)
    end

    it "refuses a file that is not a library" do
      native_build("libwebp_ffi.so", "not a library at all")
      expect { native_gem("x86_64-linux-gnu").build(File.join(@dir, "pkg")) }
        .to raise_error(described_class::Error, /built for an unknown format, not x86_64-linux-gnu/)
    end

    it "refuses to package without a native build" do
      expect { native_gem("x86_64-linux-gnu").build(File.join(@dir, "pkg")) }
        .to raise_error(described_class::Error, /run script\/build-native.sh first/)
    end

    it "refuses to package without license files" do
      native_build("libwebp_ffi.so", elf(0x3E))
      FileUtils.rm_rf(File.join(@dir, "licenses"))
      expect { native_gem("x86_64-linux-gnu").build(File.join(@dir, "pkg")) }
        .to raise_error(described_class::Error, /no license files/)
    end
  end

  describe ".binary_format" do
    def format_of(bytes)
      path = File.join(@dir, "library")
      File.binwrite(path, bytes)
      described_class.binary_format(path)
    end

    it "reads ELF and Mach-O headers" do
      expect(format_of(elf(0x3E))).to eq([:elf, "x86_64"])
      expect(format_of(elf(0xB7))).to eq([:elf, "aarch64"])
      expect(format_of(macho(0x01000007))).to eq([:macho, "x86_64"])
      expect(format_of(macho(0x0100000C))).to eq([:macho, "aarch64"])
    end

    it "returns nil for other files" do
      expect(format_of("not a library at all")).to be_nil
      expect(format_of("short")).to be_nil
    end
  end

  describe ".check_tag!" do
    it "accepts v followed by the gem version" do
      expect(described_class.check_tag!("v1.2.3", "1.2.3")).to be true
    end

    it "rejects a tag for another version" do
      expect { described_class.check_tag!("v1.2.4", "1.2.3") }
        .to raise_error(described_class::Error, /"v1.2.4" does not match the gem version 1.2.3/)
    end
  end
end

# Runs only against an installed gem: WEBP_FFI_INSTALLED_GEM=<platform> rspec
describe "installed webp-ffi gem", if: ENV['WEBP_FFI_INSTALLED_GEM'] do
  let(:gem_spec) { Gem.loaded_specs.fetch("webp-ffi") }
  let(:library) { File.realpath(WebP::C.ffi_libraries.first.name) }

  it "loads the library from the installed gem" do
    expect(library).to start_with(File.realpath(gem_spec.full_gem_path) + File::SEPARATOR)
  end

  it "is the gem for this platform" do
    expect(gem_spec.platform.to_s).to eq(ENV['WEBP_FFI_INSTALLED_GEM'])
  end

  # a nested if: replaces the outer one, so check both conditions here
  context "when prebuilt", if: ![nil, "ruby"].include?(ENV['WEBP_FFI_INSTALLED_GEM']) do
    # basenames of the shared libraries the extension needs at runtime
    def dynamic_dependencies(path)
      output = RUBY_PLATFORM.include?("darwin") ? `otool -L #{path}`.lines.drop(1) : `ldd #{path}`.lines
      output.map { |line| File.basename(line.strip.split(/\s+/).first.to_s) }.reject(&:empty?)
    end

    it "does not need shared image libraries" do
      dependencies = dynamic_dependencies(library)
      expect(dependencies).not_to be_empty
      # matches names like libpng16.16.dylib, libz.so.1 and libwebp.so.7
      expect(dependencies.grep(/\Alib(webp|sharpyuv|png|jpeg|turbojpeg|tiff|z)(\d|\.)/)).to be_empty
    end

    it "ships the versions and licenses of the bundled libraries" do
      folder = File.dirname(library)
      expect(File.read(File.join(folder, "versions.txt"))).to include("libwebp #{WebP.decoder_version}\n")
      licenses = Dir[File.join(folder, "licenses", "*")].map { |path| File.basename(path) }
      expect(licenses).to include("libwebp-COPYING", "libwebp-PATENTS", "libpng-LICENSE",
                                  "libjpeg-turbo-LICENSE.md", "libjpeg-turbo-README.ijg",
                                  "libtiff-LICENSE.md", "zlib-LICENSE")
    end
  end
end
