begin
  require 'spec'
rescue LoadError
  require 'rubygems'
  gem 'rspec'
  require 'spec'
end

require File.dirname(__FILE__) + '/../clusterer'
require File.dirname(__FILE__) + '/../cluster'
require File.dirname(__FILE__) + '/test_data.rb'
