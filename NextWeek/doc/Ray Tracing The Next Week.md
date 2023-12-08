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

![*弹跳球体*](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-2.01-bouncing-spheres.png)