#include <ruby.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
  double x;
  double y;
  long size;
} CLUSTER;

static VALUE getPoints(VALUE self) {
  return rb_iv_get(self, "@points");
}

/*
* Add a point to the array
*/
static VALUE addPoint(VALUE self, VALUE x, VALUE y) {
  long len = 2;
  VALUE holdArray = rb_ary_new3(2, x, y);
  VALUE pointArray = getPoints(self);
  rb_ary_push(pointArray, holdArray);

  return Qnil;
}

/*
* Calculate the distance (pythag) between two cluster points
*/
static double distance_from(CLUSTER * one, CLUSTER * two) {
  double rr = pow((long)one->x - (long)two->x, 2) + pow((long)one->y - (long)two->y, 2);
  return sqrt(rr);
}

/*
* Add a point to a cluster. This increments the size and calcualtes the average between
* the current cluster position and the new point.
*/
static void addToCluster(CLUSTER * dst, double x, double y) {
  dst->x = ((dst->x * dst->size) + x) / (dst->size + 1);
  dst->y = ((dst->y * dst->size) + y) / (dst->size + 1);
  dst->size++;
}

/*
* Combine two clusters into one with an average center point
*/
static void combineClusters(CLUSTER * dst, CLUSTER * src) {
  dst->x = (dst->x*dst->size + src->x*src->size) / (dst->size+src->size);
  dst->y = (dst->y*dst->size + src->y*src->size) / (dst->size+src->size);
  dst->size = dst->size + src->size;
}

/*
* Get the maximum grid size
*/
static long getMaxGrid(long resolution, CLUSTER * point_array, long num_points) {
  int i;
  int max_grid = 0;
  for(i = 0; i < num_points; i++) {
    CLUSTER * point = &point_array[i];
    int xg = point->x/resolution;
    int yg = point->y/resolution;
    if(xg>max_grid)
      max_grid = xg;
    if(yg>max_grid)
      max_grid = yg;
  }
  return max_grid+1;
}

/*
* initialize function for clusterer class. This creates the instance variables
* for separation, resolution and points.
*/
static VALUE initializeClusterer(VALUE self, VALUE separation, VALUE resolution) {
  rb_iv_set(self, "@separation", separation);
  rb_iv_set(self, "@resolution", resolution);

  VALUE pointArray = rb_ary_new();
  rb_iv_set(self, "@points", pointArray);

  return Qnil;
}

/*
* Turn the ruby array of points (format [[x,y], [x,y]]) into an array of
* CLUSTER
*/
static void nativePointArray(CLUSTER * arrayPtr, VALUE rubyArray, long num_points) {
  int i;
  for(i=0;i<num_points;i++) {
    VALUE holdArray = RARRAY(rubyArray)->ptr[i];
    double x = NUM2DBL(RARRAY(holdArray)->ptr[0]);
    double y = NUM2DBL(RARRAY(holdArray)->ptr[1]);

    arrayPtr[i].x = x;
    arrayPtr[i].y = y;
    arrayPtr[i].size = 1;
  }
}

static VALUE getClusters(VALUE self) {
  long separation = NUM2INT(rb_iv_get(self, "@separation"));
  long resolution = NUM2INT(rb_iv_get(self, "@resolution"));

  VALUE pointArray = getPoints(self);
  long num_points = RARRAY(pointArray)->len;
  CLUSTER point_array[num_points];

  nativePointArray(&point_array[0], pointArray, num_points);

  int max_grid = getMaxGrid(resolution, &point_array[0], num_points);
  int i, j;
  void *_tmp;

  CLUSTER * cluster;
  
  CLUSTER grid_array[max_grid][max_grid];
  long preclust_size = 0;

  for(i=0;i<max_grid;i++) {
    for(j=0;j<max_grid;j++) {
      grid_array[i][j].size = 0;
    }
  }

  for(i = 0; i < num_points; i++) {
    cluster = &point_array[i];

    int gx = floor(cluster->x/resolution);
    int gy = floor(cluster->y/resolution);

    addToCluster(&grid_array[gx][gy], cluster->x, cluster->y);

    if(grid_array[gx][gy].size == 1) preclust_size++;
  }

  CLUSTER *preclusters = malloc(preclust_size * sizeof(CLUSTER));
  
  int max_grid_total = max_grid * max_grid;
  CLUSTER * gridPtr = grid_array[0];

  int incr = 0;
  for(i=0;i<max_grid_total;i++) {
    if(gridPtr[i].size > 0) {
      preclusters[incr] = gridPtr[i];
      incr++;
    }
  }

  double distance_sep = 0;
  long current_cluster_size = 0;
  int found;
  long nearest_origin;
  long nearest_other;

  do {
    // calculate distance sep
    distance_sep = 0;
    nearest_other = 0;

    for(i=0;i<preclust_size;i++){
      for(j=i+1;j<preclust_size;j++){
        double distance = distance_from(&preclusters[i], &preclusters[j]);

        if(distance_sep == 0 || distance < distance_sep) {
          distance_sep = distance;
          nearest_origin = i;
          nearest_other = j;
        }
      }
    }

    if(nearest_other > 0) {
      combineClusters(&preclusters[nearest_origin], &preclusters[nearest_other]);

      CLUSTER *newarr = malloc(preclust_size * sizeof(CLUSTER));
      memcpy(&newarr[0], &preclusters[0], nearest_other * sizeof(CLUSTER));
      memcpy(&newarr[nearest_other], &preclusters[nearest_other+1], (preclust_size - (nearest_other + 1)) * sizeof(CLUSTER));

      void *_tmp = realloc(preclusters, ((preclust_size-1) * sizeof(CLUSTER)));
      preclusters = (CLUSTER*)_tmp;
      preclust_size = preclust_size - 1;

      for(i=0;i<preclust_size;i++)
        preclusters[i] = newarr[i];

      free(newarr);
    }
  } while(distance_sep <= separation && preclust_size > 1);

  ID cluster_module_id = rb_intern("Clusterer");
  ID cluster_class_id = rb_intern("Cluster");
  VALUE cluster_module = rb_const_get(rb_cObject, cluster_module_id);
  VALUE cluster_class = rb_const_get(cluster_module, cluster_class_id);
  VALUE ruby_cluster_array = rb_ary_new2(preclust_size);

  for(i=0;i<preclust_size;i++) {
    int arg_count = 3;
    VALUE arg_array[arg_count];

    arg_array[0] = rb_float_new(preclusters[i].x);
    arg_array[1] = rb_float_new(preclusters[i].y);
    arg_array[2] = INT2FIX(preclusters[i].size);

    VALUE cluster_obj = rb_class_new_instance(arg_count, arg_array, cluster_class);
    rb_ary_push(ruby_cluster_array, cluster_obj);
  }
  
  free(preclusters);

  return ruby_cluster_array;
}

void Init_clusterer() {
  VALUE clustererModule = rb_define_module("Clusterer");
  VALUE clustererClass = rb_define_class_under(clustererModule, "Base", rb_cObject);

  int arg_count = 2;
  rb_define_method(clustererClass, "initialize", initializeClusterer, arg_count);

  rb_define_method(clustererClass, "add", addPoint, arg_count);
  rb_define_method(clustererClass, "<<", addPoint, arg_count);

  arg_count = 0;
  rb_define_method(clustererClass, "clusters", getClusters, arg_count);
  rb_define_method(clustererClass, "points", getPoints, arg_count);
}