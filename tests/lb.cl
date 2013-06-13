#include "clc.h"

#define NSPEEDS 5

#define DENSITY      0.1f
#define ACCEL        0.005f
#define OMEGA        1.85f
#define VISCOSITY    (1.0f / 6.0f * (2.0f / OMEGA - 1.0f))

// Constant expressions
#define accel_w1 (DENSITY * ACCEL / 3.f)
#define c_sq (1.f/3.f)
#define inv_c_sq (1.f/c_sq)
#define inv_2c_sq (1.f/(2.f*c_sq))
#define inv_2c_sq2 (1.f/(2.f*c_sq*c_sq))
#define w0 (1.f/3.f)
#define w1 (1.f/6.f)

kernel void timestep(int nx, int ny,
                     global float *cells_0,
                     global float *cells_in1,
                     global float *cells_in2,
                     global float *cells_in3,
                     global float *cells_in4,
                     global float *cells_out1,
                     global float *cells_out2,
                     global float *cells_out3,
                     global float *cells_out4,
                     global float *av_velocities,
                     __local float *l_velocities)
{
  int x = get_global_id(0) % nx;
  int y = get_global_id(0) / nx;

  //bool valid = y < ny;
  //y = valid ? y : ny-1;
  y = min(y, ny-1);

  float av_velocity = 0.f;

  // Propagate
  const int y_n = (y + 1) % ny;
  const int x_e = (x + 1) % nx;
  const int y_s = (y == 0) ? (y + ny - 1) : (y - 1);
  const int x_w = (x == 0) ? (x + nx - 1) : (x - 1);
  const float speed0 = cells_0[y*nx + x];
  const float speed1 = cells_in1[y*nx + x_w];
  const float speed2 = cells_in2[y_s*nx + x];
  const float speed3 = cells_in3[y*nx + x_e];
  const float speed4 = cells_in4[y_n*nx + x];

  const bool collide = speed0 >= 0;
  const float accel = collide && x==0 ? accel_w1 : 0.f;

  // Cell density
  float local_density = speed0 + speed1 + speed2 + speed3 + speed4;

  // Compute velocities
  const float u_x = (speed1 - speed3) / local_density;
  const float u_y = (speed2 - speed4) / local_density;
  const float u_sq = u_x*u_x + u_y*u_y;

  // Equilibrium densities
  // Relaxation step
  const float d_equ = w0 * local_density * (1.f - u_sq*inv_2c_sq);
  const float _speed0 = collide ? speed0 + OMEGA * (d_equ - speed0) : -1.f;
  cells_0[x + y*nx] = _speed0;

  const float w1ld = w1 * local_density;
  const float xterm = w1ld + w1ld*((u_x*u_x)*inv_2c_sq2 - u_sq*inv_2c_sq);

  const float d_equ1 = w1ld*u_x* inv_c_sq + xterm;
  const float _speed1 = collide ?
    (speed1 + OMEGA*(d_equ1 - speed1) + accel) :
    speed3;
  cells_out1[x + y*nx] = _speed1;

  const float d_equ3 = w1ld*u_x*-inv_c_sq + xterm;
  const float _speed3 = collide ?
    (speed3 + OMEGA*(d_equ3 - speed3) - accel) :
    speed1;
  cells_out3[x + y*nx] = _speed3;

  const float yterm = w1ld + w1ld*((u_y*u_y)*inv_2c_sq2 - u_sq*inv_2c_sq);

  const float d_equ2 = w1ld*u_y* inv_c_sq + yterm;
  const float _speed2 = collide ?
    (speed2 + OMEGA*(d_equ2 - speed2)) :
    speed4;
  cells_out2[x + y*nx] = _speed2;

  const float d_equ4 = w1ld*u_y*-inv_c_sq + yterm;
  const float _speed4 = collide ?
    (speed4 + OMEGA*(d_equ4 - speed4)) :
    speed2;
  cells_out4[x + y*nx] = _speed4;

  local_density = _speed0;
  local_density += _speed1;
  local_density += _speed2;
  local_density += _speed3;
  local_density += _speed4;
  av_velocity = (collide ? _speed1-_speed3 : 0.f)/local_density;

  // Reduction
  int l_size = get_local_size(0);
  int l = get_local_id(0);
  l_velocities[l] = av_velocity;
  for (int offset = l_size>>1; offset >= 1; offset>>=1)
  {
    barrier(CLK_LOCAL_MEM_FENCE);
    if (l < offset)
    {
      l_velocities[l] += l_velocities[l + offset];
    }
  }
  if (l == 0)
  {
    int group = get_group_id(0);
    av_velocities[group] = l_velocities[0];
  }
}

kernel void reduce(int n, int step, int ncells,
                   global float *av_velocities,
                   global float *velocities,
                   local float *l_velocities)
{
  int i = get_global_id(0);
  int l = get_local_id(0);
  int l_size = get_local_size(0);

  // Private reduction of n/l_size elements
  float velocity = 0.f;
  for (int j = i; j < n; j+=l_size)
  {
    velocity += velocities[j];
  }

  // Local reduction of remaining l_size elements
  l_velocities[l] = velocity;
  for (int offset = l_size>>1; offset >= 1; offset>>=1)
  {
    barrier(CLK_LOCAL_MEM_FENCE);
    if (l < offset)
    {
      l_velocities[l] += l_velocities[l + offset];
    }
  }

  // Write final result
  if (l == 0)
  {
    av_velocities[step] = l_velocities[0]/ncells;
  }
}
