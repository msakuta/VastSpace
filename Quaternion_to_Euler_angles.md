---
title: Converting Quaternion to Euler Angles
---


# Overview

This article discuss how to convert a quaternion to a set of Euler angles.
This mathematics is not trivial that I can forget the rationale if I don't record it.


# Theory

Suppose we have a quaternion $\mathbf{q}$ having element $q_i$ representing a 3-dimensional orientation.
The quaternion can be expressed in complex-like form:

$$\mathbf{q} = q_0 + q_1 \mathbf{i} + q_2 \mathbf{j} + q_3 \mathbf{k}$$

Another form to express an orientation is to use Euler angles $(\phi, \theta, \psi)$.
The Euler angles are often called pitch, yaw and roll in flight dynamics.

A quaternion and a set of Euler angles are equivalent.
You can convert Euler angles to a quaternion by making quaternions for each axis angles and then obtaining product of
these quaternions, but the other way around is not as obvious.

The problem is that there are multiple conventions for Euler angles' definition, so we cannot easily copy and paste from
some random source on the Internet.  Instead, we need to intrinsically understand how to convert one another.
But there's one article whose purpose is close to ours:

[http://graphics.wikia.com/wiki/Conversion_between_quaternions_and_Euler_angles](http://graphics.wikia.com/wiki/Conversion_between_quaternions_and_Euler_angles)

Unfortunately, the article's definition for axes is different from ours.

In our case, Z-axis points *backward*, X-axis to the right and Y-axis upward.
This convention is chosen because it forms Cartesian coordinates if we were to look from the vehicle's view, and
we (and OpenGL) prefer right hand system.

Thus, our definitions are:

* Roll - $\phi$: rotation about the Z-axis
* Pitch - $\theta$: rotation about the X-axis
* Yaw - $\psi$: rotation about the Y-axis

Rotation order is the same as the reference, i.e. in the order yaw, pitch, roll.

Rotation quaternions for each angle are defined as below:

$$ \mathbf{q_\phi} = \cos(\phi/2) + \sin(\phi/2) \mathbf{k} $$
$$ \mathbf{q_\theta} = \cos(\theta/2) + \sin(\theta/2) \mathbf{i} $$
$$ \mathbf{q_\psi} = \cos(\psi/2) + \sin(\psi/2) \mathbf{j} $$


## Rotation matrices
Rotation matrix corresponding to the quaternion $\mathbf{q}$ is the same as the reference.

$$ \begin{bmatrix}
 1- 2(q_2^2 + q_3^2) &  2(q_1 q_2 - q_0 q_3) &  2(q_0 q_2 + q_1 q_3) \\
2(q_1 q_2 + q_0 q_3) & 1 - 2(q_1^2 + q_3^2)  &  2(q_2 q_3 - q_0 q_1) \\
2(q_1 q_3 - q_0 q_2) & 2( q_0 q_1 + q_2 q_3) &  1 - 2(q_1^2 + q_2^2)
\end{bmatrix} $$

The rotation matrices for each of Euler angles $(\phi, \theta, \psi)$ are

$$ \mathbf{R}_\phi =
\begin{bmatrix}
\cos\phi & -\sin\phi & 0 \\
\sin\phi &  \cos\phi & 0 \\
0        &  0        & 1 \\
\end{bmatrix} $$

$$ \mathbf{R}_\theta =
\begin{bmatrix}
1 & 0 & 0 \\
0 & \cos\theta & -\sin\theta \\
0 & \sin\theta &  \cos\theta \\
\end{bmatrix} $$

$$ \mathbf{R}_\psi =
\begin{bmatrix}
 \cos\psi & 0 & \sin\psi \\
 0        & 1 & 0        \\
-\sin\psi & 0 & \cos\psi \\
\end{bmatrix} $$

Product of those 3 matrices yields

$$ \mathbf{R}_\psi \mathbf{R}_\theta \mathbf{R}_\phi =
\begin{bmatrix}
\cos\phi \cos\psi - \sin\phi \cos\theta \sin\psi & -\sin\phi \cos\theta & \cos\phi \sin\psi + \sin\phi \sin\theta \cos\psi \\
\sin\phi \cos\psi + \cos\phi \sin\theta \sin\phi &  \cos\phi \cos\theta & \sin\phi \sin\psi - \cos\phi \sin\theta \cos\psi \\
-\cos\theta \sin\psi & \sin\theta & \cos\theta \cos\psi \\
\end{bmatrix}
$$

$\mathbf{q}$ and $\mathbf{R}_\psi \mathbf{R}_\theta \mathbf{R}_\phi$ must be equal, so we can compare the elements to obtain the relationships

$$
\theta = \sin^{-1} \{2(q_0 q_1 + q_2 q_3)\} \\
\phi = \tan^{-1} \left\{ -\frac{2(q_1 q_2 - q_0 q_3)}{1 - 2({q_1}^2 + {q_3}^2)} \right\} \\
\psi = \tan^{-1} \left\{ -\frac{2(q_1 q_3 - q_0 q_2)}{1 - 2({q_1}^2 + {q_2}^2)} \right\}
$$
