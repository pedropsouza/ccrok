#include "fvec.h"
#include "phys.h"
#include "serde.h"
#include "volksrace/volksrace.h"

wheel_dyndata *wheel_update(wheel_dyndata *w,
                            fvec vel,
                            float weight,
                            float car_angle,
                            float steer_angle) {
  float sa = car_angle + 3.14156/2.0 + steer_angle;
  
  fvec steer_versor = fvec_xy(cos(sa), sin(sa));
  float steer_mag = tan((steer_angle+car_angle) - atan2(vel.y, vel.x))*fvec_mag(vel);
  if (steer_mag > weight * w->static_friction_coeff) {
    // wheel is sliding
    w->state = WHEEL_SLIDING;
    steer_mag = weight * w->dyn_friction_coeff; // FIXME, wrong approximation
  }
  w->friction = fvec_xy(
    steer_versor.x * steer_mag,
    steer_versor.y * steer_mag
  );    
  return w;
}

void car_update(car *c) {
  for (int i = 0; i < 4; i++) {
    wheel_update(
      c->wheels + i,
      c->body.vel, 1.0, c->heading_angle, c->steering_angle
    );
    physbody_accel(&c->body, c->wheels[i].friction);
  }
  physbody_tick(&c->body);
}


