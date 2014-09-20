---
title: Triangle
---

# 概要

ここでは2次元平面上のある点が任意の三角形の内側にいるかどうか(効率的に)求める方法を論じます。

スーパー基本中の基本で車輪の再発明なところはいつもと同じです。

# ベクトルの外積を使って求める

どうしましょうか。

<div align="center"><img src="Triangle-fig1.svg" style="width: 250px;"/></div>

まずは点の位置ベクトルと各辺を巡るベクトルの外積を取って、その符号がすべて同じだったら、
その点はどの辺を表すベクトルの進む向きから見ても同じ側にあるので、三角形の内側にあるはずだ、
ということを考えます。

<div align="center"><img src="Triangle-fig2.svg" style="width: 250px;"/></div>

図中の名前を使って式で表すと

$$
(r-a) \times (b-a) \\
(r-b) \times (c-b) \\
(r-c) \times (a-c)
$$

がすべて同じ符号ということです。

これは感覚的には、車であるルートを左回りに一周して戻ってくる経路上で、常に尖塔を左手に見続けていれば、
尖塔はそのループの中にあるはずだ、というような話です。
外積の符号は、右回りか左回りかを表しているわけで、三角形の頂点がどの順で与えられるかで変わります。


# 基底の変換を使って求める

効率ということでいうと、実は後工程で$(b-a)-(c-a)$空間の座標も求めたい事情があります。
ようするに点が三角形の中のどのあたりにあるかも知りたいわけです。

<div align="center"><img src="Triangle-fig3.svg" style="width: 250px;"></div>

とりあえず$b-a$を$x'$軸、$c-a$を$y'$軸とおきます。すると$x'-y'$は斜交座標系の基底とみなせるので、
$x-y$座標系からの変換を考えることができます。

原点は一致させる線形変換として、まあ中学生的に成分を書き下してみましょうか。

$$
x' = ax + by \\
y' = cx + dy
$$

ベクトル式っぽくしてみます。

\begin{align}
\mathbf{x}' &= A \mathbf{x}, & A=
\begin{bmatrix}
a & b \\
c & d
\end{bmatrix}
\tag{1} \label{1}
\end{align}

ここで一度見方を逆にして、$x'-y'$から$x-y$座標系へ変換する問題を考えると、
$x,y$は$x',y'$の2変数関数であるとみることができます。
また、線形変換ですから高々1階までの微分係数しか持っていないわけです。
つまり

$$
x = \frac{\partial x}{\partial x'} x' + \frac{\partial x}{\partial y'} y' \\
y = \frac{\partial y}{\partial x'} x' + \frac{\partial y}{\partial y'} y' \tag{2} \label{2}
$$

と表せるわけです（原点を一致させているという条件を使って定数項を消去しています）。

ベクトルの書き方に戻ると、ヤコビの行列を使って次のようになります。

$$
\mathbf{x} = \frac{\partial(x,y)}{\partial(x',y')} \mathbf{x}'
$$

式\eqref{1}と比べて、次の$A$を求めれば座標変換ができます。もちろん$ \frac{\partial(x,y)}{\partial(x',y')}$は正則であるという前提ですが、
$x',y'$が平行でなければ（空間を張れば）正則なので、三角形であれば問題ないでしょう。

$$
A = \left( \frac{\partial(x,y)}{\partial(x',y')} \right)^{-1}
$$

ここで$\frac{\partial x_i}{\partial x_j'}$とはなんなのかを考えると、$x_j'$が単位長さ動いたときの$x_i$の増分ですから、
これは$x', y'$ベクトルを$x-y$座標系で表した時の成分です。
これは元の座標系の三角形の形として与えられているものです。

元の記号に戻って書くと

$$
r' = \begin{bmatrix}
(b-a)_x & (c-a)_x \\
(b-a)_y & (c-a)_y
\end{bmatrix}^{-1} (r-a)
$$

ということになります。

この変換後の座標系から見ると、三角形はかならず次のように見えます。

<div align="center"><img src="Triangle-fig4.svg"  style="width: 250px;"></div>

この中にいるかどうか判定するのは自明ですね。$0&lt;x', 0&lt;y', x'+y'&lt;1$が同時に成り立つ場合です。
また、$x',y'$は三角形のどのあたりにいるかも教えてくれます。例えば頂点間の線形補完に使えます。


## テンソルを使った考え方

式\eqref{2}において$x=x_1, y=x_2, x'=x_1', y'=x_2'$とおくと

\begin{align}
x_i =& \frac{\partial x_i}{\partial x_1'} x_1' + \frac{\partial x_i}{\partial x_2'} x_2' \\
=& \sum_j \frac{\partial x_i}{\partial x_j'} x_j'
\end{align}

とまとめて書けます。


$$
x = (\mathbf{x}' \cdot \mathbf{r}) \mathbf{e_x} + (\mathbf{x}' \cdot \mathbf{r}) \mathbf{e_y}
$$
