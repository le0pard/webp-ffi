# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'webp/version'

Gem::Specification.new do |spec|
  spec.name          = "webp-ffi"
  spec.version       = WebP::VERSION
  spec.authors       = ["Oleksii Vasyliev"]
  spec.email         = ["leopard.not.a@gmail.com"]
  spec.description   = %q{Ruby wrapper for libwebp}
  spec.summary       = %q{Ruby wrapper for libwebp}
  spec.homepage      = "http://leopard.in.ua/webp-ffi"
  spec.license       = "MIT"

  # only what users need: specs and test images stay in the repository
  spec.files         = Dir.chdir(__dir__) do
    Dir["lib/**/*.rb", "ext/webp_ffi/*.{c,h,rb}", "ext/webp_ffi/Rakefile",
        "README.md", "CHANGELOG.md", "LICENSE.txt"]
  end
  spec.require_paths = ["lib"]
  spec.required_ruby_version = ">= 3.2"
  spec.extensions    << 'ext/webp_ffi/Rakefile'

  spec.add_runtime_dependency "ffi", ">= 1.9.0"
  spec.add_runtime_dependency "ffi-compiler", ">= 0.1.2"
  # gem install runs ext/webp_ffi/Rakefile to build the extension
  spec.add_runtime_dependency "rake"

  spec.add_development_dependency "bundler", ">= 1.2"
  spec.add_development_dependency "rspec", ">= 3"
end
