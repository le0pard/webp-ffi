require 'open3'
require 'rbconfig'
require 'shellwords'
require 'tmpdir'

module WebPFFI
  # Build settings shared by the gem install Rakefile (ext/webp_ffi/Rakefile)
  # and the development Rakefile in the project root.
  class BuildConfig
    class MissingDependencyError < StandardError; end

    # Homebrew uses /opt/homebrew on Apple Silicon and /usr/local on Intel Macs.
    # MacPorts uses /opt/local. Libraries anywhere else need C_FLAGS and LD_FLAGS.
    LIBRARY_PREFIXES = %w[/opt/homebrew /usr/local /opt/local].freeze

    # 'requirement' is an optional preprocessor condition, such as a version check
    Dependency = Struct.new(:name, :header, :library, :code, :requirement, keyword_init: true)

    DEPENDENCIES = [
      Dependency.new(name: "libwebp 1.0.0 or newer", header: "webp/encode.h", library: "webp", code: "WebPGetEncoderVersion();",
                     requirement: "WEBP_ENCODER_ABI_VERSION >= 0x020e"),
      Dependency.new(name: "libpng", header: "png.h", library: "png", code: "png_access_version_number();"),
      Dependency.new(name: "libjpeg", header: "jpeglib.h", library: "jpeg", code: "struct jpeg_error_mgr err; jpeg_std_error(&err);"),
      Dependency.new(name: "libtiff", header: "tiffio.h", library: "tiff", code: "TIFFGetVersion();"),
      Dependency.new(name: "zlib", header: "zlib.h", library: "z", code: "zlibVersion();")
    ].freeze

    INSTALL_HINT = <<~TEXT.freeze
      Install the development packages first, for example:
        macOS (Homebrew): brew install webp libpng jpeg-turbo libtiff
        macOS (MacPorts): sudo port install webp libpng jpeg libtiff
        Debian, Ubuntu:   sudo apt-get install libwebp-dev libpng-dev libjpeg-dev libtiff-dev
        Fedora:           sudo dnf install libwebp-devel libpng-devel libjpeg-devel libtiff-devel
      If they are installed somewhere else, point the build at them:
        C_FLAGS="-I/path/to/include" LD_FLAGS="-L/path/to/lib"
    TEXT

    attr_reader :cflags, :ldflags, :libs

    def initialize(env: ENV, prefixes: LIBRARY_PREFIXES, dependencies: DEPENDENCIES)
      @env = env
      @dependencies = dependencies
      # user flags go first, so they win over the default prefixes
      @cflags = split(env['C_FLAGS'])
      @ldflags = split(env['LD_FLAGS'])
      prefixes.each do |prefix|
        include_dir = File.join(prefix, 'include')
        lib_dir = File.join(prefix, 'lib')
        @cflags << "-I#{include_dir}" if File.directory?(include_dir)
        @ldflags << "-L#{lib_dir}" if File.directory?(lib_dir)
      end
      @libs = dependencies.map { |dependency| "-l#{dependency.library}" }
    end

    # Defines the compile task and a check that runs before it. Call it inside
    # a namespace to keep the task names apart.
    def self.define_tasks(library_name, env: ENV)
      require 'rake'
      require 'ffi-compiler/compile_task'
      config = new(env: env)

      Rake.application.last_description = "Check that libwebp and the image libraries can be found"
      Rake::Task.define_task(:check_dependencies) { config.check_dependencies! }
      # defined before CompileTask, so the check runs before compiling
      Rake::Task.define_task(default: :check_dependencies)

      FFI::Compiler::CompileTask.new(library_name) { |task| config.apply(task) }
    end

    def apply(task)
      @cflags.each { |flag| task.cflags << Shellwords.escape(flag) }
      @ldflags.each { |flag| task.ldflags << Shellwords.escape(flag) }
      task.libs.concat(@libs)
      task
    end

    def missing_dependencies
      @dependencies.reject { |dependency| links?(dependency) }
    end

    def check_dependencies!
      missing = missing_dependencies
      return true if missing.empty?

      names = missing.map { |dependency| "#{dependency.name} (#{dependency.header}, -l#{dependency.library})" }
      raise MissingDependencyError, "webp-ffi cannot find: #{names.join(', ')}.\n#{INSTALL_HINT}"
    end

    private

    # compiles and links a tiny program the same way the extension is built
    def links?(dependency)
      Dir.mktmpdir("webp-ffi-check") do |dir|
        source = File.join(dir, "check.c")
        requirement = dependency.requirement ? "#if !(#{dependency.requirement})\n#error too old\n#endif\n" : ""
        File.write(source, <<~C)
          #include <stdio.h>
          #include <#{dependency.header}>
          #{requirement}int main(void) { #{dependency.code} return 0; }
        C
        command = compiler + split(@env['CFLAGS']) + @cflags +
                  [source, "-o", File.join(dir, "check")] +
                  split(@env['LDFLAGS']) + @ldflags + ["-l#{dependency.library}"]
        _output, status = Open3.capture2e(*command)
        status.success?
      end
    rescue SystemCallError
      false
    end

    def compiler
      split(@env['CC'] || RbConfig::CONFIG['CC'] || 'cc')
    end

    def split(flags)
      flags ? Shellwords.split(flags) : []
    end
  end
end
