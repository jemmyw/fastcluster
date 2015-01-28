#include <ruby.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef RUBY_19
#ifndef RFLOAT_VALUE
#define RFLOAT_VALUE(v) (RFLOAT(v)->value)
#endif
#ifndef RARRAY_LEN
#define RARRAY_LEN(v) (RARRAY(v)->len)
#endif
#ifndef RARRAY_PTR
#define RARRAY_PTR(v) (RARRAY(v)->ptr)
#endif
#endif

/*
*
* Algorithm:
*   all points are initially clusters with size 1
*   precluster - create a grid of size @resolution and cluster the points in each grid space automatically
*   loop until no cluster is less that @separation apart
*     combine two closest clusters, the new cluster has the summed size and the averaged distance (size weighted)
*     between the clusters.
**/
typedef struct {
  float x;
  float y;
  long size;
} CLUSTER;

/*
* An array of points to be clustered.
*/
static VALUE fc_get_points(VALUE self) {
  return rb_iv_get(self, "@points");
}

/*
* call-seq:
*   add(x, y) -> nil
*
* Add a point to this clusterer.
*/
static VALUE fc_add_point(VALUE self, VALUE x, VALUE y) {
  long len = 2;
  VALUE holdArray = rb_ary_new3(2, x, y);
  VALUE pointArray = fc_get_points(self);
  rb_ary_push(pointArray, holdArray);

  return Qnil;
}

/*
 * call-seq:
 *   <<(point) -> nil
 *
 * Add a point to this clusterer. The point must be in the format
 * of an array with two number.
 *
 * Example:
 *  clusterer << [1, 2]
 */
static VALUE fc_append_point(VALUE self, VALUE point) {
  VALUE pointArray = fc_get_points(self);
  rb_ary_push(pointArray, point);
  return Qnil;
}

/*
* Calculate the distance (pythag) between two cluster points
*/
static float fc_get_distance_between(CLUSTER * one, CLUSTER * two) {
  float rr = pow((long)one->x - (long)two->x, 2) + pow((long)one->y - (long)two->y, 2);
  return sqrt(rr);
}

/*
* Add a point to a cluster. This increments the size and calcualtes the average between
* the current cluster position and the new point.
*/
static void fc_add_to_cluster(CLUSTER * dst, float x, float y) {
  dst->x = ((dst->x * dst->size) + x) / (dst->size + 1);
  dst->y = ((dst->y * dst->size) + y) / (dst->size + 1);
  dst->size++;
}

/*
* Combine two clusters into one with an average center point
*/
static void fc_combine_clusters(CLUSTER * dst, CLUSTER * src) {
  dst->x = (dst->x*dst->size + src->x*src->size) / (dst->size+src->size);
  dst->y = (dst->y*dst->size + src->y*src->size) / (dst->size+src->size);
  dst->size = dst->size + src->size;
}

/*
* Get the maximum grid size
*/
static long fc_get_max_grid(long resolution, CLUSTER * point_array, long num_points) {
  int i;
  long max_grid = 0;
  for(i = 0; i < num_points; i++) {
    CLUSTER * point = &point_array[i];
    long xg = point->x/resolution;
    long yg = point->y/resolution;
    if(xg>max_grid)
      max_grid = xg;
    if(yg>max_grid)
      max_grid = yg;
  }
  return max_grid+1;
}

/*
* call-seq:
*   new(separation = 0, resolution = 0, points = nil)
*
* Create a new Clusterer. The new method accepts 3 optional arguments, separation,
* resolution and points.
*
* <tt>separation</tt> - The distance between clusters. The higher this number, the
* less clusters there will be. If this is 0 then no clustering will occur.
*
* <tt>resolution</tt> - If specified then the points are placed on a grid with each grid square
* being this size. Points falling in the same grid square are automatically clustered.
* This option should be specified clustering larger number of points to reduce processing time.
*
* <tt>points</tt> - An array of points. Each array item must be an array with
* two numbers (x, y). Example: <code>[[1, 2], [3, 4]]</code>.
*/
static VALUE fc_initialize_clusterer(int argc, VALUE *argv, VALUE self) {
  if(argc > 0)
    rb_iv_set(self, "@separation", argv[0]);
  else
    rb_iv_set(self, "@separation", INT2FIX(0));

  if(argc > 1)
    rb_iv_set(self, "@resolution", argv[1]);
  else
    rb_iv_set(self, "@resolution", INT2FIX(0));

  VALUE pointArray = rb_ary_new();
  rb_iv_set(self, "@points", pointArray);

  if(argc > 2) {
    if(TYPE(argv[2]) == T_ARRAY) {
      rb_iv_set(self, "@points", argv[2]);
    }
  }

  return Qnil;
}

/*
* Turn the ruby array of points (format [[x,y], [x,y]]) into an array of
* CLUSTER
*/
static void fc_native_point_array(CLUSTER * arrayPtr, VALUE rubyArray, long num_points) {
  int i;
  for(i=0;i<num_points;i++) {
    VALUE holdArray = RARRAY_PTR(rubyArray)[i];
    float x = NUM2DBL(RARRAY_PTR(holdArray)[0]);
    float y = NUM2DBL(RARRAY_PTR(holdArray)[1]);

    arrayPtr[i].x = x;
    arrayPtr[i].y = y;
    arrayPtr[i].size = 1;
  }
}

/*
* This function does the actual clustering. It takes the following params:
*
* <tt>separation</tt> - The minimum distance between clusters. At 0 there will be one cluster with all the points.
* <tt>resolution</tt> - Any points that fall within resolution distance will be clustered automatically.
* <tt>point_array</tt> - Array of points to cluster.
* <tt>num_points</tt> - Size of the point array.
* <tt>cluster_size</tt> - Pointer for a variable to receive the size of the returned array.
*
* This function return an array of CLUSTER.
*/
static CLUSTER *fc_calculate_clusters(long separation, long resolution, CLUSTER * point_array, long num_points, long * cluster_size) {
  int i, j;
  long preclust_size = 0;

  CLUSTER * cluster;
  CLUSTER * clusters;

  // This first section does preclustering. The points are split into a grid where each
  // grid box is of the size resolutionxresolution. When more than one point falls
  // in a grid box they are clustered.
  long max_grid = fc_get_max_grid(resolution, &point_array[0], num_points);

  // Only precluster if a resolution is specified
  if(resolution > 0) {
    CLUSTER grid_array[max_grid][max_grid];

    for(i=0;i<max_grid;i++) {
      for(j=0;j<max_grid;j++) {
        grid_array[i][j].size = 0;
      }
    }

    // Add clusters to grid
    for(i = 0; i < num_points; i++) {
      cluster = &point_array[i];

      long gx = floor(cluster->x/resolution);
      long gy = floor(cluster->y/resolution);

      fc_add_to_cluster(&grid_array[gx][gy], cluster->x, cluster->y);

      // If the grid array is holding a cluster of size 1 at this point
      // then its a new cluster, so the preclust_size is incremented.
      if(grid_array[gx][gy].size == 1) preclust_size++;
    }

    // Now the grid clusters are copied into an array
    clusters = malloc(preclust_size * sizeof(CLUSTER));

    long max_grid_total = max_grid * max_grid;
    CLUSTER * gridPtr = grid_array[0];

    int incr = 0;
    for(i=0;i<max_grid_total;i++) {
      if(gridPtr[i].size > 0) {
        clusters[incr] = gridPtr[i];
        incr++;
      }
    }
  } else {
    // As there is no grid just copy the original point array into a new array
    preclust_size = num_points;
    clusters = malloc(preclust_size * sizeof(CLUSTER));
    memcpy(&clusters[0], &point_array[0], preclust_size * sizeof(CLUSTER));
  }

  float distance_sep = 0;
  long current_cluster_size = 0;
  int found;
  long nearest_origin = 0;
  long nearest_other;

  do {
    distance_sep = 0;
    nearest_other = 0;

    for(i=0;i<preclust_size;i++){
      for(j=i+1;j<preclust_size;j++){
        float distance = fc_get_distance_between(&clusters[i], &clusters[j]);

        if(distance_sep == 0 || distance < distance_sep) {
          distance_sep = distance;

          if(distance < separation || separation == 0) {
          nearest_origin = i;
          nearest_other = j;
          }
        }
      }
    }

    // If two clusters have been identified for merging, this part merges
    // them into the first cluster and removes the second cluster from the array
    if(nearest_other > 0) {
      // merge into first cluster
      fc_combine_clusters(&clusters[nearest_origin], &clusters[nearest_other]);

      // remove second cluster by creating temporary array without it
      CLUSTER *newarr = malloc(preclust_size * sizeof(CLUSTER));
      memcpy(&newarr[0], &clusters[0], nearest_other * sizeof(CLUSTER));
      memcpy(&newarr[nearest_other], &clusters[nearest_other+1], (preclust_size - (nearest_other + 1)) * sizeof(CLUSTER));

      void *_tmp = realloc(clusters, ((preclust_size-1) * sizeof(CLUSTER)));
      clusters = (CLUSTER*)_tmp;
      preclust_size = preclust_size - 1;

      // and copying it back
      memcpy(&clusters[0], &newarr[0], preclust_size * sizeof(CLUSTER));

      free(newarr);
    }
  // keep looping until either everything is in one cluster, or all clusters are
  // outside the separation distance.
  } while((separation == 0 || distance_sep < separation) && preclust_size > 1);

  *cluster_size = preclust_size;
  return clusters;
}

/*
* Get the ruby class Cluster
*/
static VALUE fc_get_cluster_class() {
  ID cluster_module_id = rb_intern("Fastcluster");
  ID cluster_class_id = rb_intern("Cluster");
  VALUE cluster_module = rb_const_get(rb_cObject, cluster_module_id);
  return rb_const_get(cluster_module, cluster_class_id);
}

/*
* Return the clusters found for the points in this clusterer. This will be an
* array of Cluster objects.
*
* Example:
*   clusterer = Fastcluster::Clusterer.new(3, 0, [[1, 1], [1, 2], [5, 9]])
*   clusterer.clusters -> [(1.00, 1.50): 2, (5.00, 9.00): 1]
*/
static VALUE fc_get_clusters(VALUE self) {
  // Get the separation adn resolution from ruby
  long separation = NUM2INT(rb_iv_get(self, "@separation"));
  long resolution = NUM2INT(rb_iv_get(self, "@resolution"));
  int i;

  // Create a native array of clusters from the ruby array of points
  VALUE pointArray = fc_get_points(self);
  long num_points = RARRAY_LEN(pointArray);
  CLUSTER native_point_array[num_points];

  fc_native_point_array(&native_point_array[0], pointArray, num_points);

  // Calcualte the clusters
  CLUSTER * clusters = NULL;
  long cluster_size;

  clusters = fc_calculate_clusters(separation, resolution, &native_point_array[0], num_points, &cluster_size);

  // Create ruby array of clusters to return
  VALUE cluster_class = fc_get_cluster_class();
  VALUE ruby_cluster_array = rb_ary_new2(cluster_size);

  for(i=0;i<cluster_size;i++) {
    int arg_count = 3;
    VALUE arg_array[arg_count];

    arg_array[0] = rb_float_new(clusters[i].x);
    arg_array[1] = rb_float_new(clusters[i].y);
    arg_array[2] = INT2FIX(clusters[i].size);

    VALUE cluster_obj = rb_class_new_instance(arg_count, arg_array, cluster_class);
    rb_ary_push(ruby_cluster_array, cluster_obj);
  }

  // Free the clusters array
  free(clusters);

  return ruby_cluster_array;
}

void Init_clusterer() {
  VALUE clustererModule = rb_define_module("Fastcluster");
  VALUE clustererClass = rb_define_class_under(clustererModule, "Clusterer", rb_cObject);

  rb_define_method(clustererClass, "initialize", fc_initialize_clusterer, -1);
  rb_define_method(clustererClass, "add", fc_add_point, 2);

  rb_define_method(clustererClass, "<<", fc_append_point, 1);

  rb_define_method(clustererClass, "clusters", fc_get_clusters, 0);
  rb_define_method(clustererClass, "points", fc_get_points, 0);
}
