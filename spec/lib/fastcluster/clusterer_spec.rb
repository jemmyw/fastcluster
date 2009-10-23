require File.dirname(__FILE__) + '/../../spec_helper'
require 'benchmark'

describe Fastcluster::Clusterer do
  before do
    @points = POINTS
  end

  it 'should allow setting points in initializer' do
    @clusterer = Fastcluster::Clusterer.new(105, 5, @points)
    @clusterer.points.size.should == 168
  end

  describe 'instance' do
    before do
      @clusterer = Fastcluster::Clusterer.new(105, 5)
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
  end

  describe '#clusters' do
    describe 'with large test' do
      before do
        @clusterer = Fastcluster::Clusterer.new(105, 5, @points)
        @clusters = @clusterer.clusters.sort{|a, b| a.size == b.size ? a.x <=> b.x : a.size <=> b.size }
      end

      it 'should take less than 1 second' do
        time = Benchmark.measure { @clusterer.clusters }
        time.total.should be < 1
      end

      it 'should have as many cluster points as data points' do
        @clusters.inject(0){|m, n| m + n.size }.should == @points.size
      end

      it 'should have a cluster of 108 points at 836 by 178' do
        @clusters.last.size.should == 108
        @clusters.last.x.should be_close(836, 1)
        @clusters.last.y.should be_close(178, 1)
      end

      it 'should have a cluster of 1 point at 97 by 1203' do
        @clusters.first.size.should == 1
        @clusters.first.x.should be_close(97, 1)
        @clusters.first.y.should be_close(1203, 1)
      end
    end

    describe "with two points" do
      before do
        @clusterer = Fastcluster::Clusterer.new(0, 0, [[1, 5], [2, 8]])
      end

      it "should return one cluster" do
        @clusterer.clusters.size.should == 1
      end

      it "should have two points in the cluster" do
        @clusterer.clusters.first.size.should == 2
      end
    end

    describe "with three points" do
      before do
        @clusterer = Fastcluster::Clusterer.new(0, 0, [[1, 2], [5, 6], [2, 3]])
        @clusters = @clusterer.clusters
      end

      it "should return one cluster" do
        @clusters.size.should == 1
      end

      it "containing three items" do
        @clusters.first.size.should == 3
      end
    end

    describe "with four points" do
      before do
        @points = [
            [0, 1],
            [1, 0],
            [3, 4],
            [4, 3],
        ]
      end

      describe "and no separation" do
        before do
          @clusterer = Fastcluster::Clusterer.new(0, 0, @points)
        end

        it "should return one cluster" do
          @clusterer.clusters.size.should == 1
        end
      end

      describe "and separation 1" do
        before do
          require 'lib/fastcluster'
          @clusterer = Fastcluster::Clusterer.new(1, 0, @points)
        end

        it "should return all four individual points" do
          @clusterer.clusters.size.should == 4
        end
      end

      describe "and separation 2" do
        before do
          @clusterer = Fastcluster::Clusterer.new(2, 0, @points)
        end

        it "should return two clusters" do
          @clusterer.clusters.size.should == 2
        end
      end
    end

    describe "with eight points" do
      before do
        @points = [
            [0, 1],
            [1, 0],
            [3, 4],
            [4, 3],
            [7, 8],
            [8, 7],
            [8, 9],
            [9, 8]
        ]
      end

      describe "and no separation" do
        before do
          @clusterer = Fastcluster::Clusterer.new(0, 0)
          @points.each do |point|
            @clusterer << point
          end
        end

        it "should return one cluster when no minimum separation is given" do
          @clusterer.clusters.size.should == 1
        end
      end

      describe "and separation 1" do
        before do
          @clusterer = Fastcluster::Clusterer.new(1, 0)
          @points.each do |point|
            @clusterer << point
          end
        end

        it "should have all eight points in individual clusters" do
          @clusterer.clusters.size.should == 8
        end
      end

      describe "and separation 3" do
        describe "with no resolution limit" do
          before do
            @clusterer = Fastcluster::Clusterer.new(3, 0)
            @points.each do |point|
              @clusterer << point
            end
            @clusters = @clusterer.clusters.sort
          end

          it "should have three clusters" do
            @clusters.size.should == 3
          end

          it "should have clusters size 2, 2, and 4 " do
            @clusters[0].size.should == 2
            @clusters[1].size.should == 2
            @clusters[2].size.should == 4
          end
        end

        describe "with coarse resolution" do
          before do
            @clusterer = Fastcluster::Clusterer.new(3, 5)
            @points.each do |point|
              @clusterer << point
            end
            @clusters = @clusterer.clusters.sort
          end

          it "should have three clusters" do
            @clusters.size.should == 2
          end

          it "should have clusters size 2, 2, and 4 " do
            @clusters[0].size.should == 4
            @clusters[1].size.should == 4
          end
        end
      end
    end
  end
end