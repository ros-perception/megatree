#ifndef MEGATREE_COMPRESSION_H
#define MEGATREE_COMPRESSION_H

#include <Eigen/Geometry>
#include <megatree_storage/VizPoint.h>


namespace megatree
{

void toSphericalCoordinates(const double point[3], double res[3])
{
  // distance 
  res[2] = sqrt(point[0] * point[0] +
                point[1] * point[1] +
                point[2] * point[2]);

  // up-down angle
  res[0] = asin(point[2]/res[2]);

  // left-right angle
  res[1] = atan2(point[1], point[0]);
}

void fromSphericalCoordinates(const double sph_point[3], double res[3])
{
  res[0] = cos(sph_point[0]) * cos(sph_point[1]) * sph_point[2];
  res[1] = cos(sph_point[0]) * sin(sph_point[1]) * sph_point[2];
  res[2] = sin(sph_point[0]) * sph_point[2];
}





void transform(const double point[3], const Eigen::Affine3d &transform, double res[3])
{
  Eigen::Vector3d tmp_res =  transform * Eigen::Vector3d(point[0], point[1], point[2]);  

  res[0] = tmp_res[0];
  res[1] = tmp_res[1];
  res[2] = tmp_res[2];
}



void compress(const double& value, const double& min_value, const double& max_value, uint16_t& res)
{
  res = (value - min_value) * 65536.0 / (max_value - min_value) - 0.5;
}


void extract(const uint16_t& value, const double& min_value, const double& max_value, double& res)
{
  res = ((double)(value) + 0.5) * (max_value - min_value) / 65536.0 + min_value;
}





void compressSphericalPoint(const double sph_point[3], const double min_value[3], const double max_value[3], uint16_t res[3])
{
  compress(sph_point[0], min_value[0], max_value[0], res[0]);
  compress(sph_point[1], min_value[1], max_value[1], res[1]);
  compress(log(sph_point[2]), log(min_value[2]), log(max_value[2]), res[2]);

  /*
    printf("compress:   dist %f %f,  min %f %f,   max %f %f\n", 
    sph_point[2], log(sph_point[2]),
    min_value[2], log(min_value[2]),
    max_value[2], log(max_value[2]));
  */           
}

void extractSphericalPoint(const uint16_t compressed_point[3], const double min_value[3], const double max_value[3], double res[3])
{
  extract(compressed_point[0], min_value[0], max_value[0], res[0]);
  extract(compressed_point[1], min_value[1], max_value[1], res[1]);
  extract(compressed_point[2], log(min_value[2]), log(max_value[2]), res[2]);
  /*
    printf("extract:   dist %f %f,  min %f %f,   max %f %f\n", 
    res[2], exp(res[2]),
    min_value[2], log(min_value[2]),
    max_value[2], log(max_value[2]));
  */
  res[2] = exp(res[2]);
}




void extractPoint(const megatree_storage::VizPoint& msg_point, 
                  const Eigen::Affine3d &viewpoint, double min_cell_size, double max_cell_size, 
                  double point[3], double color[3])
{
  uint16_t compressed_point[3];
  compressed_point[0] = msg_point.angle_vertical;
  compressed_point[1] = msg_point.angle_horizontal;
  compressed_point[2] = msg_point.distance_log;

  double sph_point[3];
  double min_value[3] = {-M_PI, -M_PI, min_cell_size};
  double max_value[3] = { M_PI,  M_PI, max_cell_size};
  extractSphericalPoint(compressed_point, min_value, max_value, sph_point);

  double camera_point[3];
  fromSphericalCoordinates(sph_point, camera_point);

  // convert point from camera frame to absolute frame
  transform(camera_point, viewpoint, point);

  // get color
  color[0] = msg_point.r;
  color[1] = msg_point.g;
  color[2] = msg_point.b;

  /*
    printf("Compressed point %d %d %d\n", compressed_point[0], compressed_point[1], compressed_point[2]);
    printf("Spherical point %f %f %f\n", sph_point[0], sph_point[1], sph_point[2]);
    printf("Camera point %f %f %f\n", camera_point[0], camera_point[1], camera_point[2]);
    printf("World point %f %f %f\n", point[0], point[1], point[2]);  
  */
}


}

#endif
