
lib = File.expand_path("../lib", __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require "version"

Gem::Specification.new do |spec|
  spec.name          = "sqlite-vss"
  spec.version       = SqliteVss::VERSION
  spec.authors       = ["Alex Garcia"]
  spec.email         = ["alexsebastian.garcia@gmail.com"]

  spec.summary       = "a"
  spec.description   = "b"
  spec.homepage      = "https://github.com/asg017/sqlite-vss"
  spec.license       = "MIT"

  # The --platform flag would work in most cases, but on a GH action
  # linux runner, it would set platform to "ruby" and not "x86-linux".
  # Setting this to Gem::Platform::CURRENT
  spec.platform      = ENV['PLATFORM']

  if spec.respond_to?(:metadata)

    spec.metadata["homepage_uri"] = spec.homepage
    spec.metadata["source_code_uri"] = spec.homepage
    spec.metadata["changelog_uri"] = spec.homepage
  else
    raise "RubyGems 2.0 or newer is required to protect against " \
      "public gem pushes."
  end

  spec.files = Dir["lib/*.rb"] + Dir.glob('lib/*.{so,dylib,dll}')

  spec.require_paths = ["lib"]

  spec.add_development_dependency "bundler", "~> 1.17"
  spec.add_development_dependency "rake", "~> 10.0"
end
