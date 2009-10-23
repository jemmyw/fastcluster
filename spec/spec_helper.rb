begin
  require 'spec'
rescue LoadError
  require 'rubygems'
  gem 'rspec'
  require 'spec'
end

require File.dirname(__FILE__) + '/../lib/fastcluster'
require File.dirname(__FILE__) + '/test_data.rb'
