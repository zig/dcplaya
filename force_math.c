#include <math.h>

static float force2(float v)
{
  return 0.2f;
}

static float force(float v)
{
  float w = 0;
  
  v += 0.1;

  w += cos(v);
  w += sin(v);
  w += cos(v);
  w += sin(v);
  w += tan(v);
  w += atan(v);
  w += atan2(v,v);
  w += sqrt(v);
  w += pow(v,v);
  w += acos(v);
  w += exp(v); 
  w += hypot(v,v);
  w += ldexp(v,2);
  w += log(v);

  return w;
}

int force_math(float v)
{
  return force(v) + force2(v);
}
