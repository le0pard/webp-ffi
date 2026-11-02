require 'spec_helper'
require 'fileutils'
require 'ffi-compiler/compile_task'
require_relative '../ext/webp_ffi/build_config'

describe WebPFFI::BuildConfig, :source_tree do
  around do |example|
    Dir.mktmpdir("webp-ffi-build") do |dir|
      @dir = dir
      example.run
    end
  end

  # a fake install prefix with include and lib folders
  def prefix(name, include: true, lib: true)
    path = File.join(@dir, name)
    FileUtils.mkdir_p(File.join(path, "include")) if include
    FileUtils.mkdir_p(File.join(path, "lib")) if lib
    path
  end

  def dependency(**fields)
    described_class::Dependency.new(**fields)
  end

  def fake_task
    Struct.new(:cflags, :ldflags, :libs)
          .new(FFI::Compiler::Flags.new([]), FFI::Compiler::Flags.new([]), [])
  end

  describe "library folders" do
    it "looks in the Homebrew and MacPorts prefixes by default" do
      expect(described_class::LIBRARY_PREFIXES).to eq(%w[/opt/homebrew /usr/local /opt/local])
    end

    it "adds the include and lib folders that exist" do
      brew = prefix("brew")
      ports = prefix("ports", lib: false)
      config = described_class.new(env: {}, prefixes: [brew, File.join(@dir, "missing"), ports])
      expect(config.cflags).to eq(["-I#{brew}/include", "-I#{ports}/include"])
      expect(config.ldflags).to eq(["-L#{brew}/lib"])
    end

    it "puts C_FLAGS and LD_FLAGS before the default folders" do
      brew = prefix("brew")
      env = { "C_FLAGS" => "-I/custom/include -DCUSTOM", "LD_FLAGS" => "-L/custom/lib" }
      config = described_class.new(env: env, prefixes: [brew])
      expect(config.cflags).to eq(["-I/custom/include", "-DCUSTOM", "-I#{brew}/include"])
      expect(config.ldflags).to eq(["-L/custom/lib", "-L#{brew}/lib"])
    end

    it "does not run any command to find them" do
      expect(Open3).not_to receive(:capture2e)
      described_class.new(env: {}, prefixes: [prefix("brew")])
    end
  end

  describe "#apply" do
    it "passes the flags to ffi-compiler in the same order" do
      brew = prefix("brew")
      task = described_class.new(env: { "LD_FLAGS" => "-L/custom/lib" }, prefixes: [brew]).apply(fake_task)
      expect(task.cflags.to_a).to eq(["-I#{brew}/include"])
      expect(task.ldflags.to_a).to eq(["-L/custom/lib", "-L#{brew}/lib"])
      expect(task.libs).to eq(%w[-lwebp -lpng -ljpeg -ltiff -lz])
    end

    it "keeps a folder with spaces as one flag" do
      spaced = prefix("with space", lib: false)
      task = described_class.new(env: {}, prefixes: [spaced]).apply(fake_task)
      expect(task.cflags.to_a).to eq(["-I#{spaced}/include"])
    end
  end

  describe "#check_dependencies!" do
    it "finds the libraries this machine builds with" do
      expect(described_class.new.check_dependencies!).to be true
    end

    it "names the missing libraries and explains how to fix the build" do
      missing = dependency(name: "libmissing", header: "webp_ffi_missing.h", library: "webpffimissing", code: "")
      config = described_class.new(env: {}, prefixes: [], dependencies: [missing])
      expect { config.check_dependencies! }.to raise_error(
        described_class::MissingDependencyError,
        /cannot find: libmissing \(webp_ffi_missing\.h, -lwebpffimissing\).*brew install.*C_FLAGS=/m)
    end

    it "checks version requirements" do
      newer = dependency(name: "c", header: "limits.h", library: "m", code: "", requirement: "INT_MAX > 0")
      older = dependency(name: "c 99", header: "limits.h", library: "m", code: "", requirement: "INT_MAX < 0")
      config = described_class.new(env: {}, prefixes: [], dependencies: [newer, older])
      expect(config.missing_dependencies).to eq([older])
    end

    it "asks for libwebp 1.0.0 or newer" do
      expect(described_class::DEPENDENCIES.first.name).to eq("libwebp 1.0.0 or newer")
    end

    it "treats a missing compiler as missing libraries" do
      config = described_class.new(env: { "CC" => "/nonexistent/cc" }, prefixes: [])
      expect(config.missing_dependencies.map(&:name)).to include("libwebp 1.0.0 or newer")
    end

    it "uses C_FLAGS to find headers" do
      FileUtils.mkdir_p(File.join(@dir, "inc"))
      File.write(File.join(@dir, "inc", "webp_ffi_check.h"), "static int webp_ffi_check(void) { return 0; }\n")
      check = dependency(name: "check", header: "webp_ffi_check.h", library: "m", code: "webp_ffi_check();")
      without_flags = described_class.new(env: {}, prefixes: [], dependencies: [check])
      with_flags = described_class.new(env: { "C_FLAGS" => "-I#{@dir}/inc" }, prefixes: [], dependencies: [check])
      expect(without_flags.missing_dependencies).to eq([check])
      expect(with_flags.missing_dependencies).to be_empty
    end

    it "uses LD_FLAGS to find libraries" do
      lib_dir = File.join(@dir, "lib")
      FileUtils.mkdir_p(lib_dir)
      File.write(File.join(@dir, "fake.c"), "int webp_ffi_fake(void) { return 0; }\n")
      File.write(File.join(@dir, "fake.h"), "int webp_ffi_fake(void);\n")
      cc = Shellwords.split(RbConfig::CONFIG['CC'])
      system(*cc, "-c", File.join(@dir, "fake.c"), "-o", File.join(@dir, "fake.o"), exception: true)
      system("ar", "rcs", File.join(lib_dir, "libwebpffifake.a"), File.join(@dir, "fake.o"), exception: true)
      fake = dependency(name: "fake", header: "fake.h", library: "webpffifake", code: "webp_ffi_fake();")
      env = { "C_FLAGS" => "-I#{@dir}" }
      without_flags = described_class.new(env: env, prefixes: [], dependencies: [fake])
      with_flags = described_class.new(env: env.merge("LD_FLAGS" => "-L#{lib_dir}"), prefixes: [], dependencies: [fake])
      expect(without_flags.missing_dependencies).to eq([fake])
      expect(with_flags.missing_dependencies).to be_empty
    end
  end

  describe ".define_tasks" do
    around do |example|
      original = Rake.application
      record_metadata = Rake::TaskManager.record_task_metadata
      Rake.application = Rake::Application.new
      # rake only keeps task descriptions while it is listing tasks
      Rake::TaskManager.record_task_metadata = true
      begin
        example.run
      ensure
        Rake.application = original
        Rake::TaskManager.record_task_metadata = record_metadata
      end
    end

    it "checks the dependencies before compiling" do
      described_class.define_tasks(File.join(@dir, "webp_ffi"), env: {})
      prerequisites = Rake::Task[:default].prerequisites
      expect(prerequisites.size).to eq(2)
      expect(prerequisites.first).to eq("check_dependencies")
      expect(Rake::Task[:check_dependencies].comment).to match(/libwebp/)
    end
  end
end
