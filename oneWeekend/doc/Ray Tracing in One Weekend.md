![Ray Tracing in OneWeekend v4.0.0 (中文翻译)](https://pic1.zhimg.com/70/v2-b88e5b56376a3e3e9dbb1a21cc2370d6_1440w.image?source=172ae18b&biz_tag=Post)

# Ray Tracing in OneWeekend v4.0.0 (中文翻译)

> 声明：此文章仅供个人学习，由于英文水平有限，本文为我个人基于翻译插件加以优化所得，会尽可能还原原意。如果各位有更好的建议意见可以在评论区中提出。
# 1. 概述
略。
# 2. 输出一个图像
## 2.1 PPM图像格式
每当启动渲染器时，您都需要一种查看图像的方法。最直接的方法就是将其写入文件。问题是，格式有很多。其中许多都很复杂。我总是从纯文本 ppm 文件开始。这是维基百科上的一个很好的描述：
![*PPM 示例*](https://raytracing.github.io/images/fig-1.01-ppm.jpg)
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
![*第一张 PPM 图像*](https://raytracing.github.io/images/img-1.01-first-ppm-image.png)
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
![*线性插值*](https://raytracing.github.io/images/fig-1.02-lerp.jpg)
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

![*相机几何形状*](https://raytracing.github.io/images/fig-1.03-cam-geom.jpg)

现在是不可避免的棘手部分。虽然我们的三维空间有上述约定，但这与我们的图像坐标相冲突，我们希望将第一个像素放在左上方，然后一直向下直到右下方的最后一个像素。这意味着我们的图像坐标Y轴是倒置的，即Y轴在图像上向下递增。

在扫描我们的图像时，我们将从左上角的像素$$(0,0)$$开始，从左到右扫描每一行，然后逐行从上到下扫描。为了帮助浏览像素网格，我们将使用一个从左边缘到右边缘的向量$$V_u$$，和一个从上边缘到下边缘的向量$$V_v$$。

我们的像素网格将以像素间距的一半嵌入视口边缘。通过这种方式，我们的视口区域被均匀地划分为宽度$$\times$$高度的相同区域。下面是我们的视口和像素网格的样子：

![*视口和像素网格*](https://raytracing.github.io/images/fig-1.04-pixel-grid.jpg)

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
![*取决于射线 Y 坐标的蓝到白渐变*](https://raytracing.github.io/images/img-1.02-blue-to-white.png)

让我们向光线追踪器添加一个对象。人们经常在光线追踪器中使用球体，因为计算光线是否击中球体相对简单。

# 5. 添加球体

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

![*光线-球体相交结果*](https://raytracing.github.io/images/fig-1.05-ray-sphere.jpg)

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

![*一个简单的红色球体*](https://raytracing.github.io/images/img-1.03-red-sphere.png)

现在这还缺少很多东西——比如阴影、反射光线以及多个物体——但我们已经完成了一半多的工作，比刚开始时要好很多！需要注意的一点是，我们通过解二次方程来测试射线是否与球相交，并查看是否存在解，但是$$ t $$的负值解也同样有效。如果你将球心改为$$ z=+1$$，你会得到完全相同的图片，因为这个解决方案并不区分相机前面的物体和相机后面的物体。这不是一个*feature*！我们接下来会解决这些问题。

# 6. 表面法线和多个对象

## 6.1 表面法线着色

首先，让我们获得一个表面法线，以便我们可以进行着色。这是一个在交点处垂直于表面的向量。

在我们的代码中，对法线向量的关键设计决策是：法线向量是具有任意长度，还是规范化为单位长度。

如果不需要对向量进行归一化处理，跳过繁琐的平方根运算是很有诱惑力的。然而，实际上有三个重要的观察点。首先，如果需要单位长度的法向量，那么您最好预先执行一次，而不是对每个需要单位长度的位置一遍又一遍地“以防万一”。其次，我们确实在几个地方需要单位长度的法线向量。第三，如果要求法线向量是单位长度，那么通常可以通过理解特定的几何类，在其构造函数中或在`hit()`函数中高效地生成这个向量。例如，通过除以球体半径，球形法线可以简单地成为单位长度，完全避免了平方根计算。

考虑到所有这些，我们将采用所有法向量均为单位长度的策略。

对于球体，向外法线是击中点减去中心的方向：

![*球体表面法线几何形状*](https://raytracing.github.io/images/fig-1.06-sphere-normal.jpg)

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

这就产生了这张图：

![*根据法向量着色的球体*](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-1.04-normals-sphere.png)

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

这是球体的代码：

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

关于法线的第二个设计决策是它们是否应该总是指向外部。目前，找到的法线总是从中心指向交点的方向（法线指向外部）。如果光线从外部与球体相交，那么法线会与光线相反。如果光线从内部与球体相交，那么法线（总是指向外部）会与光线同向。另一种方式是，我们可以让法线总是与光线相反。如果光线在球体外部，法线会指向外部，但如果光线在球体内部，法线会指向内部。

![*球体表面法线几何的可能方向*](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-1.07-normal-sides.jpg)

我们需要选择其中一种可能性，因为我们最终想确定光线是从表面的哪一侧射入的。这对于在每一侧呈现不同的对象很重要，比如双面纸上的文字，或者像玻璃球这样有内外之分的对象。

如果我们决定让法向量总是指向外部，那么我们需要确定光线在着色时处于物体的哪一侧。我们可以通过比较光线与法线来弄清楚这一点。**如果光线和法线朝相同方向，那么光线就在物体内部；如果光线和法线朝相反方向，那么光线就在物体外部。**这可以通过计算两个向量的点积来确定，**如果它们的点积为正，那么光线就在球体内部。**

```cpp
if (dot(ray_direction, outward_normal) > 0.0) {
    // 光线在球体内侧
    ...
} else {
    // 光线在球体外侧
    ...
}
```

如果我们决定让法线始终指向光线，我们将无法使用点积来确定光线位于表面的哪一侧。相反，我们需要存储该信息：

```cpp
bool front_face;
if (dot(ray_direction, outward_normal) > 0.0) {
    // 光线在球体内测
    normal = -outward_normal;
    front_face = false;
} else {
    // 光线在球体外侧
    normal = outward_normal;
    front_face = true;
}
```

我们可以设置使法线总是从表面“向外”指，或者总是指向入射光线的反方向。这个决定取决于你是想在几何相交的时候确定表面的哪一侧，还是在着色的时候确定。在这本书中，我们有的材料类型比几何类型多，所以我们选择较少的工作量，在几何相交时就做出决定。这纯粹是个人偏好的问题，在文献中你会看到两种实现方式。

我们在 `hit_record` 类中添加一个布尔变量 `front_face`。我们还将添加一个函数来为我们解决这个计算：`set_face_normal()`。为了方便起见，我们假设传递给新的 `set_face_normal()` 函数的向量是单位长度的。我们总是可以明确地规范化这个参数，但如果几何代码这样做会更高效，因为当你更了解特定几何形状时，通常会更容易。

```cpp
// hittable.h
class hit_record {
  public:
    point3 p;
    vec3 normal;
    double t;
    bool front_face;

    void set_face_normal(const ray& r, const vec3& outward_normal) {
        // 设置命中记录的法向量
        // NOTE: outward_normal 假定为单位长度

        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};
```

然后，我们将表面侧判定添加到该类中：

```cpp
// sphere.h
class sphere : public hittable {
  public:
    ...
    bool hit(const ray& r, double ray_tmin, double ray_tmax, hit_record& rec) const {
        ...

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);

        return true;
    }
    ...
};
```

## 6.5 可命中对象列表

我们有一个称为`hittable`的通用对象，光线可以与它相交。我们现在添加一个存储命中表列表的类，命名为`hittable_list`：

```cpp
// hittable_list.h
#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "hittable.h"

#include <memory>
#include <vector>

using std::shared_ptr;
using std::make_shared;

class hittable_list : public hittable {
  public:
    std::vector<shared_ptr<hittable>> objects;

    hittable_list() {}
    hittable_list(shared_ptr<hittable> object) { add(object); }

    void clear() { objects.clear(); }

    void add(shared_ptr<hittable> object) {
        objects.push_back(object);
    }

    bool hit(const ray& r, double ray_tmin, double ray_tmax, hit_record& rec) const override {
        hit_record temp_rec;
        bool hit_anything = false;
        auto closest_so_far = ray_tmax;

        for (const auto& object : objects) {
            if (object->hit(r, ray_tmin, closest_so_far, temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        return hit_anything;
    }
};

#endif
```

## 6.6 一些新的C++特性

`hittable_list`类代码使用两个 C++ 的新特性，如果您不是 C++ 程序员，这两个功能可能会让您感到困惑：`vector`和`shared_ptr`。

`shared_ptr<type>` 是指向某个已分配类型的指针，具有引用计数语义。每次将其值分配给另一个共享指针（通常使用简单的分配）时，引用计数都会增加。当共享指针超出范围（例如在块或函数的末尾）时，引用计数就会递减。一旦计数变为零，该对象就会被安全删除。

通常，共享指针首先使用新分配的对象进行初始化，如下所示：

```cpp
shared_ptr<double> double_ptr = make_shared<double>(0.37);
shared_ptr<vec3>   vec3_ptr   = make_shared<vec3>(1.414214, 2.718281, 1.618034);
shared_ptr<sphere> sphere_ptr = make_shared<sphere>(point3(0,0,0), 1.0);
```

`make_shared<thing>(thing_constructor_params ...)`使用构造函数参数分配` thing `类型的新实例。它返回一个`shared_ptr<thing>`。

由于类型可以通过` make_shared<type>(...) `的返回类型自动推导，因此可以使用 C++ 的` auto `类型说明符更简单地表达上述代码：

```cpp
auto double_ptr = make_shared<double>(0.37);
auto vec3_ptr   = make_shared<vec3>(1.414214, 2.718281, 1.618034);
auto sphere_ptr = make_shared<sphere>(point3(0,0,0), 1.0);
```

我们将在代码中使用共享指针，因为它允许多个几何图形共享一个公共实例（例如，一堆都使用相同颜色材质的球体），并且因为它使内存管理自动化并且更容易推理。

`std::shared_ptr`包含在` <memory> `头文件中。

您可能不熟悉的第二个 C++ 功能是` std::vector`。这是任意类型的通用类数组集合。上面，我们使用了指向` hittable `的指针集合。` std::vector `随着添加更多值而自动增长：`objects.push_back(object) `将一个值添加到` std::vector `成员变量对象的末尾。

`std::vector `包含在` <vector> `头文件中。

最后，上面`hittable_list.h`中的` using `语句告诉编译器我们将从` std `库获取` shared_ptr `和` make_shared`，因此我们不需要每次引用它们时都在它们前面加上 `std:: `前缀。

## 6.7 常用常量和实用函数

我们需要一些数学常量，可以在自己的头文件中方便地定义它们。现在我们只需要无穷大，但我们还将在其中添加我们自己的` pi `定义，稍后我们将会用到它。` pi `没有标准的可移植定义，因此我们只需为其定义自己的常量。我们将在` rtweekend.h`（我们的通用主头文件）中添加常见的有用常量和未来会用到的实用函数。

```cpp
// rtweekend.h
#ifndef RTWEEKEND_H
#define RTWEEKEND_H

#include <cmath>
#include <limits>
#include <memory>


// Usings

using std::shared_ptr;
using std::make_shared;
using std::sqrt;

// 常量

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

// 实用函数

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

// 通用头文件

#include "ray.h"
#include "vec3.h"

#endif
```

这是新的`main.cc`:

```cpp
// main.cc
#include "rtweekend.h"

#include "color.h"
#include "hittable.h"
#include "hittable_list.h"
#include "sphere.h"

#include <iostream>

double hit_sphere(const point3& center, double radius, const ray& r) {
    ...
}

color ray_color(const ray& r, const hittable& world) {
    hit_record rec;
    if (world.hit(r, 0, infinity, rec)) {
        return 0.5 * (rec.normal + color(1,1,1));
    }

    vec3 unit_direction = unit_vector(r.direction());
    auto a = 0.5*(unit_direction.y() + 1.0);
    return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
}

int main() {

    // Image

    auto aspect_ratio = 16.0 / 9.0;
    int image_width = 400;

    // Calculate the image height, and ensure that it's at least 1.
    int image_height = static_cast<int>(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;

    // World

    hittable_list world;

    world.add(make_shared<sphere>(point3(0,0,-1), 0.5));
    world.add(make_shared<sphere>(point3(0,-100.5,-1), 100));

    // Camera

    auto focal_length = 1.0;
    auto viewport_height = 2.0;
    auto viewport_width = viewport_height * (static_cast<double>(image_width)/image_height);
    auto camera_center = point3(0, 0, 0);

    // Calculate the vectors across the horizontal and down the vertical viewport edges.
    auto viewport_u = vec3(viewport_width, 0, 0);
    auto viewport_v = vec3(0, -viewport_height, 0);

    // Calculate the horizontal and vertical delta vectors from pixel to pixel.
    auto pixel_delta_u = viewport_u / image_width;
    auto pixel_delta_v = viewport_v / image_height;

    // Calculate the location of the upper left pixel.
    auto viewport_upper_left = camera_center
                             - vec3(0, 0, focal_length) - viewport_u/2 - viewport_v/2;
    auto pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

    // Render

    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = 0; j < image_height; ++j) {
        std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
        for (int i = 0; i < image_width; ++i) {
            auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
            auto ray_direction = pixel_center - camera_center;
            ray r(camera_center, ray_direction);

            color pixel_color = ray_color(r, world);
            write_color(std::cout, pixel_color);
        }
    }

    std::clog << "\rDone.                 \n";
}
```

这产生的图片实际上只是球体位置及其表面法线的可视化。这通常是查看几何模型的任何缺陷或特定特征的好方法。

![法线颜色球体与地面的渲染结果](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-1.05-normals-sphere-ground.png)

## 6.8 `Interval`类

在继续之前，我们将实现一个`interval`类来管理具有最小值和最大值的实值间隔。随着我们的继续，我们最终会经常使用这个类。

```cpp
// interval.h
#ifndef INTERVAL_H
#define INTERVAL_H

class interval {
  public:
    double min, max;

    interval() : min(+infinity), max(-infinity) {} // Default interval is empty

    interval(double _min, double _max) : min(_min), max(_max) {}

    bool contains(double x) const {
        return min <= x && x <= max;
    }

    bool surrounds(double x) const {
        return min < x && x < max;
    }

    static const interval empty, universe;
};

const static interval empty   (+infinity, -infinity);
const static interval universe(-infinity, +infinity);

#endif
```

```cpp
// rtweekend.h
// Common Headers

#include "interval.h"
#include "ray.h"
#include "vec3.h"
```

```cpp
//hittable.h
class hittable {
  public:
    ...
    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;
};
```

```cpp
// hittable_list.h
class hittable_list : public hittable {
  public:
    ...
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        hit_record temp_rec;
        bool hit_anything = false;
        auto closest_so_far = ray_t.max;

        for (const auto& object : objects) {
            if (object->hit(r, interval(ray_t.min, closest_so_far), temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        return hit_anything;
    }
    ...
};
```

```cpp
// sphere.h
class sphere : public hittable {
  public:
    ...
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        ...

        // Find the nearest root that lies in the acceptable range.
        auto root = (-half_b - sqrtd) / a;
        if (!ray_t.surrounds(root)) {
            root = (-half_b + sqrtd) / a;
            if (!ray_t.surrounds(root))
                return false;
        }
        ...
    }
    ...
};
```

```cpp
// main.cc
...
color ray_color(const ray& r, const hittable& world) {
    hit_record rec;
    if (world.hit(r, interval(0, infinity), rec)) {
        return 0.5 * (rec.normal + color(1,1,1));
    }

    vec3 unit_direction = unit_vector(r.direction());
    auto a = 0.5*(unit_direction.y() + 1.0);
    return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
}
...
```

## 6.9 笔者注——关于`Interval`类的一些改动

笔者在自己尝试添加`interval`类时遇到了循环包含头文件的报错，虽然最后还是可以正常编译，但是这些报错看起来就是很不爽。由此，我添加了一个`constant.h`，并把`infinity`写进去来消除这些报错，如果读者有更好的建议欢迎发在评论区。

```cpp
// constant.h
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <limits>

const double infinity = std::numeric_limits<double>::infinity();

#endif
```

然后在`rtweekend.h`中删掉`infinity`的定义，再包含`constant.h`即可。

# 7. 将摄像机代码移入自己的类中

在继续之前，现在是将我们的相机和场景渲染代码合并到一个新类中的好时机：`camera`类。`camera`类将负责两项重要工作：

1. 构造光线并将其发送到世界中。
2. 使用这些光线的结果来构建渲染图像。

在这种重构中，我们将收集` ray_color() `函数以及主程序的图像、相机和渲染部分。新的`camera`类将包含两个公共方法`initialize()`和`render()`，以及两个私有方法`get_ray()`和`ray_color()`。

最终，相机将遵循我们能想到的最简单的使用模式：它的默认构造是没有参数的，然后代码将通过简单赋值修改相机的公共变量，最后通过调用`initialize()`函数初始化一切。这种模式被选择是为了避免拥有者调用带有大量参数的构造函数或定义并调用一系列设置方法。相反，代码只需要设置它明确关心的内容。最后，我们可以让代码调用`initialize()`，或者只让相机在`render()`开始时自动调用这个函数。此处，我们将使用第二种方法。

在`main`函数创建相机并设置默认值之后，它将调用`render()`方法。`render()`方法将为渲染准备相机，然后执行渲染循环。

以下是我们新相机类的框架：

```cpp
// camera.h
#ifndef CAMERA_H
#define CAMERA_H

#include "rtweekend.h"

#include "color.h"
#include "hittable.h"

class camera {
  public:
    /* 这里是公共相机参数 */

    void render(const hittable& world) {
        ...
    }

  private:
    /* 这里是私有相机变量 */

    void initialize() {
        ...
    }

    color ray_color(const ray& r, const hittable& world) const {
        ...
    }
};

#endif
```

首先，让我们填写` main.cc `中的` ray_color() `函数：

```cpp
// camera.h
class camera {
  ...

  private:
    ...

    color ray_color(const ray& r, const hittable& world) const {
        hit_record rec;

        if (world.hit(r, interval(0, infinity), rec)) {
            return 0.5 * (rec.normal + color(1,1,1));
        }

        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5*(unit_direction.y() + 1.0);
        return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
    }
};

#endif
```

现在我们将` main() `函数中的几乎所有内容都移到了新的相机类中。` main() `函数中唯一剩下的就是构建世界。下面是包含新迁移代码的相机类

```cpp
// camera.h
...
#include "rtweekend.h"

#include "color.h"
#include "hittable.h"

#include <iostream>

class camera {
  public:
    double aspect_ratio = 1.0;  // 图像宽度与高度之比
    int    image_width  = 100;  // 以像素为单位的渲染图像宽度

    void render(const hittable& world) {
        initialize();

        std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

        for (int j = 0; j < image_height; ++j) {
            std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
            for (int i = 0; i < image_width; ++i) {
                auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
                auto ray_direction = pixel_center - center;
                ray r(center, ray_direction);

                color pixel_color = ray_color(r, world);
                write_color(std::cout, pixel_color);
            }
        }

        std::clog << "\rDone.                 \n";
    }

  private:
    int    image_height;   // 渲染图像高度
    point3 center;         // 相机中心
    point3 pixel00_loc;    // （0，0）像素的位置
    vec3   pixel_delta_u;  // 向右偏移像素
    vec3   pixel_delta_v;  // 向下偏移像素

    void initialize() {
        image_height = static_cast<int>(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        center = point3(0, 0, 0);

        // 确定视口尺寸
        auto focal_length = 1.0;
        auto viewport_height = 2.0;
        auto viewport_width = viewport_height * (static_cast<double>(image_width)/image_height);

        // 计算横向和纵向视口边缘的矢量
        auto viewport_u = vec3(viewport_width, 0, 0);
        auto viewport_v = vec3(0, -viewport_height, 0);

        // 计算像素间的水平和垂直增量向量
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // 计算左上角像素的位置
        auto viewport_upper_left =
            center - vec3(0, 0, focal_length) - viewport_u/2 - viewport_v/2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
    }

    color ray_color(const ray& r, const hittable& world) const {
        ...
    }
};

#endif
```

这是代码大大减少后的`main.cc`：

```cpp
// main.cc
#include "rtweekend.h"

#include "camera.h"
#include "hittable_list.h"
#include "sphere.h"

int main() {
    hittable_list world;

    world.add(make_shared<sphere>(point3(0,0,-1), 0.5));
    world.add(make_shared<sphere>(point3(0,-100.5,-1), 100));

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width  = 400;

    cam.render(world);
}
```

运行这个重构后的程序应该会生成与以前相同的渲染图像。

# 8. 抗锯齿

当你放大到目前为止渲染过的图像时，你可能会注意到图像边缘的“阶梯状”特性，这种现象通常被称为“走样”(*aliasing*)或“锯齿”(jaggies)。与此相反，真实相机拍摄的照片通常沿着边缘没有锯齿，因为边缘像素是前景和背景的混合。请考虑，与我们渲染的图像不同，真实的世界图像是连续的。换句话说，世界（以及它的任何真实图像）实际上具有无限的分辨率。我们可以通过对每个像素取多个样本的平均值来获得相同的效果。

当单条光线穿过每个像素的中心时，我们执行通常所说的点采样(*point sampling*)的过程。点采样的问题可以通过渲染一个远处的小棋盘格来说明。如果这个棋盘格由一个8×8的黑白瓷砖网格组成，但只有四条射线击中它，那么所有四条射线可能只与白色瓷砖相交，或者只与黑色相交，或者是一些奇怪的组合。在现实世界中，当我们用眼睛远远地看到一个棋盘格时，我们会感知到它是灰色的，而不是黑白色的尖锐点。这是因为我们的眼睛自然地在做我们希望射线追踪器做的事情：整合落在渲染图像的特定（离散）区域上的光线（连续函数）。

显然，我们通过在像素中心多次重新采样相同的光线并不会得到任何好处——我们每次都会得到相同的结果。相反，我们想要采样落在像素周围的光，然后将这些样本整合起来，得到近似真实的连续结果。那么，我们如何整合落在像素周围的光呢？

我们将采用最简单的模型：采样位于像素中心并延伸到四个相邻像素中心的正方形区域。这不是最佳方法，但它是最直接的。（有关此主题的更深入讨论，请参阅“[A Pixel is Not a Little Square](https://www.researchgate.net/publication/244986797_A_Pixel_Is_Not_A_Little_Square_A_Pixel_Is_Not_A_Little_Square_A_Pixel_Is_Not_A_Little_Square)”）。![像素样本](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-1.08-pixel-samples.jpg)

## 8.1 一些随机数工具

我们需要一个能够返回真实随机数的随机数生成器。这个函数应该返回一个规范的随机数，按照惯例，这个数在$$0≤n<1$$的范围内。这里的$$<1$$很重要，因为有时我们会利用这一点。

一个简单的方法是使用可以在`<cstdlib>`中找到的`rand()`函数，它返回一个范围在`0`和`RAND_MAX`之间的随机整数。因此，我们可以通过以下代码片段获取所需的真实随机数，这段代码添加到`rtweekend.h`中：

```cpp
// rtweekend.h
#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
...

// 工具函数

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

inline double random_double() {
    // 返回 [0,1) 中的随机实数。
    return rand() / (RAND_MAX + 1.0);
}

inline double random_double(double min, double max) {
    // 返回 [min, max) 中的随机实数。
    return min + (max-min)*random_double();
}
```

C++ 传统上没有标准的随机数生成器，但较新版本的 C++ 已经通过` <random> `标头解决了这个问题（根据一些专家的说法这个方法并不完美）。如果你想使用这个方法，你可以通过我们需要的条件获得一个随机数，如下：

```cpp
#include <random>

inline double random_double() {
    static std::uniform_real_distribution<double> distribution(0.0, 1.0);
    static std::mt19937 generator;
    return distribution(generator);
}
```

## 8.2 使用多个样本生成像素

对于由多个样本组成的单个像素，我们将从像素周围区域选择样本，并将结果光（颜色）值进行平均。

首先，我们将更新` write_color() `函数来考虑我们使用的样本数量：我们需要找到所有样本的平均值。为此，我们将在每次迭代中添加完整的颜色，然后在写出颜色之前，在最后进行一次除法（除以样本数量）。为了确保最终结果的颜色分量保持在适当的$$ [0,1] $$范围内，我们将添加并使用一个小的辅助函数：`interval::clamp(x)`。

```cpp
// interval.h
class interval {
  public:
    ...

    bool surrounds(double x) const {
        return min < x && x < max;
    }

    double clamp(double x) const {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }
    ...
};
```

以下是更新后的` write_color() `函数，它取得像素的所有光的总和以及涉及的样本数量：

```cpp
// color.h
void write_color(std::ostream &out, color pixel_color, int samples_per_pixel) {
    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    // 用颜色除以样本数。
    auto scale = 1.0 / samples_per_pixel;
    r *= scale;
    g *= scale;
    b *= scale;

    // 写入每个颜色分量转换后的 [0,255] 值。
    static const interval intensity(0.000, 0.999);
    out << static_cast<int>(256 * intensity.clamp(r)) << ' '
        << static_cast<int>(256 * intensity.clamp(g)) << ' '
        << static_cast<int>(256 * intensity.clamp(b)) << '\n';
}
```

现在让我们更新相机类以定义并使用新的` camera::get_ray(i,j) `函数，该函数将为每个像素生成不同的样本。这个函数将使用一个新的辅助函数` pixel_sample_square()`，该函数在以原点为中心的单位正方形内生成随机样本点。然后，我们将这个理想正方形中的随机样本转换回我们当前正在采样的特定像素。

```cpp
class camera {
  public:
    double aspect_ratio      = 1.0;  // 图像宽度与高度之比
    int    image_width       = 100;  // 以像素为单位的渲染图像宽度
    int    samples_per_pixel = 10;   // 每个像素的随机样本数

    void render(const hittable& world) {
        initialize();

        std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

        for (int j = 0; j < image_height; ++j) {
            std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
            for (int i = 0; i < image_width; ++i) {
                color pixel_color(0,0,0);
                for (int sample = 0; sample < samples_per_pixel; ++sample) {
                    ray r = get_ray(i, j);
                    pixel_color += ray_color(r, world);
                }
                write_color(std::cout, pixel_color, samples_per_pixel);
            }
        }

        std::clog << "\rDone.                 \n";
    }
    ...
  private:
    ...
    void initialize() {
      ...
    }

    ray get_ray(int i, int j) const {
        // 为位置 i,j 处的像素随机获取采样的摄像机光线

        auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
        auto pixel_sample = pixel_center + pixel_sample_square();

        auto ray_origin = center;
        auto ray_direction = pixel_sample - ray_origin;

        return ray(ray_origin, ray_direction);
    }

    vec3 pixel_sample_square() const {
        // 返回原点像素周围正方形中的一个随机点返回原点像素周围正方形中的一个随机点
        auto px = -0.5 + random_double();
        auto py = -0.5 + random_double();
        return (px * pixel_delta_u) + (py * pixel_delta_v);
    }

    ...
};

#endif
```

（除了上面的新` pixel_sample_square() `函数，您还会在 Github 源代码中找到` pixel_sample_disk() `函数。如果您想尝试非正方形像素，可以包含此函数，但我们在本书中不会使用它。`pixel_sample_disk() `依赖于稍后定义的函数 `random_in_unit_disk()`。）

`main`函数更新以设置新的相机参数。

```cpp
// main.cc
int main() {
    ...

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;

    cam.render(world);
}
```

放大生成的图像，我们可以看到边缘像素的差异。

![*抗锯齿之前和之后*](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/image-20231122163154871.png)

# 9. 漫反射材质
