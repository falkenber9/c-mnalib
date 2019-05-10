
#include "cmnalib/gps_transform.h"

#include <math.h>


double gt_to_rad(double value_deg) {
    return value_deg * M_PI / 180;
}

double gt_to_deg(double value_rad) {
    return value_rad * 180 / M_PI;
}

double gt_get_distance(double src_lat, double src_lon,
                       double dst_lat, double dst_lon) {
    double distance_m = 0;

    if(!(src_lat==dst_lat && src_lon==dst_lon)) {
        double lat0 = gt_to_rad(src_lat);
        double lon0 = gt_to_rad(src_lon);
        double lat1 = gt_to_rad(dst_lat);
        double lon1 = gt_to_rad(dst_lon);

        double sinTerm = sin(lat0) * sin(lat1);
        double cosTerm = cos(lat0) * cos(lat1) * cos(lon1 - lon0);

        distance_m = acos(sinTerm + cosTerm) * 40000 / 360 * 1000;    // map to earth
        distance_m = gt_to_deg(distance_m);
    }

    return distance_m;
}
