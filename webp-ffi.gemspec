# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'webp_ffi/version'

Gem::Specification.new do |spec|
  spec.name          = "webp-ffi"
  spec.version       = WebpFfi::VERSION
  spec.authors       = ["Alexey Vasyliev"]
  spec.email         = ["leopard.not.a@gmail.com"]
  spec.description   = %q{Ruby wrapper for libwebp}
  spec.summary       = %q{Ruby wrapper for libwebp}
  spec.homepage      = "http://leopard.in.ua/webp-ffi"
  spec.license       = "MIT"

  spec.files         = `git ls-files`.split($/)
  spec.executables   = spec.files.grep(%r{^bin/}) { |f| File.basename(f) }
  spec.test_files    = spec.files.grep(%r{^(test|spec|features)/})
  spec.require_paths = ["lib"]
  spec.extensions    << 'ext/webp_ffi/Rakefile'

  spec.add_runtime_dependency "ffi", "~> 1.4.0"
  spec.add_runtime_dependency "ffi-compiler", "~> 0.1.1"

  spec.add_development_dependency "bundler", ">= 1.2"
  spec.add_development_dependency "rake"
  spec.add_development_dependency "rspec"
end
