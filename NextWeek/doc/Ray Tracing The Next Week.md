# 1. 概述

在《Ray Tracing in One Weekend》中，您构建了一个简单的强力光线追踪器。在本期中，我们将使用 BVH 添加纹理、体积（如雾）、矩形、实例、灯光以及对许多对象的支持。完成后，您将拥有一个“真正的”光线追踪器。

许多人（包括我）都认为，光线追踪的一个启发是，大多数优化都会使代码复杂化，但不会带来太多加速。在这本迷你书中，我将在我做出的每个设计决策中采用最简单的方法。检查 https://in1weekend.blogspot.com/ 以获取更复杂方法的阅读材料和参考资料。但是，我强烈建议您不要进行过早的优化；如果它在执行时间配置文件中显示不高，则在支持所有功能之前不需要优化！

本书最难的两个部分是 BVH 和 Perlin 纹理。这就是为什么标题建议你花一周而不是一个周末来进行这项工作。但如果你想要一个周末项目，你可以把这些留到最后。对于本书中介绍的概念来说，顺序并不是很重要，如果没有 BVH 和 Perlin 纹理，您仍然会得到一个 Cornell Box！

# 2. 运动模糊

当你决定使用光线追踪时，你就决定了视觉质量比运行时间更重要。在渲染模糊反射(*fuzz reflection*)和虚焦模糊(*defocus blur*)时，我们对每个像素使用多个样本。一旦你走上这条路，好消息是几乎所有效果都可以以类似的方式暴力解决。运动模糊当然是其中之一。

在真实相机中，快门在短时间间隔内保持打开状态，期间相机和世界中的物体可能会移动。为了准确再现这样的相机拍摄，我们寻求在快门对世界敞开时相机感知到的平均值。

## 2.1 时空光线追踪介绍

我们可以在快门打开时的某个随机时刻发送单个光线，以获得单个（简化的）光子的随机估计。只要我们能确定物体在那一瞬间的位置，我们就能在同一时刻精确测量出该光线的亮度。这是随机（*Monte Carlo*）光线追踪如何变得非常简单的另一个例子。暴力解决再次取得胜利！

由于光线跟踪器的 "引擎 "只需确保对象位于每条光线所需的位置，因此交点的内部结构不会有太大变化。为了实现这一点，我们需要存储每条光线的准确时间：

```cpp
// ray.h
class ray {
  public:
    ray() {}

    ray(const point3& origin, const vec3& direction) : orig(origin), dir(direction), tm(0)
    {}

    ray(const point3& origin, const vec3& direction, double time = 0.0)
      : orig(origin), dir(direction), tm(time)
    {}
    point3 origin() const  { return orig; }
    vec3 direction() const { return dir; }    
    double time() const    { return tm; }
    point3 at(double t) const {
        return orig + t*dir;
    }

  private:
    point3 orig;
    vec3 dir;    double tm;};
```

## 2.2 管理时间

在继续之前，让我们思考一下时间，以及我们可能如何在一次或多次连续渲染中管理时间。快门计时有两个方面需要考虑：**一个快门打开到下一个快门打开的时间**，以及**每帧快门保持打开的时间长短**。标准电影胶片曾经是每秒24帧拍摄的。现代数字电影可以是每秒24、30、48、60、120或导演想要的任何帧数。

每一帧都可以有自己的快门速度。这个快门速度不必是(通常也不是)整个帧的最大持续时间。你可以每一帧都将快门打开 1/1000 秒，或者 1/60 秒。

如果你想渲染一系列图像，你需要使用适当的快门时间设置相机：**帧到帧的周期**，**快门/渲染持续时间**，以及**总帧数**（总拍摄时间）。如果相机在移动而世界是静态的，你就可以开始了。然而，如果世界中的任何东西在移动，你需要为可撞击物添加一个方法，以便每个物体都能意识到当前帧的时间周期。这种方法将为所有动画对象提供在该帧期间设置其运动的方式。

这相当直接，并且绝对是一个有趣的途径，如果你愿意，可以尝试。然而，就我们目前的目的而言，我们将以一种更简单的模型继续。我们只会渲染单帧，隐含地假设从时间 = 0 开始到时间 = 1 结束。我们的第一个任务是修改相机以在 [0,1] 中发射随机时间的光线，我们的第二个任务将是创建一个动画球体类。

## 2.3 更新相机以模拟运动模糊

我们需要修改相机以在开始时间和结束时间之间的随机时刻生成光线。相机是否应该跟踪时间间隔，还是应该由相机的用户在创建光线时决定？在有疑惑时，我喜欢让构造函数变得复杂，这样可以简化调用时的操作，所以我会让相机保持跟踪，但这是个人偏好。由于现在不允许相机移动，所以相机不需要太多改变；它只是在一段时间内发送光线。

```cpp
// camera.h
class camera {
  ...
  private:
    ...
    ray get_ray(int i, int j) const {
        // 为 i,j 处的像素获取随机采样的摄像机光线，该光线来自相机散焦盘.

        auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
        auto pixel_sample = pixel_center + pixel_sample_square();

        auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
        auto ray_direction = pixel_sample - ray_origin;        
        auto ray_time = random_double();

        return ray(ray_origin, ray_direction, ray_time);    }

    ...
};
```

## 2.4 添加移动球体

现在创建一个移动对象。我将更新`sphere`类，使其中心从 `time=0` 时的 `center1` 线性移动到 `time=1` 时的 `center2`。（它在那个时间间隔之外无限期地继续，因此确实可以随时进行采样。）

```cpp
// sphere.h
class sphere : public hittable {
  public:    
    // 静止球体
    sphere(point3 _center, double _radius, shared_ptr<material> _material)
      : center1(_center), radius(_radius), mat(_material), is_moving(false) {}

    // 移动球体
    sphere(point3 _center1, point3 _center2, double _radius, shared_ptr<material> _material)
      : center1(_center1), radius(_radius), mat(_material), is_moving(true)
    {
        center_vec = _center2 - _center1;
    }
    ...

  private:    point3 center1;    double radius;
    shared_ptr<material> mat;    bool is_moving;
    vec3 center_vec;

    point3 center(double time) const {
        // 根据时间从center1 线性插值到center2，其中 t=0 表示center1，t=1 表示center2。
        return center0 + time*center_vec;
    }};

#endif
```

使特殊的静止球体移动的另一种选择是让它们全部移动，但静止球体具有相同的开始和结束位置。我对这种在更简单的代码和更高效的静止球体之间的折衷持怀疑态度，所以让你的设计品味引导你。

更新的 `sphere::hit()` 函数几乎与旧的 `sphere::hit()` 函数相同：`center` 只需要查询一个函数 `sphere_center(time)`：

```cpp
// sphere
class sphere : public hittable {
  public:
    ...
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {        
        point3 center = is_moving ? sphere_center(r.time()) : center1;        
        vec3 oc = r.origin() - center;
        auto a = r.direction().length_squared();
        auto half_b = dot(oc, r.direction());
        auto c = oc.length_squared() - radius*radius;
        ...
    }
    ...
};
```

我们需要实现上面提到的新的 `interval::contains()` 方法：

```cpp
// interval.h
class interval {
  public:
    ...
    bool contains(double x) const {
        return min <= x && x <= max;
    }
    ...
};
```

## 2.5 跟踪光线相交的时间

现在光线具有时间属性，我们需要更新 `material::scatter()` 方法来考虑相交时间：

```cpp
// material.h
class lambertian : public material {
    ...
    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        auto scatter_direction = rec.normal + random_unit_vector();

        // Catch degenerate scatter direction
        if (scatter_direction.near_zero())
            scatter_direction = rec.normal;

        scattered = ray(rec.p, scatter_direction, r_in.time());        attenuation = albedo;
        return true;
    }
    ...
};

class metal : public material {
    ...
    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);        scattered = ray(rec.p, reflected + fuzz*random_in_unit_sphere(), r_in.time());        attenuation = albedo;
        return (dot(scattered.direction(), rec.normal) > 0);
    }
    ...
};

class dielectric : public material {
    ...
    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        ...        scattered = ray(rec.p, direction, r_in.time());        return true;
    }
    ...
};
```

## 2.6 将一切放在一起

下面的代码取自上一本书末尾场景中的示例漫反射球体，并在图像渲染期间使它们移动。每个球体从其中心 $$C$$ 在时间 $$t=0$$ 处移动到时间 $$t=1$$ 处的 $$C+(0,r/2,0)$$：

```cpp
// main.cc
int main() {
    hittable_list world;

    auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, ground_material));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double();
            point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    auto albedo = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);                   
                    auto center2 = center + vec3(0, random_double(0,.5), 0);
                    world.add(make_shared<sphere>(center, center2, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                ...
    }
    ...

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;    
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;    
    cam.max_depth         = 50;

    cam.vfov     = 20;
    cam.lookfrom = point3(13,2,3);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0.02;
    cam.focus_dist    = 10.0;

    cam.render(world);
}
```

这产生了以下结果：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.01-bouncing-spheres.png" alt="*弹跳球体*" />

# 3 包围体层次结构 

这部分是我们迄今为止开发的光线追踪器中**最困难**和**最复杂**的部分。我将它放在这一章节是为了让代码运行得更快，并且因为它稍微重构了一下`hittable`，当我添加矩形和盒子时，我们就不必回头再重构它们。

光线与物体的相交是光线追踪器中的主要时间瓶颈，时间与物体数量成线性关系。但这是对同一模型的重复搜索，因此我们应该能够使其成为类似于二分搜索的对数搜索。因为我们对同一模型发送了数百万到数十亿条光线，我们可以对模型进行类似排序的操作，然后每个光线交点都可以是次线性搜索。最常见的排序方法有两种：1）**划分空间**，和 2）**划分物体**。后者通常更容易编码，对于大多数模型来说运行起来也一样快。

## 3.1 关键思想 

在一组基元上建立包围体的关键思路是找到一个完全包围（边界）所有对象的体。例如，假设你计算了一个包围 10 个物体的球体。任何未击中包围球的光线肯定不会击中内部的所有十个物体。如果光线击中包围球，那么它可能会击中其中的一个物体。因此，边界代码总是这种形式：

```cpp
if (光线击中边界物体)
    return 光线是否击中有边界的物体
else
    return false
```

关键是我们正在将物体划分为子集。我们没有在划分屏幕或体积。任何物体只在一个包围体中，但包围体可以重叠。

## 3.2 包围体的层次结构 

为了实现亚线性性能，我们需要将包围体分层。例如，如果我们将一组物体分为两组，红色和蓝色，并使用矩形包围体，我们会得到：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-2.01-bvol-hierarchy.jpg" alt="包围体层次结构" />

请注意，蓝色和红色的包围体被包含在紫色包围体中，但它们可能会重叠，且不是有序的 — 它们只是都在里面。所以，右边显示的树没有在左右子节点上的排序概念；它们只是在内部。代码将是：

```cpp
if (hits purple)
    hit0 = hits blue enclosed objects
    hit1 = hits red enclosed objects
    if (hit0 or hit1)
        return true and info of closer hit
return false
```

## 3.3 轴对齐包围盒 (AABBs) 

为了使这一切发挥作用，我们需要一种制作良好的划分方法，而不是糟糕的划分方法，以及一种使光线与包围体相交的方法。光线与包围体的相交需要快速进行，且包围体需要相当紧凑。在实践中，对于大多数模型，轴对齐包围盒比其他选择更有效，但如果你遇到不寻常类型的模型，这种设计选择始终需要记在心里。

从现在开始，我们将轴对齐的包围长方体（确实，如果精确，就应该这样称呼）称为轴对齐包围盒（*axis-aligned bounding boxes*, AABBs）。您可以使用任何方法将光线与 AABB 相交。我们只需要知道是否击中了它，而不需要命中点、法线或任何显示对象所需的东西。

大多数人使用“平板法”(*slab*)。这基于一个观察：一个 n 维的 AABB 只是 n 个轴对齐区间的交集，通常被称为“平板”。一个区间就是两个端点之间的点，例如 $$x$$ 满足 $$3<x<5$$，或者更简洁地 $x$ 在 $$(3,5)$$ 之间。在二维空间中，两个间隔重叠区域就是一个二维 AABB（矩形）：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-2.02-2d-aabb.jpg" alt="二维AABB" />

为了确定光线是否击中一个区间，我们首先需要弄清楚光线是否击中了边界。例如，还是在二维空间，这就是射线参数 $$t_0$$ 和 $$t_1$$。（如果光线与平面平行，它与平面的交点将是未定义的。）

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-2.03-ray-slab.jpg" alt="*光线与平板相交*" />

在三维中，这些边界是平面。平面的方程是 $$x=x_0$$ 和 $$x=x_1$$。光线线击中该平面的何处？回想一下，光线可以被看作只是一个给定 $$t$$ 返回位置 $$P(t)$$ 的函数：
$$
P(t)=A+tb
$$
这个方程适用于所有 $$x/y/z$$ 坐标。例如，$$x(t)=A_x+tb_x$$。这个光线在平面 $$x=x_0$$ 上的交点是满足这个方程的参数 $$t$$：
$$
x_0=A_x+t_0b_x
$$
因此，该击中点处的 $$t$$ 是：
$$
t_0=\frac{x_0−A_x}{b_x}
$$
我们对 $$x_1$$ 得到类似的表达式：
$$
t_1=\frac{x_1−A_x}{b_x}
$$
将这种一维数学转化为命中测试的关键点是，为了发生命中，t-区间需要重叠。例如，在二维中，只有当发生碰撞时，绿色和蓝色才会重叠：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-2.04-ray-slab-interval.jpg" alt="光线平板和t区间重叠" />

## 3.4 与 AABB 的光线相交 

以下伪代码用于确定平板中的 $$t$$ 区间是否重叠：

```
compute (tx0, tx1)
compute (ty0, ty1)
return overlap?( (tx0, tx1), (ty0, ty1))
```

这真是太简单了，而且三维版本也可以使用，这就是人们喜欢平板法的原因：

```
compute (tx0, tx1)
compute (ty0, ty1)
compute (tz0, tz1)
return overlap ? ((tx0, tx1), (ty0, ty1), (tz0, tz1))
```

这里有一些需要注意的地方，使得这个方法并不像初看起来那么完美。首先，假设光线沿着 $$-X$$ 方向行进。如上计算的区间 $$(t_{x0},t_{x1})$$ 可能是颠倒的，例如像 $$(7,3)$$ 这样。其次，其中的除法可能产生无穷大值。如果光线原点位于平板边界上，我们可能得到一个 `NaN`。这些问题在各种光线追踪器的 AABB 中处理方式各异。（还有像 SIMD 这样的向量化问题，我们在这里不讨论。如果你想在速度上进行额外的向量化优化，Ingo Wald 的论文是一个很好的起点。）就我们的目的而言，只要我们让它足够快，这不太可能成为主要瓶颈，所以让我们选择最简单的方法，这通常反而是最快的！首先让我们看看计算区间的方法：
$$
t_{x0}=\frac{x_0-A_x}{b_x}\\
t_{x1}=\frac{x_1-A_x}{b_x}
$$
一个麻烦的事情是，完全有效的光线会有 $$b_x=0$$，导致除以零的情况。这些光线中的一些在平板内，一些不在。此外，使用 IEEE 浮点数时，零会有一个 $$\pm$$ 符号。对于 $$b_x=0$$，如果不在 $$x_0$$ 和 $$x_1$$ 之间，$$t_{x0}$$ 和 $t_{x1}$ 都将是 $$+\infin$$ 或都是 $$-\infin$$。因此，使用 min 和 max 应该能得到正确的答案：
$$
t_{x0}=min(\frac{x_0-A_x}{b_x}, \frac{x_1-A_x}{b_x})\\
t_{x1}=max(\frac{x_0-A_x}{b_x}, \frac{x_1-A_x}{b_x})
$$
如果我们这样做，剩下的麻烦情况是 $$b_x=0$$ 且 $$x_0−A_x=0$$ 或 $$x_1−A_x=0$$，因此我们得到一个 `NaN`。在这种情况下，我们可能可以接受击中或不击中的答案，但我们稍后会重新审视这个问题。

现在，让我们看看那个重叠函数。假设我们可以假设区间没有颠倒（因此区间中的第一个值小于第二个值），并且我们希望在这种情况下返回 `true`。同时计算区间重叠 $$(f,F)$$ 的布尔值重叠，对于区间 $$(d,D)$$ 和 $$(e,E)$$ 将是：

```cpp
bool overlap(d, D, e, E, f, F)
    f = max(d, e)
    F = min(D, E)
    return (f < F)
```

如果那里有任何 NaN，在比较时将返回 `false`，因此如果我们关心擦边情况（我们也许应该这样做，因为在光线跟踪器中，所有情况最终都会出现），我们需要确保我们的包围盒有一些填充。这是实现方法：

```cpp
// interval.h
class interval {
  public:
    ...
    double size() const {
        return max - min;
    }

    interval expand(double delta) const {
        auto padding = delta/2;
        return interval(min - padding, max + padding);
    }
    ...
};
```

```cpp
// aabb.h
#ifndef AABB_H
#define AABB_H

#include "rtweekend.h"

class aabb {
  public:
    interval x, y, z;

    aabb() {} // 默认 AABB 为空，因为区间默认为空

    aabb(const interval& ix, const interval& iy, const interval& iz)
      : x(ix), y(iy), z(iz) { }

    aabb(const point3& a, const point3& b) {
        // 将 a 和 b 两点视为边界框的极值，因此我们不需要特定的最小/最大坐标顺序
        x = interval(fmin(a[0],b[0]), fmax(a[0],b[0]));
        y = interval(fmin(a[1],b[1]), fmax(a[1],b[1]));
        z = interval(fmin(a[2],b[2]), fmax(a[2],b[2]));
    }

    const interval& axis(int n) const {
        if (n == 1) return y;
        if (n == 2) return z;
        return x;
    }

    // 这部分看看就好，后面会有一个优化代码
    bool hit(const ray& r, interval ray_t) const {
        for (int a = 0; a < 3; a++) {
            auto t0 = fmin((axis(a).min - r.origin()[a]) / r.direction()[a],
                           (axis(a).max - r.origin()[a]) / r.direction()[a]);
            auto t1 = fmax((axis(a).min - r.origin()[a]) / r.direction()[a],
                           (axis(a).max - r.origin()[a]) / r.direction()[a]);
            ray_t.min = fmax(t0, ray_t.min);
            ray_t.max = fmin(t1, ray_t.max);
            if (ray_t.max <= ray_t.min)
                return false;
        }
        return true;
    }
};

#endif
```

## 3.5 一种优化的AABB命中方法

在审查这种交叉方法时，皮克斯的 Andrew Kensler 尝试了一些实验并提出了以下版本的代码。它在许多编译器上运行得非常好，我已采用它作为我的首选方法：

```cpp
// aabb.h
class aabb {
  public:
    ...
    bool hit(const ray& r, interval ray_t) const {
        for (int a = 0; a < 3; a++) {
            auto invD = 1 / r.direction()[a];
            auto orig = r.origin()[a];

            auto t0 = (axis(a).min - orig) * invD;
            auto t1 = (axis(a).max - orig) * invD;

            if (invD < 0)
                std::swap(t0, t1);

            if (t0 > ray_t.min) ray_t.min = t0;
            if (t1 < ray_t.max) ray_t.max = t1;

            if (ray_t.max <= ray_t.min)
                return false;
        }
        return true;
    }
    ...
};
```



## 3.6 为可命中物构建包围盒 

我们现在需要添加一个函数来计算所有可命中物的包围盒。然后我们将在所有基元上制作一个盒子的层次结构，单个基元（如球体）将位于叶子层。

请回想一下，没有参数构造的`interval`值默认为空。由于一个 `aabb` 对象的三个维度每个都有一个区间，因此每个区间默认都是空的，因此 `aabb` 对象也将默认为空。因此，一些对象可能具有空的包围体。例如一个没有子元素的 `hittable_list` 对象。令人高兴的是，根据我们设计的`interval`类的方式，数学上是合理的。

最后，请注意有些对象可能是动画对象。这些对象应该在从 `time=0` 到 `time=1` 的整个运动范围内返回其边界。

```cpp
// hittable.h
#include "aabb.h"
...

class hittable {
  public:
    ...
    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;

    virtual aabb bounding_box() const = 0;    ...
};
```

对于静止的球体，`bounding_box` 函数很简单：

```cpp
// sphere.h
class sphere : public hittable {
  public:
    // Stationary Sphere
    sphere(point3 _center, double _radius, shared_ptr<material> _material)
      : center1(_center), radius(_radius), mat(_material), is_moving(false)
    {
        auto rvec = vec3(radius, radius, radius);
        bbox = aabb(center1 - rvec, center1 + rvec);
    }
    ...
    aabb bounding_box() const override { return bbox; }

  private:
    point3 center1;
    double radius;
    shared_ptr<material> mat;
    bool is_moving;
    vec3 center_vec;
    aabb bbox;
    ...
};
```

对于移动的球体，我们希望得到其整个运动范围的边界。为此，我们可以取 `time=0` 时球体的盒子和 `time=1` 时球体的盒子，并计算这两个盒子周围的盒子。

```cpp
// sphere.h
class sphere : public hittable {
  public:
    ...
    // Moving Sphere
    sphere(point3 _center1, point3 _center2, double _radius, shared_ptr<material> _material)
      : center1(_center1), radius(_radius), mat(_material), is_moving(true)
    {        auto rvec = vec3(radius, radius, radius);
        aabb box1(_center1 - rvec, _center1 + rvec);
        aabb box2(_center2 - rvec, _center2 + rvec);
        bbox = aabb(box1, box2);
        center_vec = _center2 - _center1;
    }
    ...
};
```

现在我们需要一个新的 `aabb` 构造函数，它接受两个盒子作为输入。首先，我们将添加一个接受两个区间作为输入的新区间构造函数：

```cpp
// interval.h
class interval {
  public:
    ...

    interval(const interval& a, const interval& b)
      : min(fmin(a.min, b.min)), max(fmax(a.max, b.max)) {}
```

现在我们可以使用这个来从两个输入盒子构造一个轴对齐包围盒。

```cpp
// aabb.h
class aabb {
  public:
    ...    aabb(const aabb& box0, const aabb& box1) {
        x = interval(box0.x, box1.x);
        y = interval(box0.y, box1.y);
        z = interval(box0.z, box1.z);
    }    ...
};
```

## 3.7 创建对象列表的包围盒

现在我们将更新 `hittable_list` 对象，计算其子对象的边界。我们将在添加每个新子对象时逐步更新包围盒。

```cpp
// hittable_list.h
...
#include "aabb.h"
...

class hittable_list : public hittable {
  public:
    std::vector<shared_ptr<hittable>> objects;

    ...
    void add(shared_ptr<hittable> object) {
        objects.push_back(object);
        bbox = aabb(bbox, object->bounding_box());
    }

    bool hit(const ray& r, double ray_tmin, double ray_tmax, hit_record& rec) const override {
        ...
    }

    aabb bounding_box() const override { return bbox; }

  private:
    aabb bbox;
};
```

3.8 BVH 节点类

BVH 也将是一个`hittable` — 就像`hittable`的列表一样。它实际上是一个容器，但它可以响应“这个光线是否击中你？”的查询。一个设计问题是我们是否有两个类，一个用于树，一个用于树中的结点；或者我们只有一个类，并且根节点只是一个我们指向的节点。命中函数相当直接：**检查节点的盒子是否被击中，如果是，检查子节点并处理任何细节**。

在可行的情况下，我喜欢采用单类的设计。这里就有一个这样的类：

```cpp
// bvh.h
#ifndef BVH_H
#define BVH_H

#include "rtweekend.h"

#include "hittable.h"
#include "hittable_list.h"


class bvh_node : public hittable {
  public:
    bvh_node(const hittable_list& list) : bvh_node(list.objects, 0, list.objects.size()) {}

    bvh_node(const std::vector<shared_ptr<hittable>>& src_objects, size_t start, size_t end) {
        // To be implemented later.
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        if (!box.hit(r, ray_t))
            return false;

        bool hit_left = left->hit(r, ray_t, rec);
        bool hit_right = right->hit(r, interval(ray_t.min, hit_left ? rec.t : ray_t.max), rec);

        return hit_left || hit_right;
    }

    aabb bounding_box() const override { return bbox; }

  private:
    shared_ptr<hittable> left;
    shared_ptr<hittable> right;
    aabb bbox;
};

#endif
```

## 3.9 划分 BVH 

任何效率结构（包括 BVH）中最复杂的部分是构建它。我们在构造函数中进行这项工作。关于 BVH 的一个很酷的事情是，只要 `bvh_node` 中的对象列表被划分为两个子列表，命中函数就会工作。如果划分得好，以便两个子节点的包围盒比其父节点的包围盒小，那么效果最好，但这是为了速度而非正确性。我将选择适中的方案，在每个节点沿一个轴划分列表。为了简单起见，我会：

1. 随机选择一个轴 
2. 对基元进行排序（使用 `std::sort`） 
3. 将一半放入每个子树中

当传入的列表有两个元素时，我将每个子树中放入一个元素并结束递归。遍历算法应该流畅，不必检查空指针，所以如果我只有一个元素，我会在每个子树中复制它。显式检查三个元素并只遵循一个递归可能会有所帮助，但我觉得整个方法稍后会得到优化。以下代码使用了三个我们尚未定义的方法 — `box_x_compare`, `box_y_compare` 和 `box_z_compare`。

```cpp
// bvh.h
#include <algorithm>

class bvh_node : public hittable {
  public:
    ...
    bvh_node(const std::vector<shared_ptr<hittable>>& src_objects, size_t start, size_t end) {
        auto objects = src_objects; // 创建一个可修改的源场景对象数组

        int axis = random_int(0,2);
        auto comparator = (axis == 0) ? box_x_compare
                        : (axis == 1) ? box_y_compare
                                      : box_z_compare;

        size_t object_span = end - start;

        if (object_span == 1) {
            left = right = objects[start];
        } else if (object_span == 2) {
            if (comparator(objects[start], objects[start+1])) {
                left = objects[start];
                right = objects[start+1];
            } else {
                left = objects[start+1];
                right = objects[start];
            }
        } else {
            std::sort(objects.begin() + start, objects.begin() + end, comparator);

            auto mid = start + object_span/2;
            left = make_shared<bvh_node>(objects, start, mid);
            right = make_shared<bvh_node>(objects, mid, end);
        }

        bbox = aabb(left->bounding_box(), right->bounding_box());
    }
    ...
};
```

这使用了一个新函数：`random_int()`：

```cpp
// rtweekend.h
inline int random_int(int min, int max) {
    // 返回 [min,max] 范围内的随机整数。
    return static_cast<int>(random_double(min, max+1));
}
```

检查是否有包围盒的原因是防止你发送像无限平面这样没有边界盒的东西。我们没有任何这样的基元，因此在您添加这样的东西之前不会发生这种情况。

## 3.10 盒子比较函数

现在我们需要实现用于 `std::sort()` 的盒子比较函数。为此，创建一个通用比较器，如果第一个参数小于第二个参数，则在给定附加轴索引参数的情况下返回 true。然后定义使用通用比较函数的特定轴比较函数。

```cpp
// bvh.h
class bvh_node : public hittable {
  ...
  private:
    ...

    static bool box_compare(
        const shared_ptr<hittable> a, const shared_ptr<hittable> b, int axis_index
    ) {
        return a->bounding_box().axis(axis_index).min < b->bounding_box().axis(axis_index).min;
    }

    static bool box_x_compare (const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
        return box_compare(a, b, 0);
    }

    static bool box_y_compare (const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
        return box_compare(a, b, 1);
    }

    static bool box_z_compare (const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
        return box_compare(a, b, 2);
    }
};
```

此时，我们已准备好使用我们的新 BVH 代码。让我们在随机球体场景中使用它。

```cpp
// main.ccx
...

int main() {
    ...

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    world = hittable_list(make_shared<bvh_node>(world));
    camera cam;

    ...
}
```

# 4. 纹理映射

在计算机图形学中，纹理映射是指**将材料效果应用于场景中的对象的过程**。这里的“纹理”(*texture*)指的是效果，而“映射”(*mapping*)是指在数学上将一个空间映射到另一个空间的概念。这种效果可以是任何材料属性：颜色、光泽度、凹凸几何（称为凹凸映射），甚至是材料的存在（用于创建表面的剪切区域）。

最常见的纹理映射类型是将图像映射到对象的表面上，定义对象表面每一点的颜色。在实践中，我们实际上是反向实现这一过程：**给定对象上的某个点，我们会查找由纹理映射定义的颜色**。

首先，我们将使纹理颜色成为程序化的，并将创建一个恒定颜色的纹理映射。大多数程序将恒定的RGB颜色和纹理保存在不同的类中，所以你可以自由地做出不同的选择，但我非常相信这种架构，因为能够将任何颜色变成纹理是很棒的。

为了进行纹理查找，我们需要一个纹理坐标。这个坐标可以通过多种方式定义，我们将在进展中发展这个想法。目前，我们将传入二维纹理坐标。按照惯例，纹理坐标被命名为 $$u$$ 和 $$v$$。对于恒定纹理，每一对 $$(u,v)$$ 坐标都会产生一个恒定的颜色，所以我们实际上可以完全忽略坐标。然而，其他类型的纹理将需要这些坐标，所以我们在方法接口中保留这些坐标。

我们纹理类的主要方法是 `color value(...)` 方法，它根据输入坐标返回纹理颜色。除了取点的纹理坐标 $$u$$ 和 $$v$$ 外，我们还提供了问题点的位置，原因稍后会变得明显。

## 4.1 恒定颜色纹理

```cpp
// texture.h
#ifndef TEXTURE_H
#define TEXTURE_H

#include "rtweekend.h"

class texture {
  public:
    virtual ~texture() = default;

    virtual color value(double u, double v, const point3& p) const = 0;
};

class solid_color : public texture {
  public:
    solid_color(color c) : color_value(c) {}

    solid_color(double red, double green, double blue) : solid_color(color(red,green,blue)) {}

    color value(double u, double v, const point3& p) const override {
        return color_value;
    }

  private:
    color color_value;
};

#endif
```

我们需要更新 `hit_record` 结构以存储光线-对象碰撞点的 $$u,v$$ 表面坐标。

我们还需要计算每种`hittable`类型的给定点的 $$(u,v)$$ 纹理坐标。

```cpp
// hittable.h
class hit_record {
  public:
    vec3 p;
    vec3 normal;
    shared_ptr<material> mat;
    double t;
    double u;
    double v;
    bool front_face;
    ...
```

我们还需要计算每种`hittable`上给定点 $$(u,v)$$ 的纹理坐标。

## 4.2 实体纹理: 棋盘格纹理 

实体（或空间）纹理仅依赖于每个点在三维空间中的位置。你可以将实体纹理视为给空间本身的所有点上色，而不是给该空间中的特定对象上色。因此，对象可以在改变位置时穿过纹理的颜色，尽管通常你会固定对象与实体纹理之间的关系。

为了探索空间纹理，我们将实现一个空间 `checker_texture` 类，该类实现了三维棋盘图案。由于空间纹理函数由空间中给定位置驱动，因此纹理 `value()` 函数忽略 `u` 和 `v` 参数，仅使用 `p` 参数。

为了实现棋盘格图案，我们首先计算输入点的每个分量的地板值。我们可以截断坐标，但这会将值拉向零，从而在零的两侧得到相同的颜色。地板函数总是将值向左移动到整数值（朝向负无穷大）。给定这三个整数结果$$(\lfloor x\rfloor ,\lfloor y\rfloor,\lfloor z\rfloor)$$我们取它们的总和并计算模二的结果，这会得到 0 或 1。**零映射到偶数颜色，一映射到奇数颜色**。

最后，我们向纹理添加一个缩放因子，以便我们能够控制场景中棋盘图案的大小。

```cpp
// texture.h
class checker_texture : public texture {
  public:
    checker_texture(double _scale, shared_ptr<texture> _even, shared_ptr<texture> _odd)
      : inv_scale(1.0 / _scale), even(_even), odd(_odd) {}

    checker_texture(double _scale, color c1, color c2)
      : inv_scale(1.0 / _scale),
        even(make_shared<solid_color>(c1)),
        odd(make_shared<solid_color>(c2))
    {}

    color value(double u, double v, const point3& p) const override {
        auto xInteger = static_cast<int>(std::floor(inv_scale * p.x()));
        auto yInteger = static_cast<int>(std::floor(inv_scale * p.y()));
        auto zInteger = static_cast<int>(std::floor(inv_scale * p.z()));

        bool isEven = (xInteger + yInteger + zInteger) % 2 == 0;

        return isEven ? even->value(u, v, p) : odd->value(u, v, p);
    }

  private:
    double inv_scale;
    shared_ptr<texture> even;
    shared_ptr<texture> odd;
};
```

这些棋盘奇偶参数可以指向恒定纹理或某些其他程序化纹理。这符合 Pat Hanrahan 在 1980 年代引入的着色器网络的精神。

如果我们将其添加到我们的 `random_scene()` 函数的基础球体中：

> 译者注：这里代码有问题，`lambertian`那边还没修改，直接加这段是不能成功编译的，可以等4.4全部写完了再编译测试。

```cpp
// main.cc
...
#include "texture.h"


void random_spheres() {
    hittable_list world;

    auto checker = make_shared<checker_texture>(0.32, color(.2, .3, .1), color(.9, .9, .9));
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(checker)));

    for (int a = -11; a < 11; a++) {
    ...
}
...
```

我们得到：

![*棋盘格地面上的球体*](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.02-checker-ground.png)

## 4.3 渲染实体棋盘纹理 

我们将在我们的程序中添加第二个场景，并在本书进展过程中继续添加更多场景。为了帮助这一点，我们将设置一个 `switch` 语句来选择给定运行的所需场景。这是一种粗糙的方法，但我们试图保持事情非常简单，并专注于光线追踪。在你自己的光线追踪器中，你可能希望使用不同的方法，例如支持命令行参数。

这是我们的 `main.cc` 在重构为我们单个随机球体场景后的样子。将 `main()` 重命名为 `random_spheres()`，并添加一个新的 `main()` 函数来调用它：

```cpp
// main.cc
#include "rtweekend.h"

#include "camera.h"
#include "color.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"


void random_spheres() {
    hittable_list world;

    auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, ground_material));

    ...

    cam.render(world);
}

int main() {
    random_spheres();
}
```

现在添加一个场景，其中有两个棋盘球体，一个在另一个上面。

```cpp
// main.cc
#include "rtweekend.h"

#include "camera.h"
#include "color.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"


void random_spheres() {
    ...
}

void two_spheres() {
    hittable_list world;

    auto checker = make_shared<checker_texture>(0.8, color(.2, .3, .1), color(.9, .9, .9));

    world.add(make_shared<sphere>(point3(0,-10, 0), 10, make_shared<lambertian>(checker)));
    world.add(make_shared<sphere>(point3(0, 10, 0), 10, make_shared<lambertian>(checker)));

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;

    cam.vfov     = 20;
    cam.lookfrom = point3(13,2,3);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;

    cam.render(world);
}

int main() {
    switch (2) {
        case 1: random_spheres(); break;
        case 2: two_spheres();    break;
    }
}
```

我们得到这个结果：

![棋盘格球体](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.03-checker-spheres.png)

你可能会认为结果看起来有点奇怪。由于 `checker_texture` 是一个空间纹理，我们实际上是在看球体表面穿过三维棋盘空间。在许多情况下，这是完美的，或者至少足够的。在许多其他情况下，我们真的想在我们的对象表面获得一致的效果。接下来将介绍这种方法。

## 4.4 球体的纹理坐标 

恒定颜色纹理不使用坐标。实体（或空间）纹理使用空间中点的坐标。现在是时候使用 $$u,v$$ 纹理坐标了。这些坐标指定了二维源图像中的位置（或在某些二维参数化空间中）。为此，我们需要一种方法来找到三维对象表面上任何点的 $$u,v$$ 坐标。这种映射是完全任意的，但通常你希望覆盖整个表面，并且能够以某种有意义的方式缩放、定向和拉伸二维图像。我们将从派生出获取球体的 $$u,v$$ 坐标的方案开始。

对于球体，纹理坐标通常基于某种形式的经度和纬度，即球坐标。所以我们计算球坐标中的 $$(\theta,\phi)$$，其中 $$\theta$$ 是从底部极点（即从 $$-Y$$）向上的角度，$$\phi$$ 是绕 $$Y$$ 轴的角度（从 $$-X$$ 到 $$+Z$$ 到 $$+X$$ 到 $$-Z$$ 再回到 $$-X$$）。

我们希望将 $$\theta$$ 和 $$\phi$$ 映射到 $$[0,1]$$ 范围内的纹理坐标 $$u$$ 和 $$v$$，其中 $$(u=0,v=0)$$ 对应于纹理的左下角。因此，从 $$(\theta,\phi)$$ 到 $$(u,v)$$ 的标准化将是：
$$
u=\frac{\phi}{2\pi} \\
v=\frac{\theta}{\pi}
$$
为了计算位于原点中心的单位球面上给定点的 $$\theta$$ 和 $$\phi$$，我们从对应的笛卡尔坐标公式开始：
$$
y=-cos(\theta)\\
x=-cos(\phi)sin(\theta)\\
z=sin(\phi)sin(\theta)
$$
我们需要反转这些方程以求解 $$\theta$$ 和 $$\phi$$。由于方便的 `<cmath>` 函数 `atan2()`，它接受任何与正弦和余弦成比例的一对数字并返回角度，我们可以传入 $$x$$ 和 $$z$$（$$sin(\theta)$$ 抵消）来解算 $$\phi$$：
$$
\phi=atan2(z, -x)
$$
`atan2()` 返回的值范围是 $$-\pi$$ 到 $$\pi$$，但它们从 $$0$$ 到 $$\pi$$，然后翻转到 $$-\pi$$ 并回到 $$0$$。虽然这在数学上是正确的，但我们希望 $$u$$ 的范围是从 $$0$$ 到 $$1$$ 的，而不是从 $$0$$ 到 $$\frac{1}{2}$$ 然后从 $$-\frac{1}{2}$$ 到 $$0$$。幸运的是，
$$
atan2(a, b) = atan2(-a, -b) + \pi
$$
第二个公式产生 $$0$$ 到 $$2\pi$$ 的值，因此，我们可以通过以下方式计算 $$\phi$$：
$$
\phi=atan2(-z, x) + \pi
$$
$$θ$$ 的推导更为直接：
$$
\theta =arccos(-y)
$$
所以对于球体，$$(u, v)$$ 坐标的计算是通过一个实用函数完成的，该函数接受位于原点中心的单位球上的点，并计算 $$u$$ 和 $$v$$：

```cpp
// sphere.h
class sphere : public hittable {
  ...
  private:
    ...
    static void get_sphere_uv(const point3& p, double& u, double& v) {
        // p: 以原点为中心、半径为 1 的球面上的给定点
        // u: 从 X=-1 开始返回的 Y 轴角度值 [0,1]。
        // v: 返回从 Y=-1 到 Y=+1 的角度值 [0,1]。
        //     <1 0 0> yields <0.50 0.50>       <-1  0  0> yields <0.00 0.50>
        //     <0 1 0> yields <0.50 1.00>       < 0 -1  0> yields <0.50 0.00>
        //     <0 0 1> yields <0.25 0.50>       < 0  0 -1> yields <0.75 0.50>

        auto theta = acos(-p.y());
        auto phi = atan2(-p.z(), p.x()) + pi;

        u = phi / (2*pi);
        v = theta / pi;
    }
};
```

更新 `sphere::hit()` 函数，使用此函数更新碰撞记录 UV 坐标。

```cpp
// sphere.h
class sphere : public hittable {
  public:
    ...
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        ...

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);        get_sphere_uv(outward_normal, rec.u, rec.v);        rec.mat = mat;

        return true;
    }
    ...
};
```

现在，我们可以通过将 `const color& a` 替换为纹理指针来制作带有纹理的材料：

```cpp
// material.h
#include "texture.h"
...
class lambertian : public material {
  public:    lambertian(const color& a) : albedo(make_shared<solid_color>(a)) {}
    lambertian(shared_ptr<texture> a) : albedo(a) {}
    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        auto scatter_direction = rec.normal + random_unit_vector();

        // Catch degenerate scatter direction
        if (scatter_direction.near_zero())
            scatter_direction = rec.normal;

        scattered = ray(rec.p, scatter_direction, r_in.time());        attenuation = albedo->value(rec.u, rec.v, rec.p);        return true;
    }

  private:    shared_ptr<texture> albedo;};
```

从碰撞点 $$P$$ 出发，我们计算表面坐标 $$(u, v)$$。然后我们使用这些坐标索引到我们的程序化实体纹理（如大理石）中。我们还可以读入图像，并使用二维 $$(u, v)$$ 纹理坐标索引到图像中。

使用缩放的 $$(u, v)$$ 直接在图像中的一种方法是将 $$u$$ 和 $$v$$ 四舍五入为整数，并将其用作 $$(i, j)$$ 像素。这很尴尬，因为我们不想在改变图像分辨率时必须更改代码。因此，图形学中最普遍的非正式标准之一是使用纹理坐标而不是图像像素坐标。这只是图像中某种形式的分数位置。例如，对于 $$N_x$$ 乘 $$N_y$$ 图像中的像素 $$(i, j)$$，图像纹理位置是：
$$
u = \frac{i}{N_x - 1}\\
v = \frac{j}{N_y - 1}
$$
这只是一个零碎的位置。

## 4.5 访问纹理图像数据

现在是时候创建一个包含图像的纹理类了。我将使用我最喜欢的图像实用工具 [stb_image](https://github.com/nothings/stb)。它将图像数据读入一个大的无符号字符数组中。这些只是打包的 RGB，每个分量范围是 [0,255]（从黑色到全白）。为了让加载图像文件更加容易，我们提供了一个辅助类来管理所有这些 —— `rtw_image`。下面的列表假设你已经将 `stb_image.h` 头文件复制到名为 `external` 的文件夹中。根据你的目录结构进行调整。

```cpp
// rtw_stb_image.h
#ifndef RTW_STB_IMAGE_H
#define RTW_STB_IMAGE_H

// Disable strict warnings for this header from the Microsoft Visual C++ compiler.
#ifdef _MSC_VER
    #pragma warning (push, 0)
#endif

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "external/stb_image.h"

#include <cstdlib>
#include <iostream>

class rtw_image {
  public:
    rtw_image() : data(nullptr) {}

    rtw_image(const char* image_filename) {
        // Loads image data from the specified file. If the RTW_IMAGES environment variable is
        // defined, looks only in that directory for the image file. If the image was not found,
        // searches for the specified image file first from the current directory, then in the
        // images/ subdirectory, then the _parent's_ images/ subdirectory, and then _that_
        // parent, on so on, for six levels up. If the image was not loaded successfully,
        // width() and height() will return 0.

        auto filename = std::string(image_filename);
        auto imagedir = getenv("RTW_IMAGES");

        // Hunt for the image file in some likely locations.
        if (imagedir && load(std::string(imagedir) + "/" + image_filename)) return;
        if (load(filename)) return;
        if (load("images/" + filename)) return;
        if (load("../images/" + filename)) return;
        if (load("../../images/" + filename)) return;
        if (load("../../../images/" + filename)) return;
        if (load("../../../../images/" + filename)) return;
        if (load("../../../../../images/" + filename)) return;
        if (load("../../../../../../images/" + filename)) return;

        std::cerr << "ERROR: Could not load image file '" << image_filename << "'.\n";
    }

    ~rtw_image() { STBI_FREE(data); }

    bool load(const std::string filename) {
        // Loads image data from the given file name. Returns true if the load succeeded.
        auto n = bytes_per_pixel; // Dummy out parameter: original components per pixel
        data = stbi_load(filename.c_str(), &image_width, &image_height, &n, bytes_per_pixel);
        bytes_per_scanline = image_width * bytes_per_pixel;
        return data != nullptr;
    }

    int width()  const { return (data == nullptr) ? 0 : image_width; }
    int height() const { return (data == nullptr) ? 0 : image_height; }

    const unsigned char* pixel_data(int x, int y) const {
        // Return the address of the three bytes of the pixel at x,y (or magenta if no data).
        static unsigned char magenta[] = { 255, 0, 255 };
        if (data == nullptr) return magenta;

        x = clamp(x, 0, image_width);
        y = clamp(y, 0, image_height);

        return data + y*bytes_per_scanline + x*bytes_per_pixel;
    }

  private:
    const int bytes_per_pixel = 3;
    unsigned char *data;
    int image_width, image_height;
    int bytes_per_scanline;

    static int clamp(int x, int low, int high) {
        // Return the value clamped to the range [low, high).
        if (x < low) return low;
        if (x < high) return x;
        return high - 1;
    }
};

// Restore MSVC compiler warnings
#ifdef _MSC_VER
    #pragma warning (pop)
#endif

#endif
```

如果你在 `C` 或 `C++` 以外的语言中编写你的实现，你需要找到（或编写）一个提供类似功能的图像加载库。

`image_texture` 类使用 `rtw_image` 类：

```cpp
// texture.h
#include "rtweekend.h"
#include "rtw_stb_image.h"
#include "perlin.h"

...
class image_texture : public texture {
  public:
    image_texture(const char* filename) : image(filename) {}

    color value(double u, double v, const point3& p) const override {
        // 如果没有纹理数据，则返回纯青色作为调试辅助。
        if (image.height() <= 0) return color(0,1,1);

        // 将输入纹理坐标锁定为 [0,1] x [1,0] 之间 
        u = interval(0,1).clamp(u);
        v = 1.0 - interval(0,1).clamp(v);  // 将 V 翻转到图像坐标

        auto i = static_cast<int>(u * image.width());
        auto j = static_cast<int>(v * image.height());
        auto pixel = image.pixel_data(i,j);

        auto color_scale = 1.0 / 255.0;
        return color(color_scale*pixel[0], color_scale*pixel[1], color_scale*pixel[2]);
    }

  private:
    rtw_image image;
};
```

## 4.6 渲染图像纹理

我刚从网上随机找到一张地球图片 —— 任何标准投影都可以满足我们的需求。

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/earthmap.jpg" alt="earthmap.jpg" />

以下是从文件中读取图像然后将其分配给漫反射材料的代码：

```cpp
// main.cc
void earth() {
    auto earth_texture = make_shared<image_texture>("earthmap.jpg");
    auto earth_surface = make_shared<lambertian>(earth_texture);
    auto globe = make_shared<sphere>(point3(0,0,0), 2, earth_surface);

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;

    cam.vfov     = 20;
    cam.lookfrom = point3(0,0,12);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;

    cam.render(hittable_list(globe));
}

int main() {
    switch (3) {
        case 1:  random_spheres(); break;
        case 2:  two_spheres();    break;
        case 3:  earth();          break;
    }
}
```

我们开始看到所有颜色都是纹理的强大之处 —— 我们可以将任何类型的纹理分配给 `lambertian` 材料，而 `lambertian` 不需要意识到它。

如果照片返回的是中间有一个大的青色球体，那么 `stb_image` 没有找到你的地球地图照片。程序将在可执行文件的同一目录中查找文件。确保将地球图复制到你的构建目录中，或者重写 `earth()` 函数以指向其他地方。

# 5. 柏林噪声

为了获得酷炫的实体纹理，大多数人使用某种形式的柏林噪声。这些噪声以其发明者肯·柏林（*Ken Perlin*）的名字命名。柏林纹理不会像这样返回白噪声：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.06-white-noise.jpg" alt="白噪声" />

相反，它返回类似于模糊白噪声的东西：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.07-white-noise-blurred.jpg" alt="模糊的白噪声" />

柏林噪声的一个关键部分是它是可重复的：它以一个三维点作为输入，并始终返回相同的随机数。附近的点返回相似的数字。柏林噪声的另一个重要部分是它必须简单和快速，因此通常以 *hack* 的方式完成。我将根据 *Andrew Kensler* 的描述逐步构建该 *hack*。

## 5.1 使用随机数字块 

我们可以用三维随机数数组将整个空间铺满，并以块的形式使用它们。你会得到一些明显重复的块状图案：

![平铺随机图案](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.08-tile-random.jpg)

我们不妨使用某种哈希方法来混淆这一点，而不是铺砌。这需要一些辅助代码来实现：

```cpp
// perlin.h
#ifndef PERLIN_H
#define PERLIN_H

#include "rtweekend.h"

class perlin {
  public:
    perlin() {
        ranfloat = new double[point_count];
        for (int i = 0; i < point_count; ++i) {
            ranfloat[i] = random_double();
        }

        perm_x = perlin_generate_perm();
        perm_y = perlin_generate_perm();
        perm_z = perlin_generate_perm();
    }

    ~perlin() {
        delete[] ranfloat;
        delete[] perm_x;
        delete[] perm_y;
        delete[] perm_z;
    }

    double noise(const point3& p) const {
        auto i = static_cast<int>(4*p.x()) & 255;
        auto j = static_cast<int>(4*p.y()) & 255;
        auto k = static_cast<int>(4*p.z()) & 255;

        return ranfloat[perm_x[i] ^ perm_y[j] ^ perm_z[k]];
    }

  private:
    static const int point_count = 256;
    double* ranfloat;
    int* perm_x;
    int* perm_y;
    int* perm_z;

    static int* perlin_generate_perm() {
        auto p = new int[point_count];

        for (int i = 0; i < perlin::point_count; i++)
            p[i] = i;

        permute(p, point_count);

        return p;
    }

    static void permute(int* p, int n) {
        for (int i = n-1; i > 0; i--) {
            int target = random_int(0, i);
            int tmp = p[i];
            p[i] = p[target];
            p[target] = tmp;
        }
    }
};

#endif
```

现在，如果我们创建一个实际的纹理，它将这些介于 0 和 1 之间的浮点数转化为灰色：

```cpp
// texture.h
#include "perlin.h"

class noise_texture : public texture {
  public:
    noise_texture() {}

    color value(double u, double v, const point3& p) const override {
        return color(1,1,1) * noise.noise(p);
    }

  private:
    perlin noise;
};
```

我们可以在一些球体上使用这种纹理：

```cpp
// main.cc
void two_perlin_spheres() {
    hittable_list world;

    auto pertext = make_shared<noise_texture>();
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(pertext)));
    world.add(make_shared<sphere>(point3(0,2,0), 2, make_shared<lambertian>(pertext)));

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;

    cam.vfov     = 20;
    cam.lookfrom = point3(13,2,3);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;

    cam.render(world);
}
int main() {    
    switch (4) {        
        case 1:  random_spheres();     break;
        case 2:  two_spheres();        break;
        case 3:  earth();              break;        
        case 4:  two_perlin_spheres(); break;    
    x}
}
```

添加哈希确实如我们所愿地混乱了图案：

![散列随机纹理](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.09-hash-random.png)

## 5.2 平滑处理结果 

为了使其平滑，我们可以进行线性插值：

```cpp
// perlin.h
class perlin {
  public:
    ...
    double noise(const point3& p) const {        auto u = p.x() - floor(p.x());
        auto v = p.y() - floor(p.y());
        auto w = p.z() - floor(p.z());

        auto i = static_cast<int>(floor(p.x()));
        auto j = static_cast<int>(floor(p.y()));
        auto k = static_cast<int>(floor(p.z()));
        double c[2][2][2];

        for (int di=0; di < 2; di++)
            for (int dj=0; dj < 2; dj++)
                for (int dk=0; dk < 2; dk++)
                    c[di][dj][dk] = ranfloat[
                        perm_x[(i+di) & 255] ^
                        perm_y[(j+dj) & 255] ^
                        perm_z[(k+dk) & 255]
                    ];

        return trilinear_interp(c, u, v, w);    }
    ...

  private:
    ...    static double trilinear_interp(double c[2][2][2], double u, double v, double w) {
        auto accum = 0.0;
        for (int i=0; i < 2; i++)
            for (int j=0; j < 2; j++)
                for (int k=0; k < 2; k++)
                    accum += (i*u + (1-i)*(1-u))*
                            (j*v + (1-j)*(1-v))*
                            (k*w + (1-k)*(1-w))*c[i][j][k];

        return accum;
    }};
```

我们得到：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.10-perlin-trilerp.png" alt="*采用三线性插值的 Perlin 纹理*" />

## 5.3 使用埃尔米特平滑改进 

平滑处理后，效果有所改善，但仍有明显的网格特征。其中有些是马赫带，这是已知的线性插值色彩的感知假象。一个标准的技巧是使用埃尔米特(*Hermite*)三次方对插值进行舍入：

```cpp
// perlin.h
class perlin (
  public:
    ...
    double noise(const point3& p) const {
        auto u = p.x() - floor(p.x());
        auto v = p.y() - floor(p.y());
        auto w = p.z() - floor(p.z());
        u = u*u*(3-2*u);
        v = v*v*(3-2*v);
        w = w*w*(3-2*w);
        auto i = static_cast<int>(floor(p.x()));
        auto j = static_cast<int>(floor(p.y()));
        auto k = static_cast<int>(floor(p.z()));
        ...
```

这会产生更平滑的图像:

![*Perlin 纹理，三线性插值，平滑*](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.11-perlin-trilerp-smooth.png)

## 5.4 调整频率 

它的频率也有点低。我们可以缩放输入点以使其变化更快：

```cpp
// texture.h
class noise_texture : public texture {
  public:
    noise_texture() {}

    noise_texture(double sc) : scale(sc) {}
    
    color value(double u, double v, const point3& p) const override {        
        return color(1,1,1) * noise.noise(scale * p);    
    }

  private:
    perlin noise;    
    double scale;
};
```

然后我们将该缩放添加到 `two_perlin_spheres()` 场景描述中：

```cpp
// main.cc
void two_perlin_spheres() {
    ...
    auto pertext = make_shared<noise_texture>(4);
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(pertext)));
    world.add(make_shared<sphere>(point3(0, 2, 0), 2, make_shared<lambertian>(pertext)));

    camera cam;
    ...
}
```

![*Perlin 纹理，频率较高*](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.12-perlin-hifreq.png)

## 5.5 在晶格点上使用随机向量 

这看起来仍然有点块状，可能是因为图案的最小值和最大值总是精确地落在整数 x/y/z 上。*Ken Perlin*的非常巧妙的技巧是改为在晶格点上放置随机单位向量（而不仅仅是浮点数），并使用点积使最小值和最大值偏离晶格。因此，首先我们需要将随机浮点数更改为随机向量。这些向量是任何合理的不规则方向集，我不会费心让它们完全均匀：

```cpp
// perlin.h
class perlin {
  public:
    perlin() {
        ranvec = new vec3[point_count];
        for (int i = 0; i < point_count; ++i) {
            ranvec[i] = unit_vector(vec3::random(-1,1));
        }

        perm_x = perlin_generate_perm();
        perm_y = perlin_generate_perm();
        perm_z = perlin_generate_perm();
    }

    ~perlin() {
        delete[] ranvec;
        delete[] perm_x;
        delete[] perm_y;
        delete[] perm_z;
    }
    ...

  private:
    static const int point_count = 256;
    vec3* ranvec;
    int* perm_x;
    int* perm_y;
    int* perm_z;
    ...
};
```

Perlin类的 `noise()` 方法现在是：

```cpp
// perlin.h
class perlin {
  public:
    ...
    double noise(const point3& p) const {
        auto u = p.x() - floor(p.x());
        auto v = p.y() - floor(p.y());
        auto w = p.z() - floor(p.z());
        auto i = static_cast<int>(floor(p.x()));
        auto j = static_cast<int>(floor(p.y()));
        auto k = static_cast<int>(floor(p.z()));
        vec3 c[2][2][2];

        for (int di=0; di < 2; di++)
            for (int dj=0; dj < 2; dj++)
                for (int dk=0; dk < 2; dk++)
                    c[di][dj][dk] = ranvec[
                        perm_x[(i+di) & 255] ^
                        perm_y[(j+dj) & 255] ^
                        perm_z[(k+dk) & 255]
                    ];

        return perlin_interp(c, u, v, w);
    }
    ...
};
```

插值变得更复杂一些：

```cpp
// perlin.h
class perlin {
  ...
  private:
    ...
    static double perlin_interp(vec3 c[2][2][2], double u, double v, double w) {
        auto uu = u*u*(3-2*u);
        auto vv = v*v*(3-2*v);
        auto ww = w*w*(3-2*w);
        auto accum = 0.0;

        for (int i=0; i < 2; i++)
            for (int j=0; j < 2; j++)
                for (int k=0; k < 2; k++) {
                    vec3 weight_v(u-i, v-j, w-k);
                    accum += (i*uu + (1-i)*(1-uu))
                           * (j*vv + (1-j)*(1-vv))
                           * (k*ww + (1-k)*(1-ww))
                           * dot(c[i][j][k], weight_v);
                }

        return accum;
    }
    ...
};
```

*Perlin*解释的输出可能返回负值。这些负值将传递给我们的`gamma`函数的 `sqrt()` 函数，并转换为 `NaN`。我们将*perlin*输出的结果重新映射到 0 和 1 之间。

```cpp
// texture.h
class noise_texture : public texture {
  public:
    noise_texture() {}

    noise_texture(double sc) : scale(sc) {}

    color value(double u, double v, const point3& p) const override {
        return color(1,1,1) * 0.5 * (1.0 + noise.noise(scale * p));
    }

  private:
    perlin noise;
    double scale;
};
```

这最终给出了更合理的外观：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.13-perlin-shift.png" alt="*Perlin 纹理，偏移整数值*" />

## 5.6 引入湍流 

通常会使用具有多个叠加频率的复合噪声。这通常称为湍流(*turbulence*)，是对噪声的多次调用的总和：

```cpp
// perlin.h
class perlin {
  ...
  public:
    ...
    double turb(const point3& p, int depth=7) const {
        auto accum = 0.0;
        auto temp_p = p;
        auto weight = 1.0;

        for (int i = 0; i < depth; i++) {
            accum += weight*noise(temp_p);
            weight *= 0.5;
            temp_p *= 2;
        }

        return fabs(accum);
    }
    ...
```

这里 `fabs()` 是在 `<cmath>` 中定义的绝对值函数。

```cpp
class noise_texture : public texture {
  public:
    noise_texture() {}

    noise_texture(double sc) : scale(sc) {}

    color value(double u, double v, const point3& p) const override {
        auto s = scale * p;
        return color(1,1,1) * noise.turb(s);
    }

  private:
    perlin noise;
    double scale;
};
```

直接使用时，湍流会产生一种类似伪装网的外观：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.14-perlin-turb.png" alt="*带有湍流的柏林纹理*" />

## 5.7 调整相位 

然而，通常湍流是间接使用的。例如，程序化实体纹理的`hello world`是一个简单的大理石状纹理。基本思想是使颜色与类似正弦函数的东西成比例，并使用湍流来调整相位（即在 $$sin(x)$$ 中移动 $$x$$ ），这使条纹呈波浪状。注释掉直接的噪声和湍流，给出一个类似大理石的效果是：

```cpp
// texture.h
class noise_texture : public texture {
  public:
    noise_texture() {}

    noise_texture(double sc) : scale(sc) {}

    color value(double u, double v, const point3& p) const override {
        auto s = scale * p;
        return color(1,1,1) * 0.5 * (1 + sin(s.z() + 10*noise.turb(s)));
    }

  private:
    perlin noise;
    double scale;
};
```

产生以下结果：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.15-perlin-marble.png" alt="*Perlin 噪声，大理石纹理*" />

# 6. 四边形

在这系列的三本书中，我们已经用球体作为唯一的几何原始形状走过了一半以上的路程。现在是时候添加我们的第二种原始形状：四边形了。

## 6.1 定义四边形 

虽然我们将新的原始形状命名为四边形，但从技术上讲，它实际上是一个平行四边形（对边平行），而不是一般的四边形。为了我们的目的，我们将使用三个几何实体来定义一个四边形：

1. $$Q$$，左下角。 
2. $$u$$，代表第一边的向量。$$Q + u$$给出与$$Q$$相邻的一个角。 
3. $$v$$，代表第二边的向量。$$Q+v$$给出与$$Q$$相邻的另一个角。 

四边形与$$Q$$对角的角由$$Q+u+v$$给出。这些值是三维的，尽管四边形本身是二维对象。例如，一个角在原点并在$$Z$$方向延伸两个单位，在$$Y$$方向延伸一个单位的四边形将具有值$$Q=(0,0,0)$$,$$u=(0,0,2)$$,$$v=(0,1,0)$$。

下图说明了四边形的组成部分。

![*四边形组件*](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-2.05-quad-def.jpg)

四边形是平面的，所以如果四边形位于$$XY$$、$$YZ$$或$$ZX$$平面中，它的轴对齐包围盒将在一个维度上厚度为零。这可能导致与光线相交的数值问题，但我们可以通过填充任何维度为零的包围盒来解决这个问题。填充是可以的，因为我们没有改变四边形的交点；我们只是扩大其包围盒以消除数值问题的可能性，而且边界只是对实际形状的粗略近似。为此，我们添加了一个新的 `aabb::pad()` 方法来解决这个问题：

```cpp
// aabb.h
...
class aabb {
  public:
    ...
    aabb(const aabb& box0, const aabb& box1) {
        x = interval(box0.x, box1.x);
        y = interval(box0.y, box1.y);
        z = interval(box0.z, box1.z);
    }

    aabb pad() {
        // 返回一个 AABB，该 AABB 的任何一边都不比某个 delta 值窄，必要时进行填充。
        double delta = 0.0001;
        interval new_x = (x.size() >= delta) ? x : x.expand(delta);
        interval new_y = (y.size() >= delta) ? y : y.expand(delta);
        interval new_z = (z.size() >= delta) ? z : z.expand(delta);

        return aabb(new_x, new_y, new_z);
    }
```

现在我们准备好了新的四边形类的第一个草图：

```cpp
// quad.h
#ifndef QUAD_H
#define QUAD_H

#include "rtweekend.h"

#include "hittable.h"

class quad : public hittable {
  public:
    quad(const point3& _Q, const vec3& _u, const vec3& _v, shared_ptr<material> m)
      : Q(_Q), u(_u), v(_v), mat(m)
    {
        set_bounding_box();
    }

    virtual void set_bounding_box() {
        bbox = aabb(Q, Q + u + v).pad();
    }

    aabb bounding_box() const override { return bbox; }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        return false; // To be implemented
    }

  private:
    point3 Q;
    vec3 u, v;
    shared_ptr<material> mat;
    aabb bbox;
};

#endif
```

## 6.2 光线-平面相交 

正如你在之前的列表中看到的，`quad::hit()` 还有待实现。就像对于球体一样，我们需要确定给定光线是否与原始形状相交，如果相交，这种交点的各种属性（命中点、法线、纹理坐标等）。

光线与四边形的相交将通过三个步骤确定：

1. 找到包含该四边形的平面， 
2. 解决光线与包含四边形的平面的交点问题， 
3. 确定击中点是否在四边形内。 

我们将首先解决第二个步骤，解决一般的光线-平面相交问题。

球体通常是第一个学到的光线追踪原始形状，因为它们的隐式公式使得解决光线交点非常容易。与球体一样，平面也有一个隐式公式，我们可以使用它们的隐式公式来产生一个解决光线-平面相交的算法。实际上，光线-平面相交比光线-球体相交还要容易解决。

你可能已经知道这个平面的隐式公式：
$$
Ax+By+Cz+D=0
$$
其中$$A,B,C,D$$是常数，而$$x,y,z$$是任何位于平面上的点$$(x,y,z)$$的值。因此，平面就是所有满足上述公式的点$$(x,y,z)$$的集合。稍微简化一下使用这个备选公式：
$$
Ax+By+Cz=D
$$
（我们没有改变D的符号，因为它只是我们稍后会弄清楚的某个常数。）

这是一个直观的方式来思考这个公式：给定垂直于法向量$$n=(A,B,C)$$的平面，以及位置向量$$v=(x,y,z)$$（即从原点到平面上任何一点的向量），那么我们可以使用点积来求解$$D$$：
$$
n\cdot v = D
$$
对于平面上的任何位置。这是上面给出的$$Ax+By+Cz=D$$的等效公式，只不过现在用向量表示。

现在要找到与某个光线$$R(t)=P+td$$的交点。把光线方程代入，我们得到


$$
n \cdot (P+td) = D
$$
求解$$t$$:
$$
n \cdot P + n\cdot td = D\\
n \cdot P + t(n\cdot d) = D\\
t = \frac{D - n \cdot P}{n \cdot d}
$$
这就给出了$$t$$的值，我们可以将其代入光线方程来找到交点。请注意，如果光线与平面平行，分母$$n\cdot d$$将为零。在这种情况下，我们可以立即记录光线与平面之间的未命中。对于其他原始图形，如果光线的$$t$$参数小于最小可接受值，我们也会记录未命中。

好了，我们可以找到光线与包含给定四边形的平面之间的交点。事实上，我们可以使用这种方法来测试任何平面原始图形，比如三角形和圆盘（稍后再谈）。

## 6.3 寻找包含给定四边形的平面 

我们已经解决了上面的第二步：解决光线-平面相交问题，假设我们有平面方程。为此，我们需要处理第一步：找到包含四边形的平面的方程。我们有四边形参数$$Q$$、$$u$$和$$v$$，想要得到由这三个值定义的四边形包含的平面的相应方程。

幸运的是，这很简单。回想一下，在方程$$Ax+By+Cz=D$$中，$$(A,B,C)$$代表法向量。要得到这个，我们只需使用两个边向量$$u$$和$$v$$的叉积：
$$
n=unit\_vector(u\times v)
$$
平面被定义为满足方程$$Ax+By+Cz=D$$的所有点$$(x,y,z)$$。好了，我们知道$$Q$$在平面上，所以这足以求出$$D$$：
$$
\begin{aligned}
D&=𝑛_𝑥𝑄_𝑥+𝑛_𝑦𝑄_𝑦+𝑛_𝑧𝑄_𝑧\\
&=n⋅Q
\end{aligned}
$$
将平面值添加到四边形类中：

```cpp
// quad.h
class quad : public hittable {
  public:
    quad(const point3& _Q, const vec3& _u, const vec3& _v, shared_ptr<material> m)
      : Q(_Q), u(_u), v(_v), mat(m)
    {
        auto n = cross(u, v);
        normal = unit_vector(n);
        D = dot(normal, Q);

        set_bounding_box();
    }
    ...

  private:
    point3 Q;
    vec3 u, v;
    shared_ptr<material> mat;
    aabb bbox;
    vec3 normal;
    double D;
};
```

我们将使用`normal`和$$D$$两个值来找到给定射线与包含四边形的平面之间的交点。

作为过渡，让我们实现`hit()`方法来处理包含四边形的无限平面。

```cpp
// quad.h
class quad : public hittable {
    ...
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        auto denom = dot(normal, r.direction());

        // 如果光线与平面平行，则不会命中。
        if (fabs(denom) < 1e-8)
            return false;

        // 如果命中点参数 t 位于光线区间之外，则返回 false。
        auto t = (D - dot(normal, r.origin())) / denom;
        if (!ray_t.contains(t))
            return false;

        auto intersection = r.at(t);

        rec.t = t;
        rec.p = intersection;
        rec.mat = mat;
        rec.set_face_normal(r, normal);

        return true;
    }
    ...
```

## 6.4 在平面上定位点

在这个阶段，交点位于包含四边形的平面上，但它可能位于平面的任何地方：光线-平面交点可能在四边形内部或外部。我们需要测试位于四边形内部的交点（命中），并排除位于外部的点（未命中）。为了确定一个点相对于四边形的位置，并为交点分配纹理坐标，我们需要在平面上定位交点。

为此，我们将为平面构建一个坐标框架——一种定位平面上任何点的方式。我们已经在使用我们的三维空间的坐标框架——这是由原点$$O$$和三个基向量$$x$$、$$y$$和$$z$$定义的。

由于平面是一个二维结构，我们只需要一个平面原点$$Q$$和两个基向量：$$u$$和$$v$$。通常，轴是彼此垂直的。然而，为了覆盖整个空间，这并不需要这样——你只需要两个不平行的轴。

![光线平面相交](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-2.06-ray-plane.jpg)

考虑上图作为一个例子。光线$$R$$与平面相交，产生交点$$P$$（不要与上面的光线原点$$P$$混淆）。根据平面向量$$u$$和$$v$$的测量，上面例子中的交点$$P$$位于$$Q+(1)u+(\frac{1}{2})v$$。换句话说，交点$$P$$的$$UV$$（平面）坐标是$$(1,\frac{1}{2})$$。

通常，给定某个任意点$$P$$，我们寻找两个标量值$$\alpha$$和$$\beta$$，使得
$$
P=Q+\alpha  u+\beta v
$$
如果$$u$$和$$v$$保证彼此垂直（在它们之间形成90°角），那么这将是一个简单的问题，只需使用点积将$$P$$投影到每个基向量$$u$$和$$v$$上。然而，由于我们没有限制$$u$$和$$v$$垂直，所以在数学上有点棘手。
$$
P=Q+\alpha u+\beta v \\
p=P−Q=\alpha u+\beta v
$$
在这里，$$P$$是交点，$$p$$是从$$Q$$到$$P$$的向量。

分别与$$u$$和$$v$$交叉上述方程：
$$
\begin{aligned}
\bf u \times p &= \bf u \times (\alpha u + \beta v)\\
&= \bf u \times \alpha u + u \times \beta v\\
&= \bf \alpha(u \times u) + \beta (u \times v)
\end{aligned}
$$

$$
\begin{aligned}
\bf v \times p &= \bf v \times (\alpha u + \beta v)\\
&= \bf v \times \alpha u + v \times \beta v\\
&= \bf \alpha(v \times u) + \beta (v \times v)
\end{aligned}
$$

由于任何向量与其自身的叉积都为零，这些方程简化为
$$
\bf v \times p = \alpha(v\times u)\\
\bf u \times p = \beta (u \times v)
$$
现在求解系数$$\alpha$$和$$\beta$$。如果你是向量数学的新手，你可能会尝试除以$$\bf u \times v$$和$$\bf v \times u$$，但你**不能除以向量**。相反，我们可以对上述方程的两边进行点积运算，将两边都简化为标量，然后进行除法。
$$
\bf n \cdot (v\times p) = n \cdot \alpha(v \times u)\\
\bf n \cdot (u \times p) = n \cdot \beta(u \times v)
$$
现在分离系数就是简单的除法问题：
$$
\bf \alpha = \frac{n\cdot (v\times p)}{n\cdot(v\times u)}\\
\bf \beta = \frac{n\cdot (u\times p)}{n\cdot(u\times v)}
$$
反转$$\alpha$$的分子和分母的叉积（回想一下，$$\bf a \times b = -b \times a$$）给我们两个系数的共同分母：
$$
\bf \alpha = \frac{n\cdot (p\times v)}{n\cdot(u\times v)}\\
\bf \beta = \frac{n\cdot (u\times p)}{n\cdot(u\times v)}
$$
现在我们可以进行最后一次简化，计算一个向量$$\bf w$$，它对于平面的基础框架中的任何平面点$$P$$都是恒定的：
$$
\bf w = \frac{n}{n \cdot (u \times v)}=\frac{n}{n \cdot n} \\
\bf \alpha = w \cdot (p \times v)\\
\bf \beta = w \cdot (u \times p)
$$
向量$$\bf w$$对于给定的四边形是恒定的，所以我们将缓存这个值。

```cpp
// quad.h
class quad : public hittable {
  public:
    quad(const point3& _Q, const vec3& _u, const vec3& _v, shared_ptr<material> m)
      : Q(_Q), u(_u), v(_v), mat(m)
    {
        auto n = cross(u, v);
        normal = unit_vector(n);
        D = dot(normal, Q);
        w = n / dot(n,n);

        set_bounding_box();
    }
    ...

  private:
    point3 Q;
    vec3 u, v;
    shared_ptr<material> mat;
    aabb bbox;
    vec3 normal;
    double D;
    vec3 w;
};
```

## 6.5 使用UV坐标对交点进行内部测试 

现在我们已经得到了交点的平面坐标$$\alpha$$和$$\beta$$，我们可以轻松地使用这些坐标来确定交点是否在四边形内部——也就是说，光线是否实际击中了四边形。

平面被划分为坐标区域，如下所示：

![四边形坐标](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-2.07-quad-coords.jpg)

因此，要查看具有平面坐标$$(\alpha, \beta)$$的点是否位于四边形内部，它只需要满足以下标准：

1. $$0 \leq \alpha \leq 1 $$
2. $$0 \leq \beta \leq 1 $$

这是实现四边形原始形状所需的最后一部分。

在这里稍作停顿，考虑一下，如果你使用$$(\alpha, \beta)$$坐标来确定一个点是否位于四边形（平行四边形）内部，不难想象使用这些相同的二维坐标来确定交点是否位于任何其他二维（平面）原始形状内部！

我们将把这些额外的二维形状作为练习题留给读者，这取决于你的探索欲望。可以考虑三角形、圆盘和圆环（所有这些都非常容易）。你甚至可以根据纹理贴图的像素或 *Mandelbrot* 形状创建剪切模板！

为了使这种实验更容易一些，我们将从`hit`方法中分离出$(\alpha,\beta)$内部测试方法。

```cpp
//quad.h
...
#include <cmath>

class quad : public hittable {
  public:
    ...

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        auto denom = dot(normal, r.direction());

        // No hit if the ray is parallel to the plane.
        if (fabs(denom) < 1e-8)
            return false;

        // Return false if the hit point parameter t is outside the ray interval.
        auto t = (D - dot(normal, r.origin())) / denom;
        if (!ray_t.contains(t))
            return false;

        // Determine the hit point lies within the planar shape using its plane coordinates.
        auto intersection = r.at(t);
        vec3 planar_hitpt_vector = intersection - Q;
        auto alpha = dot(w, cross(planar_hitpt_vector, v));
        auto beta = dot(w, cross(u, planar_hitpt_vector));

        if (!is_interior(alpha, beta, rec))
            return false;

        // Ray hits the 2D shape; set the rest of the hit record and return true.
        rec.t = t;
        rec.p = intersection;
        rec.mat = mat;
        rec.set_face_normal(r, normal);

        return true;
    }

    virtual bool is_interior(double a, double b, hit_record& rec) const {
        // Given the hit point in plane coordinates, return false if it is outside the
        // primitive, otherwise set the hit record UV coordinates and return true.

        if ((a < 0) || (1 < a) || (b < 0) || (1 < b))
            return false;

        rec.u = a;
        rec.v = b;
        return true;
    }
    ...
};

#endif
```

现在我们添加一个新的场景来展示我们的新四边形原始形状：

```cpp
// main.cpp
...
#include "quad.h"
...
void quads() {
    hittable_list world;

    // Materials
    auto left_red     = make_shared<lambertian>(color(1.0, 0.2, 0.2));
    auto back_green   = make_shared<lambertian>(color(0.2, 1.0, 0.2));
    auto right_blue   = make_shared<lambertian>(color(0.2, 0.2, 1.0));
    auto upper_orange = make_shared<lambertian>(color(1.0, 0.5, 0.0));
    auto lower_teal   = make_shared<lambertian>(color(0.2, 0.8, 0.8));

    // Quads
    world.add(make_shared<quad>(point3(-3,-2, 5), vec3(0, 0,-4), vec3(0, 4, 0), left_red));
    world.add(make_shared<quad>(point3(-2,-2, 0), vec3(4, 0, 0), vec3(0, 4, 0), back_green));
    world.add(make_shared<quad>(point3( 3,-2, 1), vec3(0, 0, 4), vec3(0, 4, 0), right_blue));
    world.add(make_shared<quad>(point3(-2, 3, 1), vec3(4, 0, 0), vec3(0, 0, 4), upper_orange));
    world.add(make_shared<quad>(point3(-2,-3, 5), vec3(4, 0, 0), vec3(0, 0,-4), lower_teal));

    camera cam;

    cam.aspect_ratio      = 1.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;

    cam.vfov     = 80;
    cam.lookfrom = point3(0,0,9);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;

    cam.render(world);
}

int main() {
    switch (5) {
        case 1:  random_spheres();     break;
        case 2:  two_spheres();        break;
        case 3:  earth();              break;
        case 4:  two_perlin_spheres(); break;
        case 5:  quads();              break;
    }
}
```

![四边形](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.16-quads.png)

# 7. 光照

光照是光线追踪的关键组成部分。早期简单的光线追踪器使用抽象光源，比如空间中的点或方向。现代方法有更多基于物理的光源，它们具有位置和大小。要创建这样的光源，我们需要能够将任何普通对象转变成能够向我们的场景发射光的东西。

## 7.1 自发光材料 

首先，让我们制作一个发光材料。我们需要添加一个发射函数（我们也可以将其添加到 `hit_record` 中————这是设计品味的问题）。像背景一样，它只告诉光线它是什么颜色，并且不进行反射。这非常简单：

```cpp
// material.h
class diffuse_light : public material {
  public:
    diffuse_light(shared_ptr<texture> a) : emit(a) {}
    diffuse_light(color c) : emit(make_shared<solid_color>(c)) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        return false;
    }

    color emitted(double u, double v, const point3& p) const override {
        return emit->value(u, v, p);
    }

  private:
    shared_ptr<texture> emit;
};
```

为了避免让所有非自发光材料实现 `emitted()`，我让基类返回黑色：

```cpp
// material.h
class material {
  public:
    ...

    virtual color emitted(double u, double v, const point3& p) const {
        return color(0,0,0);
    }

    virtual bool scatter(
        const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
    ) const = 0;
};
```

## 7.2 向射线颜色函数中添加背景色 

接下来，我们希望有一个纯黑色的背景，这样场景中的唯一光源就来自发光体。为此，我们将向 `ray_color` 函数中添加一个背景色参数，并注意新的 `color_from_emission` 值。

```cpp
// camera.h
class camera {
  public:
    double aspect_ratio      = 1.0;  // Ratio of image width over height
    int    image_width       = 100;  // Rendered image width in pixel count
    int    samples_per_pixel = 10;   // Count of random samples for each pixel
    int    max_depth         = 10;   // Maximum number of ray bounces into scene
    color  background;               // Scene background color

    ...

  private:
    ...
    color ray_color(const ray& r, int depth, const hittable& world) const {
        hit_record rec;

        // 如果我们超过了光线反弹限制，则不会收集更多的光线。
        if (depth <= 0)
            return color(0,0,0);

        // 如果光线没有击中任何物体，则返回背景颜色。
        if (!world.hit(r, interval(0.001, infinity), rec))
            return background;

        ray scattered;
        color attenuation;
        color color_from_emission = rec.mat->emitted(rec.u, rec.v, rec.p);

        if (!rec.mat->scatter(r, rec, attenuation, scattered))
            return color_from_emission;

        color color_from_scatter = attenuation * ray_color(scattered, depth-1, world);

        return color_from_emission + color_from_scatter;
    }
};
```

更新 `main()` 为之前的场景设置背景色：

```cpp
// main.cc
void random_spheres() {
    ...
    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = color(0.70, 0.80, 1.00);
    ...
}

void two_spheres() {
    ...
    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = color(0.70, 0.80, 1.00);
    ...
}

void earth() {
    ...
    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = color(0.70, 0.80, 1.00);
    ...
}

void two_perlin_spheres() {
    ...
    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = color(0.70, 0.80, 1.00);
    ...
}

void quads() {
    ...
    camera cam;

    cam.aspect_ratio      = 1.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = color(0.70, 0.80, 1.00);
    ...
}
```

由于我们移除了用于确定光线击中天空时的天空颜色的代码，我们需要为我们旧的场景渲染传入一个新的颜色值。我们选择了整个天空统一的平淡蓝白色。你总是可以传入一个布尔值，以在之前的天空盒代码和新的纯色背景之间切换。我们在这里保持简单。

## 7.3 将对象变成灯光 

如果我们设置一个矩形作为光源：

```cpp
// main.cc
void simple_light() {
    hittable_list world;

    auto pertext = make_shared<noise_texture>(4);
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(pertext)));
    world.add(make_shared<sphere>(point3(0,2,0), 2, make_shared<lambertian>(pertext)));

    auto difflight = make_shared<diffuse_light>(color(4,4,4));
    world.add(make_shared<quad>(point3(3,1,-2), vec3(2,0,0), vec3(0,2,0), difflight));

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = color(0,0,0);

    cam.vfov     = 20;
    cam.lookfrom = point3(26,3,6);
    cam.lookat   = point3(0,2,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;

    cam.render(world);
}

int main() {
    switch (6) {
        case 1:  random_spheres();     break;
        case 2:  two_spheres();        break;
        case 3:  earth();              break;
        case 4:  two_perlin_spheres(); break;
        case 5:  quads();              break;
        case 6:  simple_light();       break;
    }
}
```

我们得到：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.17-rect-light.png" alt="矩形光源场景" />

请注意，光源的亮度超过(1,1,1)。这使它足够亮，可以照亮东西。

尝试让一些球体也成为灯光。

```cpp
// main.cc
void simple_light() {
    ...
    auto difflight = make_shared<diffuse_light>(color(4,4,4));
    world.add(make_shared<sphere>(point3(0,7,0), 2, difflight));
    world.add(make_shared<quad>(point3(3,1,-2), vec3(2,0,0), vec3(0,2,0), difflight));
    ...
}
```

![带有矩形和球形光源的场景](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.18-rect-sphere-light.png)

## 7.4 创建一个空的“*Cornell Box*” 

“*Cornell Box*”于1984年被引入，用于模拟光线在漫反射表面之间的相互作用。让我们制作盒子的5个墙壁和灯光：

```cpp
// main.cc
void cornell_box() {
    hittable_list world;

    auto red   = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light = make_shared<diffuse_light>(color(15, 15, 15));

    world.add(make_shared<quad>(point3(555,0,0), vec3(0,555,0), vec3(0,0,555), green));
    world.add(make_shared<quad>(point3(0,0,0), vec3(0,555,0), vec3(0,0,555), red));
    world.add(make_shared<quad>(point3(343, 554, 332), vec3(-130,0,0), vec3(0,0,-105), light));
    world.add(make_shared<quad>(point3(0,0,0), vec3(555,0,0), vec3(0,0,555), white));
    world.add(make_shared<quad>(point3(555,555,555), vec3(-555,0,0), vec3(0,0,-555), white));
    world.add(make_shared<quad>(point3(0,0,555), vec3(555,0,0), vec3(0,555,0), white));

    camera cam;

    cam.aspect_ratio      = 1.0;
    cam.image_width       = 600;
    cam.samples_per_pixel = 200;
    cam.max_depth         = 50;
    cam.background        = color(0,0,0);

    cam.vfov     = 40;
    cam.lookfrom = point3(278, 278, -800);
    cam.lookat   = point3(278, 278, 0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;

    cam.render(world);
}

int main() {
    switch (7) {
        case 1:  random_spheres();     break;
        case 2:  two_spheres();        break;
        case 3:  earth();              break;
        case 4:  two_perlin_spheres(); break;
        case 5:  quads();              break;
        case 6:  simple_light();       break;
        case 7:  cornell_box();        break;
    }
}
```

我们得到：

![空的Cornell box](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.19-cornell-empty.png)

这幅图像非常嘈杂，因为光源很小。

# 8. 实例

康奈尔盒子通常包含两个相对于墙壁旋转的块体。首先，让我们创建一个返回盒子的函数，通过创建一个由六个矩形组成的可命中列表（`hittable_list`）：

```cpp
// quad.h
...
#include "hittable_list.h"
...
inline shared_ptr<hittable_list> box(const point3& a, const point3& b, shared_ptr<material> mat)
{
    // 返回包含两个相对顶点 a 和 b 的三维方框（六边形）。

    auto sides = make_shared<hittable_list>();

    // 用最小坐标和最大坐标构造两个相对的顶点。
    auto min = point3(fmin(a.x(), b.x()), fmin(a.y(), b.y()), fmin(a.z(), b.z()));
    auto max = point3(fmax(a.x(), b.x()), fmax(a.y(), b.y()), fmax(a.z(), b.z()));

    auto dx = vec3(max.x() - min.x(), 0, 0);
    auto dy = vec3(0, max.y() - min.y(), 0);
    auto dz = vec3(0, 0, max.z() - min.z());

    sides->add(make_shared<quad>(point3(min.x(), min.y(), max.z()),  dx,  dy, mat)); // front
    sides->add(make_shared<quad>(point3(max.x(), min.y(), max.z()), -dz,  dy, mat)); // right
    sides->add(make_shared<quad>(point3(max.x(), min.y(), min.z()), -dx,  dy, mat)); // back
    sides->add(make_shared<quad>(point3(min.x(), min.y(), min.z()),  dz,  dy, mat)); // left
    sides->add(make_shared<quad>(point3(min.x(), max.y(), max.z()),  dx, -dz, mat)); // top
    sides->add(make_shared<quad>(point3(min.x(), min.y(), min.z()),  dx,  dz, mat)); // bottom

    return sides;
}
```

现在我们可以添加两个块体（但未旋转）。

```cpp
// main.cc
void cornell_box() {
    ...
    world.add(make_shared<quad>(point3(555,0,0), vec3(0,555,0), vec3(0,0,555), green));
    world.add(make_shared<quad>(point3(0,0,0), vec3(0,555,0), vec3(0,0,555), red));
    world.add(make_shared<quad>(point3(343, 554, 332), vec3(-130,0,0), vec3(0,0,-105), light));
    world.add(make_shared<quad>(point3(0,0,0), vec3(555,0,0), vec3(0,0,555), white));
    world.add(make_shared<quad>(point3(555,555,555), vec3(-555,0,0), vec3(0,0,-555), white));
    world.add(make_shared<quad>(point3(0,0,555), vec3(555,0,0), vec3(0,555,0), white));

    world.add(box(point3(130, 0, 65), point3(295, 165, 230), white));
    world.add(box(point3(265, 0, 295), point3(430, 330, 460), white));

    camera cam;
    ...
}
```

既然我们有了盒子，我们需要稍微旋转它们，以使其与真实的康奈尔盒子匹配。在光线追踪中，这通常是通过实例来完成的。一个实例是已经放置到场景中的几何原始体的副本。这个实例完全独立于原始体的其他副本，并且可以移动或旋转。在这种情况下，我们的几何原始体是可命中的盒子对象，我们想要旋转它。这在光线追踪中特别容易，因为我们实际上不需要移动场景中的对象；相反，我们以相反的方向移动光线。例如，考虑一个平移（通常称为移动）。我们可以把原点处的粉色盒子的所有x分量加2，或者（正如我们在光线追踪中几乎总是做的）让盒子保持在原位，但在其命中例程中从光线原点的x分量中减去2。

![移动光线与盒子的光线-盒子相交](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-2.08-ray-box.jpg)

## 8.1 实例平移 

您可以将此视为移动或坐标变换，这取决于您。关于此的合理思考方式是考虑将入射光线向后移动偏移量，确定是否发生交叉，然后将交点向前移动偏移量。

我们需要将交点向前移动偏移量，以确保交点实际上在入射光线的路径中。如果我们忘记将交点向前移动，那么交点将在偏移光线的路径中，这是不正确的。让我们添加代码来实现这一点。

```cpp
// hittable.h
class translate : public hittable {
  public:
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // 将光线向后移动偏移量
        ray offset_r(r.origin() - offset, r.direction(), r.time());

        // 确定沿偏移发生相交的位置（如果有）
        if (!object->hit(offset_r, ray_t, rec))
            return false;

        // 将交叉点向前移动偏移量
        rec.p += offset;

        return true;
    }

  private:
    shared_ptr<hittable> object;
    vec3 offset;
};
```

... 然后补充`translate`类的剩余部分：

```cpp
// hittable.h
class translate : public hittable {
  public:
    translate(shared_ptr<hittable> p, const vec3& displacement)
      : object(p), offset(displacement)
    {
        bbox = object->bounding_box() + offset;
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // Move the ray backwards by the offset
        ray offset_r(r.origin() - offset, r.direction(), r.time());

        // Determine where (if any) an intersection occurs along the offset ray
        if (!object->hit(offset_r, ray_t, rec))
            return false;

        // Move the intersection point forwards by the offset
        rec.p += offset;

        return true;
    }

    aabb bounding_box() const override { return bbox; }

  private:
    shared_ptr<hittable> object;
    vec3 offset;
    aabb bbox;
};
```

我们还需要记得偏移边界框，否则入射光线可能会在错误的地方寻找交点并轻易忽略相交的位置。上面的表达式`object->bounding_box() + offset` 需要一些额外的支持。

```cpp
// aabb.h
class aabb {
    ...
};

aabb operator+(const aabb& bbox, const vec3& offset) {
    return aabb(bbox.x + offset.x(), bbox.y + offset.y(), bbox.z + offset.z());
}

aabb operator+(const vec3& offset, const aabb& bbox) {
    return bbox + offset;
}
```

由于每个轴对齐包围盒（aabb）的维度都表示为区间（interval），我们还需要为区间增加一个加法运算符。

```cpp
// interval.h
class interval {
    ...
};

const interval interval::empty    = interval(+infinity, -infinity);
const interval interval::universe = interval(-infinity, +infinity);

interval operator+(const interval& ival, double displacement) {
    return interval(ival.min + displacement, ival.max + displacement);
}

interval operator+(double displacement, const interval& ival) {
    return ival + displacement;
}
```

## 8.2 实例旋转 

旋转并不像平移那样容易理解或生成公式。一种常见的图形学策略是沿 x、y 和 z 轴进行所有旋转。这些旋转在某种意义上是轴对齐的。首先，让我们绕 z 轴旋转 $$\theta$$ 角度。这将仅改变 x 和 y ，并且不依赖于 z。

![绕Z轴旋转](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-2.09-rot-z.jpg)

这涉及到一些基本的三角形相关公式，我在这里不会详细介绍。这会给你留下一个印象，即这是稍微复杂一点的问题，但它是直接的，你可以在任何图形学教科书和许多讲义中找到。绕 z 轴逆时针旋转的结果是：
$$
x^\prime = cos(\theta)\cdot x - sin(\theta)\cdot y \\
y^\prime = sin(\theta)\cdot x + cos(\theta)\cdot y
$$
这个公式对于任何 $$\theta$$ 都适用，并且不需要考虑象限或类似的情况。逆变换则是相反的几何操作：旋转 $$-\theta$$ 角度。这里要记住，$$cos(\theta)=cos(−\theta)$$ 和 $$sin(-\theta)=−sin(\theta)$$，所以公式非常简单。

类似地，绕 y 轴旋转（正如我们希望对盒子中的块体所做的那样）的公式是：
$$
x^\prime = cos(\theta)\cdot x + sin(\theta)\cdot z \\
z^\prime = -sin(\theta)\cdot x + cos(\theta)\cdot z
$$
如果我们想绕 x 轴旋转：
$$
y^\prime = cos(\theta)\cdot y - sin(\theta)\cdot z \\
z^\prime = sin(\theta)\cdot y + cos(\theta)\cdot z
$$
将平移视为初始光线的简单移动是理解发生了什么的好方法。但是，对于像旋转这样的复杂操作，很容易意外地混淆公式（或忘记一个负号），所以最好将旋转视为坐标变换。

上面的 `translate::hit` 函数的伪代码描述了该函数的移动：

1. 将光线向后移动 
2. 确定沿偏移光线是否存在交点（如果有，交点在哪里） 
3. 将交点向前移动

但这也可以理解为坐标变换：

1. 将光线从世界空间改为物体空间 
2. 确定物体空间中是否存在交点（如果有，交点在哪里） 
3. 将交点从物体空间改为世界空间 

旋转物体不仅会改变交点，还会改变表面法线向量，从而改变反射和折射的方向。因此，我们也需要改变法线。幸运的是，法线会与向量类似地旋转，因此我们可以使用上面的相同公式。虽然法线和向量在经历旋转和平移的物体上看起来相同，但在经历缩放的物体上需要特别注意以保持法线与表面垂直。我们在这里不会涉及这一点，但如果你实现了缩放，应该研究表面法线变换。

我们需要从将光线从世界空间改为物体空间开始，这对于旋转意味着以 $$-\theta$$ 角度旋转。
$$
x^\prime = cos(\theta)\cdot x - sin(\theta)\cdot z \\
z^\prime = sin(\theta)\cdot x + cos(\theta)\cdot z
$$
 我们现在可以为 y 轴旋转创建一个类：

```cpp
// hittable.h
class rotate_y : public hittable {
  public:

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // 将光线从世界空间更改为对象空间
        auto origin = r.origin();
        auto direction = r.direction();

        origin[0] = cos_theta*r.origin()[0] - sin_theta*r.origin()[2];
        origin[2] = sin_theta*r.origin()[0] + cos_theta*r.origin()[2];

        direction[0] = cos_theta*r.direction()[0] - sin_theta*r.direction()[2];
        direction[2] = sin_theta*r.direction()[0] + cos_theta*r.direction()[2];

        ray rotated_r(origin, direction, r.time());

        // 确定对象空间中发生相交的位置（如果有）
        if (!object->hit(rotated_r, ray_t, rec))
            return false;

        // 将交点从对象空间更改为世界空间
        auto p = rec.p;
        p[0] =  cos_theta*rec.p[0] + sin_theta*rec.p[2];
        p[2] = -sin_theta*rec.p[0] + cos_theta*rec.p[2];

        // 将法线从对象空间更改为世界空间
        auto normal = rec.normal
        normal[0] =  cos_theta*rec.normal[0] + sin_theta*rec.normal[2];
        normal[2] = -sin_theta*rec.normal[0] + cos_theta*rec.normal[2];

        rec.p = p;
        rec.normal = normal;

        return true;
    }
};
```

... 然后完成该类的其余部分：

```cpp
// hittable.h
class rotate_y : public hittable {
  public:
    rotate_y(shared_ptr<hittable> p, double angle) : object(p) {
        auto radians = degrees_to_radians(angle);
        sin_theta = sin(radians);
        cos_theta = cos(radians);
        bbox = object->bounding_box();

        point3 min( infinity,  infinity,  infinity);
        point3 max(-infinity, -infinity, -infinity);

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                for (int k = 0; k < 2; k++) {
                    auto x = i*bbox.x.max + (1-i)*bbox.x.min;
                    auto y = j*bbox.y.max + (1-j)*bbox.y.min;
                    auto z = k*bbox.z.max + (1-k)*bbox.z.min;

                    auto newx =  cos_theta*x + sin_theta*z;
                    auto newz = -sin_theta*x + cos_theta*z;

                    vec3 tester(newx, y, newz);

                    for (int c = 0; c < 3; c++) {
                        min[c] = fmin(min[c], tester[c]);
                        max[c] = fmax(max[c], tester[c]);
                    }
                }
            }
        }

        bbox = aabb(min, max);
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        ...
    }

    aabb bounding_box() const override { return bbox; }

  private:
    shared_ptr<hittable> object;
    double sin_theta;
    double cos_theta;
    aabb bbox;
};
```

*Cornell Box*的更改如下：

```cpp
// main.cc
void cornell_box() {
    ...
    world.add(make_shared<quad>(point3(0,0,555), vec3(555,0,0), vec3(0,555,0), white));

    shared_ptr<hittable> box1 = box(point3(0,0,0), point3(165,330,165), white);
    box1 = make_shared<rotate_y>(box1, 15);
    box1 = make_shared<translate>(box1, vec3(265,0,295));
    world.add(box1);

    shared_ptr<hittable> box2 = box(point3(0,0,0), point3(165,165,165), white);
    box2 = make_shared<rotate_y>(box2, -18);
    box2 = make_shared<translate>(box2, vec3(130,0,65));
    world.add(box2);

    camera cam;
    ...
}
```

这会得到：

![标准Cornell Box](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.21-cornell-standard.png)

# 9. 体积雾

在光线追踪器中添加烟、雾、薄雾等效果是一种不错的补充。这些有时被称为**体积雾**(*volumes*)或**参与介质**(*participating media*)。另一个值得添加的功能是次表面散射，这有点像物体内部的密集雾气。这通常会给软件架构带来混乱，因为体积雾与表面是不同的东西，但一个巧妙的技巧是将体积雾视为随机表面。大量的烟雾可以被替换为在体积雾的每个点可能存在或可能不存在的表面。当你看到代码时，这会更有意义。

## 9.1 恒定密度介质 

首先，我们从一个恒定密度的体积雾开始。光线穿过这种介质时，可能会在体积内部发生散射，或者像图中的中间光线那样完全穿过。像轻雾这样的较薄、较透明的体积雾，更有可能让光线像中间那样穿过。光线穿过体积雾的距离也决定了光线完全穿过的可能性。

![光线与体积雾的相互作用](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-2.10-ray-vol.jpg)

随着光线穿过体积雾，它可能在任何点发生散射。体积雾越密集，发生这种情况的可能性越大。光线在任何小距离 $$\Delta L$$ 内发生散射的概率是：
$$
probability = C \cdot \Delta L
$$
其中 $$C$$ 与体积雾的光学密度成正比。如果你仔细研究所有的微分方程，对于一个随机数，你会得到散射发生的距离。如果那个距离在体积雾外，则没有“命中”。对于恒定密度的体积雾，我们只需要密度 $$C$$ 和边界。我将使用另一个可碰撞对象作为边界。

结果类是：

```cpp
// constant_medium.h
#ifndef CONSTANT_MEDIUM_H
#define CONSTANT_MEDIUM_H

#include "rtweekend.h"

#include "hittable.h"
#include "material.h"
#include "texture.h"

class constant_medium : public hittable {
  public:
    constant_medium(shared_ptr<hittable> b, double d, shared_ptr<texture> a)
      : boundary(b), neg_inv_density(-1/d), phase_function(make_shared<isotropic>(a))
    {}

    constant_medium(shared_ptr<hittable> b, double d, color c)
      : boundary(b), neg_inv_density(-1/d), phase_function(make_shared<isotropic>(c))
    {}

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // 调试时偶尔打印示例。要启用，请将enableDebug 设置为true。
        const bool enableDebug = false;
        const bool debugging = enableDebug && random_double() < 0.00001;

        hit_record rec1, rec2;

        if (!boundary->hit(r, interval::universe, rec1))
            return false;

        if (!boundary->hit(r, interval(rec1.t+0.0001, infinity), rec2))
            return false;

        if (debugging) std::clog << "\nray_tmin=" << rec1.t << ", ray_tmax=" << rec2.t << '\n';

        if (rec1.t < ray_t.min) rec1.t = ray_t.min;
        if (rec2.t > ray_t.max) rec2.t = ray_t.max;

        if (rec1.t >= rec2.t)
            return false;

        if (rec1.t < 0)
            rec1.t = 0;

        auto ray_length = r.direction().length();
        auto distance_inside_boundary = (rec2.t - rec1.t) * ray_length;
        auto hit_distance = neg_inv_density * log(random_double());

        if (hit_distance > distance_inside_boundary)
            return false;

        rec.t = rec1.t + hit_distance / ray_length;
        rec.p = r.at(rec.t);

        if (debugging) {
            std::clog << "hit_distance = " <<  hit_distance << '\n'
                      << "rec.t = " <<  rec.t << '\n'
                      << "rec.p = " <<  rec.p << '\n';
        }

        rec.normal = vec3(1,0,0);  // 任意
        rec.front_face = true;     // 任意
        rec.mat = phase_function;

        return true;
    }

    aabb bounding_box() const override { return boundary->bounding_box(); }

  private:
    shared_ptr<hittable> boundary;
    double neg_inv_density;
    shared_ptr<material> phase_function;
};

#endif
```

各向同性的散射函数选择一个均匀随机方向：

```cpp
// material.h
class isotropic : public material {
  public:
    isotropic(color c) : albedo(make_shared<solid_color>(c)) {}
    isotropic(shared_ptr<texture> a) : albedo(a) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        scattered = ray(rec.p, random_unit_vector(), r_in.time());
        attenuation = albedo->value(rec.u, rec.v, rec.p);
        return true;
    }

  private:
    shared_ptr<texture> albedo;
};
```

我们必须非常小心地处理边界逻辑，因为我们需要确保这适用于体积雾内部的光线起源点。在云层中，光线会频繁反弹，这是一个常见的情况。

此外，上述代码假设一旦光线离开恒定介质边界，它将永远在边界外继续传播。换句话说，它假设边界形状是凸的。因此，这种特定的实现适用于盒子或球体这样的边界，但不适用于环面或包含空隙的形状。可以编写一个处理任意形状的实现，但我们将把它留作读者的练习。

## 9.2 用烟雾和雾气渲染康奈尔盒子 

如果我们用烟雾和雾气（深色和浅色颗粒）替换两个块，并使光线更大（并调暗，以免过曝）以加快收敛：

```cpp
// mian.cc
#include "constant_medium.h"
...
void cornell_smoke() {
    hittable_list world;

    auto red   = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light = make_shared<diffuse_light>(color(7, 7, 7));

    world.add(make_shared<quad>(point3(555,0,0), vec3(0,555,0), vec3(0,0,555), green));
    world.add(make_shared<quad>(point3(0,0,0), vec3(0,555,0), vec3(0,0,555), red));
    world.add(make_shared<quad>(point3(113,554,127), vec3(330,0,0), vec3(0,0,305), light));
    world.add(make_shared<quad>(point3(0,555,0), vec3(555,0,0), vec3(0,0,555), white));
    world.add(make_shared<quad>(point3(0,0,0), vec3(555,0,0), vec3(0,0,555), white));
    world.add(make_shared<quad>(point3(0,0,555), vec3(555,0,0), vec3(0,555,0), white));

    shared_ptr<hittable> box1 = box(point3(0,0,0), point3(165,330,165), white);
    box1 = make_shared<rotate_y>(box1, 15);
    box1 = make_shared<translate>(box1, vec3(265,0,295));

    shared_ptr<hittable> box2 = box(point3(0,0,0), point3(165,165,165), white);
    box2 = make_shared<rotate_y>(box2, -18);
    box2 = make_shared<translate>(box2, vec3(130,0,65));

    world.add(make_shared<constant_medium>(box1, 0.01, color(0,0,0)));
    world.add(make_shared<constant_medium>(box2, 0.01, color(1,1,1)));

    camera cam;

    cam.aspect_ratio      = 1.0;
    cam.image_width       = 600;
    cam.samples_per_pixel = 200;
    cam.max_depth         = 50;
    cam.background        = color(0,0,0);

    cam.vfov     = 40;
    cam.lookfrom = point3(278, 278, -800);
    cam.lookat   = point3(278, 278, 0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;

    cam.render(world);
}

int main() {
    switch (8) {
        case 1:  random_spheres();     break;
        case 2:  two_spheres();        break;
        case 3:  earth();              break;
        case 4:  two_perlin_spheres(); break;
        case 5:  quads();              break;
        case 6:  simple_light();       break;
        case 7:  cornell_box();        break;
        case 8:  cornell_smoke();      break;
    }
}
```

我们得到：

![康奈尔烟雾箱](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.22-cornell-smoke.png)
