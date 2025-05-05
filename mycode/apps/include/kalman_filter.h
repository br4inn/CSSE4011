#ifndef _KALMAN_FILTER_H_
#define _KALMAN_FILTER_H_

void kalman_filter(float x_hat[4], float cov[4][4], float x_var, float y_var, float co_var, float pos[2]);

#endif