require File.dirname(__FILE__) + '/lib/fastcluster'
require 'benchmark'

points = [[237, 434], [282, 435], [281, 429], [241, 427], [259, 434], [499, 218], [254, 431], [222, 433], [253, 441], [212, 440], [252, 432], [279, 433], [248, 428], [249, 202], [249, 202], [252, 202], [252, 202], [562, 402], [728, 23], [227, 424], [267, 428], [247, 438], [290, 452]]

puts Benchmark.measure {
clusterer = Fastcluster::Clusterer.new(25, 15, points)
clusters = clusterer.clusters
puts clusters.inspect

#clusters.sort{|a,b| a.size == b.size ? a.x <=> b.x : a.size <=> b.size }.each do |cluster|
#  puts cluster
#end

}
