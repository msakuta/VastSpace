---
title: jHitSphere
---

[English](jHitSphere.html)

# 概要

jHitSphere という関数は3次元空間で光線と球体の衝突を判定します。

ゲームエンジン内で弾丸と物体の当たり判定や、恒星の光が惑星によって遮られるかの判定などに使われています。

原理を忘れて線形代数を思い出すのに時間がかかったので、後での参照用に記録しておきます。

# 原理

この関数はベクトル演算を使って直線と球体の共有点を見つけます。

直線が次のベクトル方程式で表されたとしましょう：

$$\vec{r}=\vec{a}+t\vec{d}$$

そして球体も：

$$\vert \vec{r}-\vec{b}\vert =R$$

共有点は方程式を連立することで求まります。

$$\vert \vec{a}+t\vec{d}-\vec{b}\vert =R$$

相対的な位置を定ベクトル $\vec{r_0}=\vec{a}-\vec{b}$ と見なすと方程式は次のようになります：

$$\vert \vec{r_0}+t\vec{d}\vert =R$$

絶対値の項を展開すると次の式になります。

$$\vert \vec{r_0}\vert ^2+2t\vec{r_0}\cdot\vec{d}+t^2\vert \vec{d}\vert ^2=R^2$$

tについて2次方程式を解くと次の答えが出ます。

$$t=\frac{-\vec{r_0}\cdot\vec{d}\pm\sqrt{(\vec{r_0}\cdot\vec{d})^2-\vert \vec{d}\vert ^2(\vert \vec{r_0}\vert ^2-R^2)}}{\vert \vec{d}\vert ^2}$$

この $t$ の2つの値は、共有点を示すベクトル方程式のパラメータです。

判別式

$$D=(\vec{r_0}\cdot\vec{d})^2-\vert \vec{d}\vert ^2(\vert \vec{r_0}\vert ^2-R^2)$$

の符号を調べることで、共有点があるかどうかを知ることができます。

この結果から実装が次のようになるわけです。

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

実際のコード中のいろいろな処理はオプションだとか細かい話で、論理の核心は上に示されているので全てです。

なお、虚数解であろうとも、直線と球の中心との最近点は共有点の中点をとることでわかります。

$$t_m=-\frac{\vec{r_0}\cdot\vec{d}}{\vert \vec{d}\vert ^2}$$
