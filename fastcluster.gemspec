Gem::Specification.new do |s|
  s.name = "fastcluster"
  s.version = "0.9"
  s.platform = Gem::Platform::RUBY
  s.summary = "A clustering library for 2 dimensional points"
  s.description = "A clustering library for 2 dimensional points"
  s.homepage = "http://github.com/jemmyw/fastcluster"
  s.authors = ["Jeremy Wells"]
  s.email = ["jemmyw@gmail.com"]

  s.files = Dir["ext/**/*"] + Dir["lib/**/*"] + ["CHANGELOG", "README.rdoc"]
  s.test_files = Dir["spec/**/*.rb"] + ["test.rb"]
end
