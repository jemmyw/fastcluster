#include <ruby.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
  double x;
  double y;
} POINT;

typedef struct {
  double x;
  double y;
  long size;
} CLUSTER;

static int num_points = 0;
static int num_allocated_points = 0;
static POINT *point_array = NULL;

static int AddToPointsArray (POINT * itemPtr) {
  if(num_points == num_allocated_points) {
      if (num_allocated_points == 0)
        num_allocated_points = 3;
      else
        num_allocated_points *= 2;
      void *_tmp = realloc(point_array, (num_allocated_points * sizeof(POINT)));
      if (!_tmp) {
        fprintf(stderr, "ERROR: Couldn't realloc memory!\n");
        return(-1);
      }
      point_array = (POINT*)_tmp;
    }

    POINT item = *itemPtr;

//    memcpy(&point_array[num_points], &item, sizeof(POINT));
    point_array[num_points] = item;
    num_points++;
    return num_points;
}


static VALUE addPoint(VALUE self, VALUE x, VALUE y) {
  double rx = NUM2DBL(x);
  double ry = NUM2DBL(y);

  POINT temp;
  temp.x = rx;
  temp.y = ry;

  AddToPointsArray(&temp);

  return Qnil;
}

static double distance_from(CLUSTER * one, CLUSTER * two) {
  double rr = pow((long)one->x - (long)two->x, 2) + pow((long)one->y - (long)two->y, 2);
  return sqrt(rr);
}

static int resolution = 5;
static int separation = 105;

static void addToCluster(CLUSTER * dst, double x, double y) {
  dst->x = ((dst->x * dst->size) + x) / (dst->size + 1);
  dst->y = ((dst->y * dst->size) + y) / (dst->size + 1);
  dst->size++;
}

static void combineClusters(CLUSTER * dst, CLUSTER * src) {
  dst->x = (dst->x*dst->size + src->x*src->size) / (dst->size+src->size);
  dst->y = (dst->y*dst->size + src->y*src->size) / (dst->size+src->size);
  dst->size = dst->size + src->size;
}

static long getMaxGrid() {
  int i;
  int max_grid = 0;
  for(i = 0; i < num_points; i++) {
    POINT * point = &point_array[i];
    int xg = point->x/resolution;
    int yg = point->y/resolution;
    if(xg>max_grid)
      max_grid = xg;
    if(yg>max_grid)
      max_grid = yg;
  }
  return max_grid+1;
}


static VALUE getClusters(VALUE self) {
  int max_grid = getMaxGrid();
  int i, j;
  void *_tmp;

  POINT * point;
  CLUSTER * cluster;
  
  CLUSTER grid_array[max_grid][max_grid];
  long preclust_size = 0;

  for(i=0;i<max_grid;i++) {
    for(j=0;j<max_grid;j++) {
      grid_array[i][j].x = 0;
      grid_array[i][j].y = 0;
      grid_array[i][j].size = 0;
    }
  }

  for(i = 0; i < num_points; i++) {
    point = &point_array[i];

    int gx = floor(point->x/resolution);
    int gy = floor(point->y/resolution);

    addToCluster(&grid_array[gx][gy], point->x, point->y);

    if(grid_array[gx][gy].size == 1) preclust_size++;
  }

  printf("Number of preclusters: %d\n", preclust_size);

  CLUSTER *preclusters = NULL;
  _tmp = realloc(preclusters, (preclust_size * sizeof(CLUSTER)));
  preclusters = (CLUSTER*)_tmp;

  for(i=0;i<preclust_size;i++) {
    preclusters[i].x = preclusters[i].y = preclusters[i].size = 0;
  }

  int incr = 0;
  for(i = 0; i < max_grid; i++) {
    for(j = 0; j < max_grid; j++) {
      CLUSTER * one = &grid_array[i][j];

      if(one->size > 0) {
        CLUSTER * two = &preclusters[incr];

        two->x = one->x;
        two->y = one->y;
        two->size = one->size;
        
        printf("pre: %f, %f, %d\n", one->x, one->y, one->size);


        incr++;
      }
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

      CLUSTER *newarr = NULL;
      _tmp = realloc(newarr, (preclust_size * sizeof(CLUSTER)));
      newarr = (CLUSTER*)_tmp;

      memcpy(&newarr[0], &preclusters[0], nearest_other * sizeof(CLUSTER));
      memcpy(&newarr[nearest_other], &preclusters[nearest_other+1], (preclust_size - (nearest_other + 1)) * sizeof(CLUSTER));

      void *_tmp = realloc(preclusters, ((preclust_size-1) * sizeof(CLUSTER)));
      preclusters = (CLUSTER*)_tmp;
      preclust_size = preclust_size - 1;

      for(i=0;i<preclust_size;i++)
        preclusters[i] = newarr[i];
    }
  } while(distance_sep <= separation && preclust_size > 1);

  VALUE cluster_class = rb_eval_string("require File.dirname(__FILE__) + '/cluster'; Clusterer::Cluster");
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

  return ruby_cluster_array;
}

void Init_clusterer() {
  VALUE clustererModule = rb_define_module("Clusterer");
  VALUE clustererClass = rb_define_class_under(clustererModule, "Base", rb_cObject);

  int arg_count = 2;
  rb_define_method(clustererClass, "add", addPoint, arg_count);

  arg_count = 0;
  rb_define_method(clustererClass, "clusters", getClusters, arg_count);
}