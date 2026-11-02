require_relative 'native_gem'

namespace :native do
  desc "Build libwebp_ffi with libwebp, libpng, libjpeg-turbo, libtiff and zlib linked in (needs cmake)"
  task :build do
    sh "sh", File.expand_path("../script/build-native.sh", __dir__)
  end

  desc "Package a prebuilt gem from tmp/native/out, for example rake native:gem[x86_64-linux-gnu]"
  task :gem, [:platform] do |_task, args|
    platforms = WebPFFI::NativeGem::PLATFORMS.keys.join(", ")
    abort "usage: rake native:gem[PLATFORM], PLATFORM is one of #{platforms}" unless args[:platform]
    puts "Built #{WebPFFI::NativeGem.new(args[:platform]).build}"
  end
end

namespace :release do
  desc "Check that a release tag matches the gem version, for example rake release:check_version[v1.0.0]"
  task :check_version, [:tag] do |_task, args|
    abort "usage: rake release:check_version[TAG]" unless args[:tag]
    WebPFFI::NativeGem.check_tag!(args[:tag])
    puts "#{args[:tag]} matches the gem version"
  end
end
