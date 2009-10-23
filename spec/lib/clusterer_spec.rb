require File.dirname(__FILE__) + '/../spec_helper'
require 'benchmark'

describe Clusterer do
  before do
    @clusterer = Clusterer::Base.new(105, 5)
    @points = POINTS
  end

  describe '#add' do
    it 'should add an x y point to the clusterer' do
      @clusterer.add(5, 10)
      @clusterer.points.size.should == 1
      @clusterer.points.first.should == [5, 10]
    end
  end

  describe '#<<' do
    it 'should add the value to the clusterer' do
      @clusterer << [5, 10]
      @clusterer.points.size.should == 1
      @clusterer.points.first.should == [5, 10]
    end
  end

  describe '#clusters' do
    before do
      @points.each do |point|
        @clusterer.add(point[0], point[1])
      end
      @clusters = @clusterer.clusters.sort{|a, b| a.size == b.size ? a.x <=> b.x : a.size <=> b.size }
    end

    it 'should take less than 1 second' do
      time = Benchmark.measure { @clusterer.clusters }
      time.total.should be < 1
    end

    it 'should have as many cluster points as data points' do
      @clusters.inject(0){|m, n| m + n.size }.should == @points.size
    end

    it 'should have a cluster of 109 points at 835 by 178' do
      @clusters.last.size.should == 109
      @clusters.last.x.should be_close(835, 1)
      @clusters.last.y.should be_close(178, 1)
    end

    it 'should have a cluster of 1 point at 97 by 1203' do
      @clusters.first.size.should == 1
      @clusters.first.x.should be_close(97, 1)
      @clusters.first.y.should be_close(1203, 1)
    end
  end
end