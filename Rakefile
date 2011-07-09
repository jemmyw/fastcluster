require 'spec'
require 'spec/rake/spectask'

desc "Comile the gem"
task :compile do
  Dir.chdir('ext') do
    ruby 'extconf.rb'
  end

  Dir.chdir('ext') do
    pid = fork { exec 'make' }
    Process.waitpid pid
  end

  FileUtils.cp('ext/clusterer.so', 'lib/clusterer.so') if File.exists?('ext/clusterer.so')
  FileUtils.cp('ext/clusterer.bundle', 'lib/clusterer.bundle') if File.exists?('ext/clusterer.bundle')
end

desc "Remove compiled files"
task :clean do
  %w(o so bundle).each do |ext|
    system("rm -rf ext/*.#{ext}")
    system("rm -rf lib/*.#{ext}")
  end
end

desc "Build then gem"
task :build do
  pid = fork { exec 'gem build fastcluster.gemspec' }
  Process.waitpid pid
end

desc "Run the specs under spec"
Spec::Rake::SpecTask.new do |t|
  t.spec_opts = ['--options', "spec/spec.opts"]
  t.spec_files = FileList['spec/**/*_spec.rb']
end
