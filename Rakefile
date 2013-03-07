require "bundler/gem_tasks"

require 'ffi-compiler/export_task'

def gem_spec
  @gem_spec ||= Gem::Specification.load('webp_ffi.gemspec')
end

FFI::Compiler::ExportTask.new('lib', 'ext', :gem_spec => gem_spec) do |t|
  t.export 'webp_ffi.rb'
end
