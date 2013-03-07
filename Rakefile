require "bundler/gem_tasks"
require 'ffi-compiler/compile_task'

require 'rspec/core'
require 'rspec/core/rake_task'

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
    c.cflags << "-arch x86_64" if c.platform.mac?
    c.ldflags << "-arch x86_64" if c.platform.mac?
  end
end
task :compile => ["ffi-compiler:default"]

RSpec::Core::RakeTask.new(:spec) do |spec|
  spec.pattern = FileList['spec/**/*_spec.rb']
end

task :default => [:clean, :compile, :spec, :clean]