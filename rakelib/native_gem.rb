require 'fileutils'
require 'rubygems/package'
require 'tmpdir'
require_relative '../lib/webp/version'

module WebPFFI
  # Packages a prebuilt platform gem from the output of script/build-native.sh.
  # The platform gem has the library inside, so gem install compiles nothing.
  class NativeGem
    class Error < StandardError; end

    # Gem platform => folder FFI::Compiler::Loader searches at runtime. The
    # folder is "#{FFI::Platform::ARCH}-#{FFI::Platform::OS}" on that platform.
    PLATFORMS = {
      "x86_64-linux-gnu" => "x86_64-linux",
      "aarch64-linux-gnu" => "aarch64-linux",
      "x86_64-linux-musl" => "x86_64-linux",
      "aarch64-linux-musl" => "aarch64-linux",
      "x86_64-darwin" => "x86_64-darwin",
      "arm64-darwin" => "aarch64-darwin"
    }.freeze

    ELF_MACHINES = { 0x3E => "x86_64", 0xB7 => "aarch64" }.freeze
    MACHO_CPU_TYPES = { 0x01000007 => "x86_64", 0x0100000C => "aarch64" }.freeze

    attr_reader :platform

    def self.check_tag!(tag, version = WebP::VERSION)
      return true if tag == "v#{version}"
      raise Error, "tag #{tag.inspect} does not match the gem version #{version}, expected \"v#{version}\""
    end

    # [:elf or :macho, "x86_64" or "aarch64"] from the header of a 64-bit
    # little-endian library, or nil when the file is something else
    def self.binary_format(path)
      header = File.binread(path, 20).to_s
      return nil if header.bytesize < 20
      if header.start_with?("\x7FELF".b)
        [:elf, ELF_MACHINES[header.byteslice(18, 2).unpack1("v")]]
      elsif header.start_with?("\xCF\xFA\xED\xFE".b)
        [:macho, MACHO_CPU_TYPES[header.byteslice(4, 4).unpack1("V")]]
      end
    end

    def initialize(platform, root: File.expand_path("..", __dir__), native_dir: nil)
      unless PLATFORMS.key?(platform)
        raise Error, "unknown platform #{platform.inspect}, use one of #{PLATFORMS.keys.join(', ')}"
      end
      @platform = platform
      @root = root
      @native_dir = native_dir || File.join(root, "tmp", "native", "out")
    end

    def folder
      "ext/webp_ffi/#{PLATFORMS.fetch(platform)}"
    end

    def library_name
      darwin? ? "libwebp_ffi.bundle" : "libwebp_ffi.so"
    end

    # build output path => path inside the gem
    def native_files
      files = {
        File.join(@native_dir, library_name) => "#{folder}/#{library_name}",
        File.join(@native_dir, "versions.txt") => "#{folder}/versions.txt"
      }
      Dir[File.join(@native_dir, "licenses", "*")].sort.each do |license|
        files[license] = "#{folder}/licenses/#{File.basename(license)}"
      end
      files
    end

    def spec
      # dup copies the file and dependency lists, so the cached source spec stays as it is
      spec = Gem::Specification.load(File.join(@root, "webp-ffi.gemspec")).dup
      spec.platform = Gem::Platform.new(platform)
      spec.extensions = []
      # nothing is compiled at install time, so rake is not needed there
      spec.dependencies.delete_if { |dependency| dependency.name == "rake" }
      spec.files = spec.files + native_files.values
      spec
    end

    def validate!
      library = File.join(@native_dir, library_name)
      unless File.file?(library) && File.file?(File.join(@native_dir, "versions.txt"))
        raise Error, "#{library} or versions.txt not found, run script/build-native.sh first"
      end
      if Dir[File.join(@native_dir, "licenses", "*")].empty?
        raise Error, "no license files in #{File.join(@native_dir, 'licenses')}"
      end
      expected = [darwin? ? :macho : :elf, PLATFORMS.fetch(platform).split("-").first]
      actual = self.class.binary_format(library)
      return true if actual == expected
      raise Error, "#{library} is built for #{actual ? actual.join(' ') : 'an unknown format'}, not #{platform}"
    end

    # writes pkg/webp-ffi-VERSION-PLATFORM.gem and returns its path
    def build(out_dir = File.join(@root, "pkg"))
      validate!
      gemspec = spec
      sources = native_files.invert
      out_dir = File.expand_path(out_dir)
      FileUtils.mkdir_p(out_dir)
      Dir.mktmpdir("webp-ffi-gem") do |stage|
        gemspec.files.each do |path|
          target = File.join(stage, path)
          FileUtils.mkdir_p(File.dirname(target))
          FileUtils.cp(sources.fetch(path) { File.join(@root, path) }, target)
        end
        file_name = Dir.chdir(stage) { Gem::Package.build(gemspec) }
        FileUtils.mv(File.join(stage, file_name), out_dir)
        File.join(out_dir, file_name)
      end
    end

    private

    def darwin?
      platform.end_with?("darwin")
    end
  end
end
