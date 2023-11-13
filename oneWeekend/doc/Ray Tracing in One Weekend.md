> 声明：此文章仅供个人学习，由于英文水平有限，本文为我个人基于翻译插件加以优化所得，会尽可能还原原意。如果各位有更好的建议意见可以在评论区中提出。
# 1. 概述
略。
# 2. 输出一个图像
## 2.1 PPM图像格式
每当启动渲染器时，您都需要一种查看图像的方法。最直接的方法就是将其写入文件。问题是，格式有很多。其中许多都很复杂。我总是从纯文本 ppm 文件开始。这是维基百科上的一个很好的描述：
![img](https://raytracing.github.io/images/fig-1.01-ppm.jpg)
让我们编写一些 C++ 代码来输出这样的内容：

```cpp
// main.cc
#include <iostream>

int main() {

    // Image

    int image_width = 256;
    int image_height = 256;

    // Render

    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = 0; j < image_height; ++j) {
        for (int i = 0; i < image_width; ++i) {
            auto r = double(i) / (image_width-1);
            auto g = double(j) / (image_height-1);
            auto b = 0;

            int ir = static_cast<int>(255.999 * r);
            int ig = static_cast<int>(255.999 * g);
            int ib = static_cast<int>(255.999 * b);

            std::cout << ir << ' ' << ig << ' ' << ib << '\n';
        }
    }
}
```
这段代码中有一些地方需要注意：
1. 像素按行写出。
2. 每行像素都是从左到右写出的。
3. 这些行是从上到下写出的。
4. 按照惯例，每个红/绿/蓝分量在内部由范围从 0.0 到 1.0 的实值变量表示。在打印之前，必须将它们缩放为 0 到 255 之间的整数值。
5. 红色从左到右从完全关闭（黑色）到完全打开（亮红色），绿色从顶部完全关闭到底部变为黑色。将红光和绿光加在一起会产生黄色，因此我们应该期望右下角是黄色的。

## 2.2 创建图像文件
由于该文件被写入标准输出流，因此您需要将其重定向到图像文件。通常，这是通过使用 > 重定向运算符从命令行完成的，如下所示：
```shell
build\Release\inOneWeekend.exe > image.ppm
```
（此示例假设您正在使用 CMake 进行构建，使用与所包含源中的 CMakeLists.txt 文件相同的方法。使用您喜欢的任何构建环境和语言。）
这就是使用 CMake 在 Windows 上的情况。在 Mac 或 Linux 上，它可能如下所示：
```shell
build/inOneWeekend > image.ppm
```
打开输出文件（在Mac 上的 ToyViewer 中，但如果您的查看器不支持，请在您最喜欢的图像查看器或者 Google搜索“ppm viewer”中尝试）显示以下结果：
![img](https://raytracing.github.io/images/img-1.01-first-ppm-image.png)
好极了！这就是图形学中的“hello world”。如果您的图像看起来不是这样，请在文本编辑器中打开输出文件，看看它是什么样子。它应该这样开始：

```txt
// image.ppm
P3
256 256
255
0 0 0
1 0 0
2 0 0
3 0 0
4 0 0
5 0 0
6 0 0
7 0 0
8 0 0
9 0 0
10 0 0
11 0 0
12 0 0
...
```
如果您的 PPM 文件与此不同，请仔细检查您的代码。如果它看起来确实像这样但无法渲染，那么您可能会遇到行结束差异或类似的问题，从而使您的图像查看器感到困惑。为了帮助调试这个问题，您可以在 Github 项目的 images 目录中找到一个文件 test.ppm。这应该有助于确保您的查看器可以处理 PPM 格式并用作与生成的 PPM 文件的比较。

一些读者报告在 Windows 上查看生成的文件时出现问题。在这种情况下，问题通常是 PPM 通常从 PowerShell 写为 UTF-16。如果您遇到此问题，请参阅[#1114](https://github.com/RayTracing/raytracing.github.io/discussions/1114) 以获取有关此问题的帮助。

如果所有内容都显示正确，那么您已经基本解决了系统和 IDE 问题——本系列其余部分中的所有内容都使用相同的简单机制来生成渲染图像。

如果您想生成其他图像格式，你可以使用stb_image.h，这是一个仅包含标头的图像库，可在 GitHub 上找到：[https://github.com/nothings/stb](https://github.com/nothings/stb)。

> **译者注：**
> 笔者是使用xmake进行文件构建的，下面提供一下xmake的构建方法。
> ```lua
> -- xmake.lua
> add_rules("mode.debug", "mode.release")
> target("oneWeekend")
> set_kind("binary")
> add_files("src/*.cpp")
> 
> on_run(function(target)
> 	os.mkdir("image") -- 确保 image 目录存在
> 	os.execv(target:targetfile(), {}, { stdout = "image/image.ppm" })
> end)
> ```
> 在`xmake.lua`中填入上述内容，在命令行中输入`xmake run`就可以直接运行项目，运行出来的结果会直接保存在根目录的`image`文件夹中。

## 2.3 添加进度指示器
在继续之前，让我们向输出添加一个进度指示器。这是跟踪长时间渲染进度的便捷方法，并且还可以识别由于无限循环或其他问题而停止的运行。

我们的程序将图像输出到标准输出流（std::cout），所以不要管它，而是写入日志输出流（std::clog）：

```cpp
// main.cc
    for (int j = 0; j < image_height; ++j) {
        std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
        for (int i = 0; i < image_width; ++i) {
            auto r = double(i) / (image_width-1);
            auto g = double(j) / (image_height-1);
            auto b = 0;

            int ir = static_cast<int>(255.999 * r);
            int ig = static_cast<int>(255.999 * g);
            int ib = static_cast<int>(255.999 * b);

            std::cout << ir << ' ' << ig << ' ' << ib << '\n';
        }
    }

    std::clog << "\rDone.                 \n";
```

现在，在运行时，您将看到剩余扫描行数的运行计数。它运行得非常快，以至于您甚至看不到它！别担心，当我们扩展光线追踪器时，您将来会有很多时间观看缓慢更新的进度线。

# 3. vec3类
几乎所有图形程序都有一些用于存储几何向量和颜色的类。在许多系统中，这些向量是 4D 的（3D 位置加上几何的齐次坐标，或者 RGB 加上颜色的 alpha 透明度分量）。对于我们的目的来说，三个坐标就足够了。我们将使用相同的 vec3 类来表示颜色、位置、方向、偏移量等。有些人不喜欢这个，因为它并不能阻止你做一些愚蠢的事情，比如从颜色中减去一个位置。他们的想法很好，但在没有明显错误的情况下，我们总是会采取“更少代码”的路线。尽管如此，我们还是为 vec3 声明了两个别名：`point3` 和 `color`。由于这两种类型只是 vec3 的别名，因此如果将颜色传递给需要 point3 的函数，则不会收到警告，并且没有什么可以阻止您将 point3 添加到颜色，但它使代码更容易一些阅读和理解。

我们在新的 vec3.h 头文件的上半部分定义 vec3 类，并在下半部分定义一组有用的向量实用函数：
```cpp
// vec3.h
#ifndef VEC3_H
#define VEC3_H

#include <cmath>
#include <iostream>

using std::sqrt;

class vec3 {
  public:
    double e[3];

    vec3() : e{0,0,0} {}
    vec3(double e0, double e1, double e2) : e{e0, e1, e2} {}

    double x() const { return e[0]; }
    double y() const { return e[1]; }
    double z() const { return e[2]; }

    vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }
    double operator[](int i) const { return e[i]; }
    double& operator[](int i) { return e[i]; }

    vec3& operator+=(const vec3 &v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        e[2] += v.e[2];
        return *this;
    }

    vec3& operator*=(double t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        return *this;
    }

    vec3& operator/=(double t) {
        return *this *= 1/t;
    }

    double length() const {
        return sqrt(length_squared());
    }

    double length_squared() const {
        return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
    }
};

// point3 只是 vec3 的别名，但它可以让代码变得清晰易懂。
using point3 = vec3;


// 向量实用函数

inline std::ostream& operator<<(std::ostream &out, const vec3 &v) {
    return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2];
}

inline vec3 operator+(const vec3 &u, const vec3 &v) {
    return vec3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}

inline vec3 operator-(const vec3 &u, const vec3 &v) {
    return vec3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}

inline vec3 operator*(const vec3 &u, const vec3 &v) {
    return vec3(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}

inline vec3 operator*(double t, const vec3 &v) {
    return vec3(t*v.e[0], t*v.e[1], t*v.e[2]);
}

inline vec3 operator*(const vec3 &v, double t) {
    return t * v;
}

inline vec3 operator/(vec3 v, double t) {
    return (1/t) * v;
}

inline double dot(const vec3 &u, const vec3 &v) {
    return u.e[0] * v.e[0]
         + u.e[1] * v.e[1]
         + u.e[2] * v.e[2];
}

inline vec3 cross(const vec3 &u, const vec3 &v) {
    return vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
                u.e[2] * v.e[0] - u.e[0] * v.e[2],
                u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}

inline vec3 unit_vector(vec3 v) {
    return v / v.length();
}

#endif
```
我们在这里使用 double，但有些光线追踪器使用 float。 double 具有更大的精度和范围，但大小是 float 的两倍。如果您在有限的内存条件（例如硬件着色器）下进行编程，则大小的增加可能很重要。两者都可以——遵循你自己的意愿。
## 3.1 颜色实用函数
使用新的 vec3 类，我们将创建一个新的 color.h 头文件并定义一个实用函数，该函数将单个像素的颜色写入标准输出流。
```cpp
// color.h
#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"

#include <iostream>

using color = vec3;

void write_color(std::ostream &out, color pixel_color) {
    // 写入每个颜色分量的转换后的 [0,255] 值
    out << static_cast<int>(255.999 * pixel_color.x()) << ' '
        << static_cast<int>(255.999 * pixel_color.y()) << ' '
        << static_cast<int>(255.999 * pixel_color.z()) << '\n';
}

#endif
```
现在我们可以更改`main.cc` 以使用这两个头文件：
```cpp
// main.cc
#include "color.h"
#include "vec3.h"

#include <iostream>

int main() {

    // Image

    int image_width = 256;
    int image_height = 256;

    // Render

    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = 0; j < image_height; ++j) {
        std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
        for (int i = 0; i < image_width; ++i) {
            auto pixel_color = color(double(i)/(image_width-1), double(j)/(image_height-1), 0);
            write_color(std::cout, pixel_color);
        }
    }

    std::clog << "\rDone.                 \n";
}
```
您应该得到与之前完全相同的图片。

#  4. 光线、简易相机以及背景
## 4.1 光线类
所有光线追踪器都拥有的是光线类和沿着光线看到的颜色的计算。让我们将射线视为一个函数$$ P(t) = A + t\vec{b} $$。这里的$$P$$是沿着三维空间中某条直线的三维位置。其中$$A$$是射线原点，$$\vec{b}$$是射线方向，$$t$$是类型为`double`的实数。输入不同的$$t$$值，$$P(t)$$ 就会沿着光线移动这个点。如果您输入的是负数$$t$$，就可以在3D线上的任何位置移动。如果是正的$$t$$，您只能得到A前面的部分，这通常被称为半线(*half-line*)或光线(*ray*)。
![img](https://raytracing.github.io/images/fig-1.02-lerp.jpg)
我们可以将ray的概念表示为一个class，并且将$$P(t)$$用函数`ray::at(t)`来表示：

```cpp
//ray.h
#ifndef RAY_H
#define RAY_H

#include "vec3.h"

class ray {
  public:
    ray() {}

    ray(const point3& origin, const vec3& direction) : orig(origin), dir(direction) {}

    point3 origin() const  { return orig; }
    vec3 direction() const { return dir; }

    point3 at(double t) const {
        return orig + t*dir;
    }

  private:
    point3 orig;
    vec3 dir;
};

#endif
```
## 4.2 向场景中发送光线
现在我们准备转个弯，制作一个光线追踪器。光线追踪器的核心是发送穿过像素的光线，并计算光线方向上的颜色。相关步骤如下：
1. 计算从 "眼睛 "穿过像素的光线，
2. 确定射线与哪些物体相交，
3. 以及为最近的交点计算颜色。

在第一次开发光线追踪器时，我总是用一个简单的摄像头来启动和运行代码。

我经常在调试时使用正方形图像而遇到问题，因为我太频繁地颠倒了x和y，所以我们将使用一个非正方形的图像。正方形图像的宽高比是1:1，因为它的宽度和高度相同。由于我们想要一个非正方形图像，我们将选择16:9，因为它非常常见。16:9的宽高比意味着图像宽度与高度的比率是16:9。换句话说，给定一个16:9宽高比的图像，则
$$
width/height = 16/9=1.7778
$$

举个实际例子，一幅宽800像素、高400像素的图像的长宽比为2:1。

图像的高宽比可以通过高度和宽度的比值来确定。不过，由于我们心中已经有了一个给定的高宽比，因此可以先设置图像的宽度和高宽比，然后再以此计算其高度。这样，我们就可以通过改变图像宽度来放大或缩小图像，而不会影响我们想要的宽高比。我们必须确保在求解图像高度时，得到的高度至少为 1。

除了设置渲染图像的像素尺寸外，我们还需要设置一个虚拟视口(*viewport*)，以便我们的场景光线能通过。视口是三维世界中的一个虚拟矩形，包含图像像素位置的网格。如果像素在水平方向上的间距与垂直方向上的间距相等，那么将它们包围起来的视口就会与渲染的图像具有相同的高宽比。相邻两个像素之间的距离称为像素间距，标准的像素间距为正方形。

首先，我们任意选择视口高度为2.0，然后缩放视口宽度，以获得所需的高宽比。下面是这段代码的片段：

```cpp
auto aspect_ratio = 16.0 / 9.0;
int image_width = 400;

// 计算图像高度，确保至少为 1。
int image_height = static_cast<int>(image_width / aspect_ratio);
image_height = (image_height < 1) ? 1 : image_height;

// 视口宽度小于 1 也可以，因为它们是真实值。
auto viewport_height = 2.0;
auto viewport_width = viewport_height * (static_cast<double>(image_width)/image_height);
```

如果你想知道为什么我们在计算`viewport_width`时不直接使用`aspect_ratio`，那是因为设置为`aspect_ratio`的值是理想的比例，它可能不是`image_width`和`image_height`之间的实际比例。如果图像高度可以是实值，而不仅仅是整数，那么使用`aspect_ratio`就没有问题。但是，`image_width`和`image_height`之间的实际比例会因代码的两个部分而变化。首先，`integer_height`会向下舍入为最接近的整数，这可能会增加比例。其次，我们不允许`integer_height`小于 1，这也会改变实际的宽高比。

请注意，`aspect_ratio`是一个理想的比例，我们尽可能以图像宽度与图像高度的整数比例来近似它。为了让我们的视口比例完全匹配我们的图像比例，我们使用计算出的图像宽高比来确定我们最终的视口宽度。

接下来，我们将定义摄像机中心：三维空间中的一个点，所有场景光线都将从这个点出发（通常也称为眼睛点(*eye point*)）。从摄像机中心到视口中心的矢量将与视口正交。我们最初会将视口与摄像机中心点之间的距离设置为一个单位。这个距离通常称为焦距(*focal length*)。

为了简化问题，我们将摄像机中心设置在$$(0,0,0)$$位置。我们还将设置y轴向上，x轴向右，负z轴指向观察方向。（这通常被称为右手坐标系(*right-handed coordinates*)。）

![img](https://raytracing.github.io/images/fig-1.03-cam-geom.jpg)

现在是不可避免的棘手部分。虽然我们的三维空间有上述约定，但这与我们的图像坐标相冲突，我们希望将第一个像素放在左上方，然后一直向下直到右下方的最后一个像素。这意味着我们的图像坐标Y轴是倒置的，即Y轴在图像上向下递增。

在扫描我们的图像时，我们将从左上角的像素$$(0,0)$$开始，从左到右扫描每一行，然后逐行从上到下扫描。为了帮助浏览像素网格，我们将使用一个从左边缘到右边缘的向量$$V_u$$，和一个从上边缘到下边缘的向量$$V_v$$。

我们的像素网格将以像素间距的一半嵌入视口边缘。通过这种方式，我们的视口区域被均匀地划分为宽度$$\times$$高度的相同区域。下面是我们的视口和像素网格的样子：

![img](https://raytracing.github.io/images/fig-1.04-pixel-grid.jpg)

在这个图中，我们有视口————一个$$7\times5$$分辨率图像的像素网格，视口左上角的点$$Q$$，像素$$P_{(0,0)}$$的位置，视口向量$$V_u$$（`viewport_u`），视口向量$$V_v$$（`viewport_v`），以及像素增量向量$$\delta u$$和$$\delta v$$。

基于所有这些信息，这里是实现摄像机的代码。我们将构建一个函数`ray_color(const ray& r)`，它返回给定场景光线的颜色————现在我们将其设置为始终返回黑色。

```cpp
//main.cc
#include "color.h"
#include "ray.h"
#include "vec3.h"

#include <iostream>

color ray_color(const ray& r) {
    return color(0,0,0);
}

int main() {

    // 图像

    auto aspect_ratio = 16.0 / 9.0;
    int image_width = 400;

    // 计算图像高度，并确保其至少为1
    int image_height = static_cast<int>(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;

    // 摄像机

    auto focal_length = 1.0;
    auto viewport_height = 2.0;
    auto viewport_width = viewport_height * (static_cast<double>(image_width)/image_height);
    auto camera_center = point3(0, 0, 0);

    // 计算横向和纵向视口边缘的矢量
    auto viewport_u = vec3(viewport_width, 0, 0);
    auto viewport_v = vec3(0, -viewport_height, 0);

    // 计算像素间的水平和垂直三角矢量
    auto pixel_delta_u = viewport_u / image_width;
    auto pixel_delta_v = viewport_v / image_height;

    // 计算左上角像素的位置
    auto viewport_upper_left = camera_center
                             - vec3(0, 0, focal_length) - viewport_u/2 - viewport_v/2;
    auto pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

    // 渲染

    std::cout << "P3\n" << image_width << " " << image_height << "\n255\n";

    for (int j = 0; j < image_height; ++j) {
        std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
        for (int i = 0; i < image_width; ++i) {
            auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
            auto ray_direction = pixel_center - camera_center;
            ray r(camera_center, ray_direction);

            color pixel_color = ray_color(r);
            write_color(std::cout, pixel_color);
        }
    }

    std::clog << "\rDone.                 \n";
}
```

请注意，在上面的代码中，我没有将`ray_direction`设置为单位向量，因为我认为不这样做可以使代码更简单且略微提高一点速度。

现在我们将填充`ray_color(ray)`函数以实现一个简单的渐变。这个函数将根据y坐标的高度（在将光线方向缩放到单位长度后，$$−1.0<y<1.0$$）线性混合白色和蓝色。因为我们是在标准化向量后查看y的高度，你会注意到除了垂直渐变之外，颜色中还有水平渐变。

我将使用一个标准的图形学技巧来线性缩放$$0.0≤a≤1.0$$。当$$a=1.0$$时，我想要蓝色。当$$a=0.0$$时，我想要白色。在两者之间，我想要一个混合色。这形成了一个“线性混合(*linear blend*)”或“线性插值(*linear interpolation*)”。这通常被称为两个值之间的*lerp*。*lerp*总是以下面的形式表示：

$$
blendedValue = (1-a)\cdot startValue + a\cdot endValue
$$

其中$$a$$从0变成1。

综合以上的信息，我们得到了以下结果：
```cpp
//main.cc
#include "color.h"
#include "ray.h"
#include "vec3.h"

#include <iostream>


color ray_color(const ray& r) {
    vec3 unit_direction = unit_vector(r.direction());
    auto a = 0.5*(unit_direction.y() + 1.0);
    return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
}

...
```

由此，可以生成以下的图像：
![img](https://raytracing.github.io/images/img-1.02-blue-to-white.png)

让我们向光线追踪器添加一个对象。人们经常在光线追踪器中使用球体，因为计算光线是否击中球体相对简单。

## 5.1 光线-球体相交

以原点为圆心、半径为$$r$$的球体方程是一个重要的数学方程：
$$
x^2 + y^2+z^2 = r^2
$$
也可以这样理解，如果一个给定的点$$ (x, y, z) $$在球体上，那么 $$x² + y² + z² = r²$$。如果一个给定的点$$ (x, y, z) $$在球体内部，那么$$ x² + y² + z² < r²$$；如果一个给定的点$$ (x, y, z) $$在球体外部，那么$$ x² + y² + z² > r²$$。

如果我们想让球心位于任意点$$ (C_x, C_y, C_z)$$，那么方程会变得复杂得多：
$$
(x-C_x)^2+(y-C_y)^2+(z-C_z)^2=r^2
$$
在图形学中，你几乎总是希望你的公式用向量表示，这样所有的$$ x/y/z $$变量就可以简单地用`vec3`类表示。你可能注意到从中心点$$ C=(C_x, C_y, C_z) $$到点$$ P=(x, y, z)$$ 的向量是 $$(P−C)$$。如果我们使用点积的定义：
$$
(P-C)\cdot(P-C)=(x-C_x)^2+(y-C_y)^2+(z-C_z)^2
$$
那么我们可以将球的方程重写为向量形式：
$$
(P-C)\cdot(P-C)=r^2
$$
我们可以将这读作*“任何满足这个方程的点$$ P $$都在球面上”*。我们想知道我们的射线$$ P(t)=A+t\vec b $$是否会击中球体的任何位置。如果它击中了球体，那么存在某个$$ t $$使得$$ P(t) $$满足球的方程。所以我们在寻找满足以下条件的任意$$ t$$：
$$
(P(t)-C)\cdot(P(t)-C)=r^2
$$
这可以通过将 P(t) 替换为它的展开形式来找到：
$$
((A+t\vec b)-C)\cdot((A+t\vec b)-C)=r^2
$$
左边有三个向量点乘右边的三个向量。如果我们解开完整的点积，将会得到九个向量。你当然可以去写出所有东西，但我们不需要这么努力。如果你还记得，我们想要解出$$ t$$，所以我们将根据是否有$$t$$来分离项：
$$
(tb+(A-C))\cdot(tb+(A-C))=r^2
$$
现在我们按照向量代数的规则分配点积：
$$
t^2b\cdot b+2tb\cdot(A-C)+(A-C)+(A-C)\cdot (A-C)=r^2
$$
将半径的平方移到左边：
$$
t^2b\cdot b+2tb\cdot(A-C)+(A-C)+(A-C)\cdot (A-C)-r^2=0
$$
很难明确这个方程是什么，但方程中的向量和$$ r $$都是常量并且是已知的。此外，我们有的唯一向量通过点积被简化为标量。唯一的未知数是$$t$$，我们有一个$$ t²$$，这意味着这个方程是二次方程。你可以通过使用二次公式来解二次方程：
$$
\frac{-b\pm \sqrt{b^2-4ac}}{2a}
$$
对于射线-球交点，$$a/b/c $$的值为：
$$
a=b\cdot b\\
b=2b\cdot(A-C)\\
c=(A-C)\cdot(A-C)-r^2
$$
使用上述所有内容，你可以解出$$ t$$，但方程中有一个平方根部分，可以是正的（意味着有两个实数解），负的（意味着没有实数解），或零（意味着一个实数解）。在图形学中，代数几乎总是与几何直接相关。我们拥有的是：

![img](https://raytracing.github.io/images/fig-1.05-ray-sphere.jpg)

## 5.2 创建我们的第一个光线追踪图像

如果我们将这个数学公式硬编码到我们的程序中，我们可以通过在$$ z=-1 $$的位置放置一个小球体，然后将与其相交的任何像素着色为红色来测试我们的代码。

```cpp
//main.cc
bool hit_sphere(const point3& center, double radius, const ray& r) {
    vec3 oc = r.origin() - center;
    auto a = dot(r.direction(), r.direction());
    auto b = 2.0 * dot(oc, r.direction());
    auto c = dot(oc, oc) - radius*radius;
    auto discriminant = b*b - 4*a*c;
    return (discriminant >= 0);
}

color ray_color(const ray& r) {
    if (hit_sphere(point3(0,0,-1), 0.5, r))
        return color(1, 0, 0);

    vec3 unit_direction = unit_vector(r.direction());
    auto a = 0.5*(unit_direction.y() + 1.0);
    return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
}
```

我们可以得到这个：

![img](https://raytracing.github.io/images/img-1.03-red-sphere.png)

现在这还缺少很多东西——比如阴影、反射光线以及多个物体——但我们已经完成了一半多的工作，比刚开始时要好很多！需要注意的一点是，我们通过解二次方程来测试射线是否与球相交，并查看是否存在解，但是$$ t $$的负值解也同样有效。如果你将球心改为$$ z=+1$$，你会得到完全相同的图片，因为这个解决方案并不区分相机前面的物体和相机后面的物体。这不是一个*feature*！我们接下来会解决这些问题。

# 6. 表面法线和多个对象

## 6.1 表面法线着色

首先，让我们获得一个表面法线，以便我们可以进行着色。这是一个在交点处垂直于表面的向量。

在我们的代码中，对法线向量的关键设计决策是：法线向量是具有任意长度，还是规范化为单位长度。

如果不需要对向量进行归一化处理，跳过繁琐的平方根运算是很有诱惑力的。然而，实际上有三个重要的观察点。首先，如果需要单位长度的法向量，那么您最好预先执行一次，而不是对每个需要单位长度的位置一遍又一遍地“以防万一”。其次，我们确实在几个地方需要单位长度的法线向量。第三，如果要求法线向量是单位长度，那么通常可以通过理解特定的几何类，在其构造函数中或在`hit()`函数中高效地生成这个向量。例如，通过除以球体半径，球形法线可以简单地成为单位长度，完全避免了平方根计算。

考虑到所有这些，我们将采用所有法向量均为单位长度的策略。

对于球体，向外法线是击中点减去中心的方向：

![img](https://raytracing.github.io/images/fig-1.06-sphere-normal.jpg)

在地球上，这意味着从地球中心到你的向量垂直指向上方。现在让我们把这个加入到代码中，并进行着色。我们还没有任何灯光或类似的东西，所以就让我们通过颜色贴图来直观地展示法线。一个用于可视化法线的常见技巧（因为它简单且直观，假设$$\vec n$$是单位长度向量——则每个分量介于$$ -1 $$和$$ 1 $$之间）是将每个分量映射到从$$ 0 $$到$$ 1 $$的区间，然后将$$ (x, y, z) $$映射到 (红, 绿, 蓝)。对于法线，我们需要击中点(*hit point*)，而不仅仅是我们是否击中（这是我们目前正在计算的全部）。我们场景中只有一个球体，而且它直接在摄像机前方，所以我们还不用担心$$ t $$的负值。我们只假设最近的击中点（最小的$$ t$$）是我们想要的那个。这些代码的更改让我们能够计算并可视化$$\vec n$$：

`````cpp
//main.cc
double hit_sphere(const point3& center, double radius, const ray& r) {
    vec3 oc = r.origin() - center;
    auto a = dot(r.direction(), r.direction());
    auto b = 2.0 * dot(oc, r.direction());
    auto c = dot(oc, oc) - radius*radius;
    auto discriminant = b*b - 4*a*c;

    if (discriminant < 0) {
        return -1.0;
    } else {
        return (-b - sqrt(discriminant) ) / (2.0*a);
    }
}

color ray_color(const ray& r) {
    auto t = hit_sphere(point3(0,0,-1), 0.5, r);
    if (t > 0.0) {
        vec3 N = unit_vector(r.at(t) - vec3(0,0,-1));
        return 0.5*color(N.x()+1, N.y()+1, N.z()+1);
    }

    vec3 unit_direction = unit_vector(r.direction());
    auto a = 0.5*(unit_direction.y() + 1.0);
    return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
}
`````

## 6.2 简化光线—球相交代码

让我们回顾一下射线球函数：

```cpp
//main.cc
double hit_sphere(const point3& center, double radius, const ray& r) {
    vec3 oc = r.origin() - center;
    auto a = dot(r.direction(), r.direction());
    auto b = 2.0 * dot(oc, r.direction());
    auto c = dot(oc, oc) - radius*radius;
    auto discriminant = b*b - 4*a*c;

    if (discriminant < 0) {
        return -1.0;
    } else {
        return (-b - sqrt(discriminant) ) / (2.0*a);
    }
}
```

首先，回想一下，向量点乘自身等于该向量长度的平方。

其次，请注意$$ b $$的方程中有一个系数$$2$$。想想如果$$ b=2h$$
$$
\frac{-b\pm\sqrt{b^2-4ac}}{2a}\\
=\frac{-2h\pm\sqrt{(2h)^2-4ac}}{2a}\\
=\frac{-2h\pm2\sqrt{h^2-ac}}{2a}\\
=\frac{-h\pm\sqrt{h^2-ac}}{a}
$$
利用这些观察结果，我们现在可以将球体相交代码简化为：

```cpp
//main.cc
double hit_sphere(const point3& center, double radius, const ray& r) {
    vec3 oc = r.origin() - center;
    auto a = r.direction().length_squared();
    auto half_b = dot(oc, r.direction());
    auto c = oc.length_squared() - radius*radius;
    auto discriminant = half_b*half_b - a*c;

    if (discriminant < 0) {
        return -1.0;
    } else {
        return (-half_b - sqrt(discriminant) ) / a;
    }
}
```

## 6.3 对`Hittable`对象的抽象

如果有多个球体呢？虽然拥有一组球体很诱人，但一个非常干净的解决方案是为光线可能击中的任何物体创建一个“抽象类”，并使球体和球体列表都只是可以击中的东西。该类应该被称为什么是一个两难的问题——如果不是因为“**面向对象**”编程，将其称为“**对象**”会很好。“**表面**”（*Surface*）这个名称经常被使用，缺点是我们可能需要体积（雾、云等）。` hittable`强调了它们共有的成员函数。我对这些名称都不是很满意，但我们将使用`hittable`。

`hittable`类将有一个接受射线的` hit `函数。大多数光线追踪器发现添加一个有效的碰撞区间$$ t_{min} $$到$$ t_{max} $$很方便，因此只有当$$ t_{min} < t < t_{max}$$时，碰撞才“有效”。对于初始射线来说，这是正的$$ t$$，但正如我们将看到的，有一个从$$ t_{min} $$到$$ t_{max} $$的区间可以简化我们的代码。一个设计问题是，是否在我们击中某物时计算法线。在我们的搜索过程中，我们可能会击中更近的东西，而我们只需要最近物体的法线。我会选择一个简单的解决方案，并计算一系列我将存储在某种结构中的东西。这里是抽象类：

```cpp
//hittable.h
#ifndef HITTABLE_H
#define HITTABLE_H

#include "ray.h"

class hit_record {
  public:
    point3 p;
    vec3 normal;
    double t;
};

class hittable {
  public:
    virtual ~hittable() = default;

    virtual bool hit(const ray& r, double ray_tmin, double ray_tmax, hit_record& rec) const = 0;
};

#endif
```

And here’s the sphere:

```cpp
//sphere.h
#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#include "vec3.h"

class sphere : public hittable {
  public:
    sphere(point3 _center, double _radius) : center(_center), radius(_radius) {}

    bool hit(const ray& r, double ray_tmin, double ray_tmax, hit_record& rec) const override {
        vec3 oc = r.origin() - center;
        auto a = r.direction().length_squared();
        auto half_b = dot(oc, r.direction());
        auto c = oc.length_squared() - radius*radius;

        auto discriminant = half_b*half_b - a*c;
        if (discriminant < 0) return false;
        auto sqrtd = sqrt(discriminant);

        // 找出位于可接受范围内的最近的根
        auto root = (-half_b - sqrtd) / a;
        if (root <= ray_tmin || ray_tmax <= root) {
            root = (-half_b + sqrtd) / a;
            if (root <= ray_tmin || ray_tmax <= root)
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        rec.normal = (rec.p - center) / radius;

        return true;
    }

  private:
    point3 center;
    double radius;
};

#endif
```

## 6.4 正面和背面

