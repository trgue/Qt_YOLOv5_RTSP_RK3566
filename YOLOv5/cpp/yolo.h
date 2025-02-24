#ifndef __YOLO_H__
#define __YOLO_H__

#include "yolov5.h"

int yolo_init(char *model_path_tmp);
int yolo_detect(image_buffer_t* src_image);





#endif