Gem::Specification.new do |s|
  s.name = "fastcluster"
  s.version = "1.0.1"
  s.platform = Gem::Platform::RUBY
  s.summary = "A clustering library for 2 dimensional points"
  s.description = "A clustering library for 2 dimensional points"
  s.homepage = "http://github.com/jemmyw/fastcluster"
  s.authors = ["Jeremy Wells"]
  s.email = ["jemmyw@gmail.com"]

  s.files = Dir["ext/*.rb"] + Dir['ext/*.c'] + Dir["lib/**/*.rb"] + ["CHANGELOG", "README.rdoc"]
  s.test_files = Dir["spec/**/*.rb"] + ["test.rb"]
  s.extensions << 'ext/extconf.rb'
end
