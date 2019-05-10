#pragma once

#ifdef __cplusplus
extern "C" {
#endif

double gt_to_rad(double value_deg);
double gt_to_deg(double value_rad);
double gt_get_distance(double src_lat, double src_lon,
                       double dst_lat, double dst_lon);

#ifdef __cplusplus
}
#endif
