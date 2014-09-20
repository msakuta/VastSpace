---
title: jHitSphere
---

[日本語](jHitSphere-ja.html)

# Overview

The jHitSphere function checks if a ray of light is blocked by a sphere in 3-D space.

This function is used by the game engine to check if bullets hit objects, and to
check if a star's light is blocked by a planet.

I have been forgotten the rationale and had hard time remembering the linear algebra,
so I record the process for later reference.

# Rationale

The function resolves intersecting points of a line and a sphere using vector arithmetic.

Suppose we represent a line in space by vector equation:

$$\vec{r}=\vec{a}+t\vec{d}$$

and a sphere:

$$|\vec{r}-\vec{b}|=R$$

The intersecting points are obtained by forming a system of equations.

$$|\vec{a}+t\vec{d}-\vec{b}|=R$$

The relative position can be assumed as a constant vector $\vec{r_0}=\vec{a}-\vec{b}$ and the equation becomes:

$$|\vec{r_0}+t\vec{d}|=R$$

Expanding the absolute value term yields following equation.

$$\|\vec{r_0}\|^2+2t\vec{r_0}\cdot\vec{d}+t^2\|\vec{d}\|^2=R^2$$

Solving the 2nd order equation about $t$ yields the following answer.

$$t=\frac{-\vec{r_0}\cdot\vec{d}\pm\sqrt{(\vec{r_0}\cdot\vec{d})^2-|\vec{d}|^2(|\vec{r_0}|^2-R^2)}}{|\vec{d}|^2}$$

This two $t$ values are the vector equation parameters of the intersecting points.

Examining sign of the discriminant of the equation

$$D=(\vec{r_0}\cdot\vec{d})^2-|\vec{d}|^2(|\vec{r_0}|^2-R^2)$$

tells us if the intersection occurs at all.

This result leads us to the implementation.

    bool jHitSphere(const Vec3d &obj, double radius, const Vec3d &src, const Vec3d &dir, double dt){
      double b, c, D, d, t0, t1, dirslen;
      bool ret;

      Vec3d del = src - obj;

      /* scalar product of the ray and the vector. */
      b = dir.sp(del);

      dirslen = dir.slen();
      c = dirslen * (del.slen() - radius * radius);

      /* Discriminant */
      D = b * b - c;
      if(D <= 0)
        return false;

      d = sqrt(D);

      /* Avoid zerodiv */
      if(dirslen == 0.)
        return false;
      t0 = (-b - d) / dirslen;
      t1 = (-b + d) / dirslen;

      return 0. <= t1 && t0 < dt;
    }

All other codes in actual source are options or implementation details. The core of the logic is all shown above.

Also, you can retrieve the position closest to the sphere's center by using average of the intersecting points,

$$t_m=-\frac{\vec{r_0}\cdot\vec{d}}{|\vec{d}|^2}$$

even if the solution is imaginary.
