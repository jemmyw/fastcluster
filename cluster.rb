module Clusterer
  class Cluster
    attr_reader :x, :y, :size

    def initialize(x, y, size)
      @x = x
      @y = y
      @size = size
    end

    def to_s
      '(%f, %f): %d' % [@x, @y, @size]
    end
  end
end