require 'rubygems'
require 'bundler/setup'
require 'rake'
require 'rake/clean'
require 'bundler/gem_tasks'
require 'rspec/core/rake_task'
require_relative 'ext/webp_ffi/build_config'

desc "compiler tasks"
namespace "ffi-compiler" do
  # the same build that gem install runs from ext/webp_ffi/Rakefile
  WebPFFI::BuildConfig.define_tasks('ext/webp_ffi/webp_ffi')
end
task :compile => ["ffi-compiler:default"]

desc "run specs"
task :spec do
  RSpec::Core::RakeTask.new
end

desc "run specs with RBS runtime type checks against sig/"
task "spec:types" do
  env = { "RBS_TEST_TARGET" => "WebP::*,WebP", "RBS_TEST_OPT" => "-I sig", "RBS_TEST_LOGLEVEL" => "error" }
  sh env, "ruby -r rbs/test/setup -S rspec"
end

task :default => [:clean, :compile, :spec]

CLEAN.include('ext/**/*{.o,.log,.so,.bundle}')
CLEAN.include('lib/**/*{.o,.log,.so,.bundle}')
CLEAN.include('ext/**/Makefile')
