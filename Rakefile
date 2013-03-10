require 'bundler/setup'
require 'rake'
require 'rake/clean'
require 'bundler/gem_tasks'
require 'rspec/core/rake_task'
require 'ffi-compiler/compile_task'

desc "compiler tasks"
namespace "ffi-compiler" do
  FFI::Compiler::CompileTask.new('ext/webp_ffi/webp_ffi') do |c|
    c.have_header?('stdio.h', '/usr/local/include')
    c.have_func?('puts')
    c.have_library?('z')
    c.have_header?('decode.h', '/usr/local/include')
    c.have_header?('encode.h', '/usr/local/include')
    c.have_func?('WebPDecoderConfig')
    c.have_func?('WebPGetInfo')
    c.have_library?('webp')
    c.have_library?('png')
    c.have_library?('jpeg')
    c.have_library?('tiff')
    c.cflags << "-arch x86_64" if c.platform.mac?
    c.ldflags << "-arch x86_64" if c.platform.mac?
  end
end
task :compile => ["ffi-compiler:default"]

desc "run specs"
task :spec do
  RSpec::Core::RakeTask.new
end

task :default => [:clean, :compile, :spec]

CLEAN.include('ext/**/*{.o,.log,.so,.bundle}')
CLEAN.include('lib/**/*{.o,.log,.so,.bundle}')
CLEAN.include('ext/**/Makefile')