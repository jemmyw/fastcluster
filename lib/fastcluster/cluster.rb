module Fastcluster
  class Cluster
    include Comparable

    attr_reader :x, :y, :size

    def initialize(x, y, size)
      @x = x
      @y = y
      @size = size
    end

    def <=>(anOther)
      size <=> anOther.size
    end

    def inspect
      to_s
    end

    def to_s
      '(%0.2f, %0.2f): %d' % [@x, @y, @size]
    end
  end
end