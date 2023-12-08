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

当你放大到目前为止渲染过的图像时，你可能会注意到图像边缘的“阶梯状”特性，这种现象通常被称为“走样”(*aliasing*)或“锯齿”(*jaggies*)。与此相反，真实相机拍摄的照片通常沿着边缘没有锯齿，因为边缘像素是前景和背景的混合。请考虑，与我们渲染的图像不同，真实的世界图像是连续的。换句话说，世界（以及它的任何真实图像）实际上具有无限的分辨率。我们可以通过对每个像素取多个样本的平均值来获得相同的效果。

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

现在我们有了对象和每个像素多条光线，我们可以制作一些看起来逼真的材质。我们将从漫反射材质（*diffuse*，也称为哑光(*matte*)）开始。一个问题是我们是否混合和匹配几何体和材质（以便我们可以将材质分配给多个球体，反之亦然），或者几何体和材质是否紧密结合（这对于几何体和材质链接的程序对象可能很有用））。我们将采用大多数渲染器通常采用的分离方式，但请注意还有其他方法。

## 9.1 简单的漫反射材质

不发光的漫射物体只会呈现周围环境的颜色，但它们确实会用自己的固有颜色来调节周围环境的颜色。从漫反射表面反射的光的方向是随机的，因此，如果我们将三束光线发送到两个漫反射表面之间的裂缝中，它们将具有不同的随机行为：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-1.09-light-bounce-20231128185709736.jpg" alt="光线反射" />

它们也可能被吸收而不是反射。表面越暗，光线被吸收的可能性就越大（这就是为什么它是暗的！）。事实上，任何随机化方向的算法都会产生看起来无光泽的表面。让我们从最直观的办法开始：**在所有方向上随机均匀地反射光线的表面**。对于这种材质，照射到表面的光线在远离表面的任何方向上都有相同的概率反弹。

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-1.10-random-vec-horizon.jpg" alt="地平线上的等距离反射" />

这种非常直观的材质是最简单的漫反射类型，事实上，许多第一批光线追踪论文都使用了这种漫反射方法（在采用我们稍后将实现的更准确的方法之前）。我们目前没有办法随机反射光线，因此我们需要向矢量实用程序标头添加一些函数。我们首先需要的是生成任意随机向量的能力：

```cpp
// vec3.h
class vec3 {
  public:
    ...

    double length_squared() const {
        return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
    }

    static vec3 random() {
        return vec3(random_double(), random_double(), random_double());
    }

    static vec3 random(double min, double max) {
        return vec3(random_double(min,max), random_double(min,max), random_double(min,max));
    }
};
```

然后，我们需要弄清楚如何操纵一个随机向量，以便我们只得到位于半球表面的结果。有一些方法可以做到这一点，但实际上这些方法理解起来出奇的复杂，实现起来也相当复杂。相反，我们将使用通常最简单的算法：排除法。排除法通过重复生成随机样本，直到产生符合所需标准的样本为止。换句话说，不断排除不合适的样本，直到你找到一个好的。

使用排除法在半球上生成随机向量有许多同样有效的方法，但出于我们的目的，我们将使用最简单的方法，即：

1. 在单位球体内部生成随机向量
2. 标准化这个向量
3. 如果标准化后向量落在错误的半球表面上，反转它

首先，我们将使用排除法在单位球体内生成随机向量。在单位立方体内选择一个随机点，其中$$ 𝑥$$、$$𝑦 $$和$$ 𝑧 $$的范围都是从 −1 到 +1，如果这个点在单位球体外部就排除它。

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-1.11-sphere-vec.jpg" alt="在找到一个更好的向量之前，两个向量被舍弃" />

```cpp
// vec3.h
...

inline vec3 unit_vector(vec3 v) {
    return v / v.length();
}

inline vec3 random_in_unit_sphere() {
    while (true) {
        auto p = vec3::random(-1,1);
        if (p.length_squared() < 1)
            return p;
    }
}
```

一旦我们在单位球面上有了一个随机向量，我们就需要对其进行标准化以获得单位球面上的向量。

<img src="https://raytracing.github.io/images/fig-1.12-sphere-unit-vec.jpg" alt="标准化被成功被接受的向量" />

```cpp
// vec3.h
...

inline vec3 random_in_unit_sphere() {
    while (true) {
        auto p = vec3::random(-1,1);
        if (p.length_squared() < 1)
            return p;
    }
}


inline vec3 random_unit_vector() {
    return unit_vector(random_in_unit_sphere());
}
```

现在我们在单位球体的表面上有了一个随机向量，我们可以通过与表面法线进行比较来确定它是否在正确的半球上：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-1.13-surface-normal-20231128191336982.jpg" alt="法向量告诉我们需要哪个半球" />

我们可以采用表面法线和随机向量的点积来确定它是否位于正确的半球中。**如果点积为正，则向量位于正确的半球中。如果点积为负，那么我们需要反转向量。**

```cpp
// vec3.h
...

inline vec3 random_unit_vector() {
    return unit_vector(random_in_unit_sphere());
}


inline vec3 random_on_hemisphere(const vec3& normal) {
    vec3 on_unit_sphere = random_unit_vector();
    if (dot(on_unit_sphere, normal) > 0.0) // In the same hemisphere as the normal
        return on_unit_sphere;
    else
        return -on_unit_sphere;
}
```

如果光线从材质上反射并保持 100% 的颜色，那么我们就说该材质是白色的。如果光线从材质上反射并保留 0% 的颜色，则我们称该材质为黑色。作为漫反射材质的首次演示，我们将设置` ray_color `函数以返回反射颜色的 50%。我们应该期望得到漂亮的灰色。

```cpp
// camera.h
class camera {
  ...
  private:
    ...
    color ray_color(const ray& r, const hittable& world) const {
        hit_record rec;

        if (world.hit(r, interval(0, infinity), rec)) {
            vec3 direction = random_on_hemisphere(rec.normal);
            return 0.5 * ray_color(ray(rec.p, direction), world);        }

        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5*(unit_direction.y() + 1.0);
        return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
    }
};
```

...事实上，我们确实得到了相当漂亮的灰色球体：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-1.07-first-diffuse.png" alt="漫反射球体的第一次渲染" />

## 9.2 限制子光线的数量

这里潜伏着一个潜在的问题。请注意，`ray_color `函数是递归的。什么时候会停止递归？当它无法击中任何东西时。然而，在某些情况下，这可能会很长——长到足以摧毁堆栈。为了防止这种情况，我们限制最大递归深度，在最大深度处不返回任何光线：

```cpp
// camera.h
class camera {
  public:
    double aspect_ratio      = 1.0;  // Ratio of image width over height
    int    image_width       = 100;  // Rendered image width in pixel count
    int    samples_per_pixel = 10;   // Count of random samples for each pixel
    int    max_depth         = 10;   // Maximum number of ray bounces into scene
    void render(const hittable& world) {
        initialize();

        std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

        for (int j = 0; j < image_height; ++j) {
            std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
            for (int i = 0; i < image_width; ++i) {
                color pixel_color(0,0,0);
                for (int sample = 0; sample < samples_per_pixel; ++sample) {
                    ray r = get_ray(i, j);
                    pixel_color += ray_color(r, max_depth, world);                }
                write_color(std::cout, pixel_color, samples_per_pixel);
            }
        }

        std::clog << "\rDone.                 \n";
    }
    ...
  private:
    ...
    color ray_color(const ray& r, int depth, const hittable& world) const {        hit_record rec;


        // If we've exceeded the ray bounce limit, no more light is gathered.
        if (depth <= 0)
            return color(0,0,0);
        if (world.hit(r, interval(0, infinity), rec)) {
            vec3 direction = random_on_hemisphere(rec.normal);
            return 0.5 * ray_color(ray(rec.p, direction), depth-1, world);        }

        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5*(unit_direction.y() + 1.0);
        return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
    }
};
```

更新` main() `函数以使用这个新的深度限制：

```cpp
// main.cc
int main() {
    ...

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.render(world);
}
```

对于这个非常简单的场景，我们应该得到基本相同的结果：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-1.08-second-diffuse.png" alt="有限次反弹的漫反射球体的第二次渲染" />

## 9.3 修复阴影痤疮

我们还需要解决一个微妙的错误。当光线与表面相交时，它会尝试准确计算交点。不幸的是，对我们来说，这种计算容易受到浮点数舍入误差的影响，可能导致交点略有偏差。这意味着下一个光线的起点，即从表面随机散射的光线，不太可能与表面完全平齐。它可能刚好在表面之上，也可能刚好在表面之下。如果光线的起点刚好在表面下方，那么它可能会再次与该表面相交。这意味着它会在$$ 𝑡=0.00000001 $$或者击中函数给出的任何浮点数近似值时找到最近的表面。解决这个问题最简单的方法就是忽略非常接近计算交点的碰撞：

```cpp
// camera.h
class camera {
  ...
  private:
    ...
    color ray_color(const ray& r, int depth, const hittable& world) const {
        hit_record rec;

        // If we've exceeded the ray bounce limit, no more light is gathered.
        if (depth <= 0)
            return color(0,0,0);


        if (world.hit(r, interval(0.001, infinity), rec)) {            vec3 direction = random_on_hemisphere(rec.normal);
            return 0.5 * ray_color(ray(rec.p, direction), depth-1, world);
        }

        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5*(unit_direction.y() + 1.0);
        return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
    }
};
```

这样就解决了阴影痤疮问题。是的，确实是这么叫的。结果如下：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-1.09-no-acne.png" alt="没有阴影痤疮的漫反射球体" />

## 9.4 真正的朗伯反射(*Lambertian Reflection*)

均匀地在半球上散射反射光线可以产生漂亮的柔和漫反射模型，但我们肯定可以做得更好。更准确地表现真实漫反射物体的是朗伯分布(*Lambertian distribution*)。这种分布以与$$ cos\phi $$成比例的方式散射反射光线，其中$$ \phi $$是反射光线与表面法线之间的角度。**这意味着反射光线最有可能在接近表面法线的方向上散射，而在远离法线的方向上散射的可能性较小。**这种非均匀的朗伯分布比我们之前的均匀散射更好地模拟了现实世界中的材料反射。

我们可以通过向法线向量添加一个随机单位向量来创建这x种分布。在表面的交点处，有击中点$$ P $$和表面的法线$$ n$$。在交点处，这个表面恰好有两个侧面，因此任何交点只能有两个唯一的与之相切的单位球体（每个表面的一侧一个）。这两个单位球体将被其半径的长度从表面上移开，对于单位球体来说，这个长度恰好是1。

一个球体将朝着表面的法线$$𝐧$$的方向移开，另一个球体将朝着相反方向（$$−n$$）移开。这给我们留下了两个单位大小的球体，它们只会在交点处与表面刚好接触。由此，一个球体的中心位于（$$P+n$$），另一个球体的中心位于（$$𝐏−𝐧$$）。位于（$$P−n$$）的球体被认为是在表面内部，而中心位于（$$P+n$$）的球体被认为是在表面外部。

我们想要选择与光线原点处于同一侧的切线单位球体。在这个单位半径球体上选择一个随机点 $$S$$，并从击中点 $$P$$ 发送一条光线到随机点 $$S$$（这是向量（$$S−P$$））：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-1.14-rand-unitvec.jpg" alt="根据朗伯分布随机生成向量" />

变化实际上相当小：

```cpp
// camera.h
class camera {
    ...
    color ray_color(const ray& r, int depth, const hittable& world) const {
        hit_record rec;

        // If we've exceeded the ray bounce limit, no more light is gathered.
        if (depth <= 0)
            return color(0,0,0);

        if (world.hit(r, interval(0.001, infinity), rec)) {
            vec3 direction = rec.normal + random_unit_vector();            return 0.5 * ray_color(ray(rec.p, direction), depth-1, world);
    }

        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5*(unit_direction.y() + 1.0);
        return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
    }
};
```

渲染后我们得到类似的图像：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-1.10-correct-lambertian.png" alt="朗伯球体的正确渲染" />

在我们的两个球体组成的简单场景中，很难区分这两种漫反射方法的差异，但你应该能注意到两个重要的视觉差异：

1. 变更后，阴影更加明显
2. 变更后，两个球体都因天空的蓝色而呈现出蓝色。

这些变化都是由于光线的散射更不均匀——更多的光线朝着法线散射。这意味着对于漫反射物体，它们会显得更暗，因为反弹向相机的光线较少。对于阴影，更多的光线直接向上反弹，因此球体下方的区域更暗。

大多数常见的日常物体并不是完全漫反射的，因此我们对这些物体在光照下的行为的直觉可能形成不佳。随着本书中场景的复杂化，鼓励您在这里呈现的不同漫反射渲染器之间切换。大多数感兴趣的场景将包含大量的漫反射材料。通过了解不同漫反射方法对场景照明的影响，你可以获得宝贵的见解。

## 9.5 使用伽马校正进行准确的颜色强度
注意球体下的阴影。图片非常暗，但我们的球体只吸收每次碰撞的一半能量，因此它们是 50% 的反射器。球体应该看起来很亮（在现实生活中是浅灰色的），但它们看起来相当暗。如果我们逐步展示我们的漫反射材料的完整亮度范围，我们可以更清楚地看到这一点。我们首先将` ray_color `函数的反射率从 0.5（50%）设置为 0.1（10%）：

```cpp
// camera.h
class camera {
    ...
    color ray_color(const ray& r, int depth, const hittable& world) const {
        hit_record rec;

        // 如果我们超过了光线反弹的限制，就无法再聚集更多的光线。
        if (depth <= 0)
            return color(0,0,0);

        if (world.hit(r, interval(0.001, infinity), rec)) {
            vec3 direction = rec.normal + random_unit_vector();
            return 0.1 * ray_color(ray(rec.p, direction), depth-1, world);    }

        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5*(unit_direction.y() + 1.0);
        return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
    }
};
```

我们在这个新的 10% 反射率下进行渲染。然后我们将反射率设置为 30%，再次渲染。我们重复 50%、70% 和最终的 90%。你可以在你选择的照片编辑器中从左到右叠加这些图像，你应该会得到一个非常好的视觉呈现，展示你选择的亮度范围的增加。这是我们到目前为止一直在使用的：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-1.11-linear-gamut.png" alt="迄今为止渲染器的色域" />

如果你仔细观察，或者使用颜色拾取器，你会注意到 50% 反射率的渲染（中间的那个）比白色和黑色之间的中间灰色（中灰色）要暗得多。事实上，70% 的反射器更接近中灰色。造成这种情况的原因是，几乎所有的计算机程序都假设图像在写入图像文件之前已经进行了“伽马校正”(*gamma corrected*)。这意味着 0 到 1 的值在存储为字节之前应用了某种转换。没有进行转换的数据写入的图像被称为线性空间(*linear space*)中的图像，而进行了转换的图像被称为伽马空间(*gamma space*)中的图像。你正在使用的图像查看器可能期望图像在伽马空间中，但我们给它的是线性空间中的图像。这就是我们的图像看起来不准确地暗的原因。

图像应该存储在伽马空间中有很多好的理由，但对于我们的目的，我们只需要意识到这一点。我们将把我们的数据转换到伽马空间，以便我们的图像查看器可以更准确地显示我们的图像。作为一个简单的近似，我们可以使用`gamma 2`作为我们的转换，这是从伽马空间到线性空间转换时使用的指数。我们需要从线性空间转换到伽马空间，这意味着取`gamma 2`的逆，即指数为$$ 1/gamma$$，这只是平方根。

```cpp
// color.h
inline double linear_to_gamma(double linear_component)
{
    return sqrt(linear_component);
}
void write_color(std::ostream &out, color pixel_color, int samples_per_pixel) {
    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    // 用颜色除以样本数。
    auto scale = 1.0 / samples_per_pixel;
    r *= scale;
    g *= scale;
    b *= scale;


    // 线性到伽玛变换。
    r = linear_to_gamma(r);
    g = linear_to_gamma(g);
    b = linear_to_gamma(b);
    
    // 写入每个颜色分量的转换后的 [0,255] 值。
    static const interval intensity(0.000, 0.999);
    out << static_cast<int>(256 * intensity.clamp(r)) << ' '
        << static_cast<int>(256 * intensity.clamp(g)) << ' '
        << static_cast<int>(256 * intensity.clamp(b)) << '\n';
}
```

使用这种伽马校正，我们现在得到了一个从暗到亮更加一致的渐变：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-1.12-gamma-gamut.png" alt="*两个漫反射球体的伽马校正渲染*" />

# 10. 金属

## 10.1 材料的抽象类

如果我们想让不同的物体拥有不同的材料，我们面临一个设计决策。我们可以拥有一个带有许多参数的通用材料类型，以便任何单一材料类型都可以忽略不影响它的参数。这不是一个坏方法。或者我们可以有一个封装独特行为的抽象材料类。我更喜欢后一种方法。对于我们的程序来说，材料需要做两件事：

1. 产生一个散射光线（或者说它吸收了入射光线）。
2. 如果散射了，说明光线应该被衰减多少。

这就是抽象类的定义：

```cpp
// material.h
#ifndef MATERIAL_H
#define MATERIAL_H

#include "rtweekend.h"

class hit_record;

class material {
  public:
    virtual ~material() = default;

    virtual bool scatter(
        const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const = 0;
};

#endif
```

## 10.2 描述光线与物体相交的数据结构

`hit_record `是为了避免一堆参数，所以我们可以把任何我们想要的信息放进去。你可以用参数代替封装类型，这只是个人喜好的问题。可撞击物和材料需要能够在代码中引用对方的类型，所以存在一些引用的循环性。在 C++ 中，我们添加了一行` class material`; 来告诉编译器` material `是一个稍后会定义的类。因为我们只是指定了一个指向类的指针，编译器不需要知道类的细节，这解决了循环引用的问题。

```cpp
// hittable.h
#include "rtweekend.h"

class material;
class hit_record {
  public:
    point3 p;
    vec3 normal;    
    shared_ptr<material> mat;    
    double t;
    bool front_face;

    void set_face_normal(const ray& r, const vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};
```

`hit_record `只是一种将一堆参数塞入一个类的方法，这样我们就可以把它们作为一个组发送。当一条光线撞击一个表面（例如一个特定的球体）时，`hit_record `中的材料指针将被设置为指向在` main() `中设置球体时给予的材料指针。当 `ray_color() `程序获取` hit_record `时，它可以调用材料指针的成员函数来找出是否有光线被散射。

为了实现这一点，`hit_record `需要被告知分配给球体的材料。

```cpp
// shpere.h
class sphere : public hittable {
  public:
    sphere(point3 _center, double _radius, shared_ptr<material> _material)
      : center(_center), radius(_radius), mat(_material) {}

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        ...

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        rec.mat = mat;

        return true;
    }

  private:
    point3 center;
    double radius;
    shared_ptr<material> mat;
};
```

## 10.3 模拟光的散射和反射

对于我们已经拥有的漫反射(*Lambertian*)情况，它可以始终散射并通过其反射率$$ R $$进行衰减，或者它可以有时散射（概率为$$ 1−R$$）并不进行衰减（其中没有散射的光线就被材料吸收）。它也可以是这两种策略的混合。我们选择始终散射，所以*Lambertian*材料变成了这样一个简单的类：

```cpp
// material.h
class material {
    ...
};

class lambertian : public material {
  public:
    lambertian(const color& a) : albedo(a) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        auto scatter_direction = rec.normal + random_unit_vector();
        scattered = ray(rec.p, scatter_direction);
        attenuation = albedo;
        return true;
    }

  private:
    color albedo;
};
```

请注意我们可以选择以某种固定概率$$ p $$散射，并且让衰减为$ albedo/p $的第三个选项。由你选择。

如果你仔细阅读上面的代码，你会注意到一个小小的恶作剧。如果我们生成的随机单位向量与法线向量完全相反，两者将相加为零，这将导致零散射方向向量。这会导致后续的糟糕情况（无穷大和` NaN`），所以我们需要在传递之前拦截这个条件。

为此，我们将创建一个新的向量方法 —` vec3::near_zero() `— 如果向量在所有维度上非常接近零，则返回` true`。

```cpp
// vec3.h
class vec3 {
    ...

    double length_squared() const {
        return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
    }

    bool near_zero() const {
        // Return true if the vector is close to zero in all dimensions.
        auto s = 1e-8;
        return (fabs(e[0]) < s) && (fabs(e[1]) < s) && (fabs(e[2]) < s);
    }
    ...
};
```

```cpp
// material.h
class lambertian : public material {
  public:
    lambertian(const color& a) : albedo(a) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        auto scatter_direction = rec.normal + random_unit_vector();

        // Catch degenerate scatter direction
        if (scatter_direction.near_zero())
            scatter_direction = rec.normal;

        scattered = ray(rec.p, scatter_direction);
        attenuation = albedo;
        return true;
    }

  private:
    color albedo;
};
```

## 10.4 镜面反射

对于抛光金属，光线不会随机散射。关键问题是：光线如何从金属镜面反射？向量数学在这里是我们的朋友：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-1.15-reflection.jpg" alt="光线反射" />

红色的反射光线方向只是$$ v+2b$$。在我们的设计中，$$n $$是单位向量，但$$ v $$可能不是。$$b $$的长度应该是$$ v⋅n$$。因为$$ v $$指向内侧，我们需要一个负号，得到：

```cpp
// vec3.h
...

inline vec3 random_on_hemisphere(const vec3& normal) {
    ...
}

vec3 reflect(const vec3& v, const vec3& n) {
    return v - 2*dot(v,n)*n;
}

...
```

金属材料仅使用以下公式反射光线：

```cpp
// material.h
...

class lambertian : public material {
    ...
};

class metal : public material {
  public:
    metal(const color& a) : albedo(a) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
        scattered = ray(rec.p, reflected);
        attenuation = albedo;
        return true;
    }

  private:
    color albedo;
};
```

我们需要修改` ray_color() `函数来进行所有更改：

```cpp
// camera.h
...
#include "rtweekend.h"

#include "color.h"
#include "hittable.h"
#include "material.h"
...

class camera {
  ...
  private:
    ...
    color ray_color(const ray& r, int depth, const hittable& world) const {
        hit_record rec;

        // 如果我们超过了光线反弹的限制，就无法再聚集更多的光线
        if (depth <= 0)
            return color(0,0,0);

        if (world.hit(r, interval(0.001, infinity), rec)) {
            ray scattered;
            color attenuation;
            if (rec.mat->scatter(r, rec, attenuation, scattered))
                return attenuation * ray_color(scattered, depth-1, world);
            return color(0,0,0);
        }

        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5*(unit_direction.y() + 1.0);
        return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
    }
};
```

## 10.5 金属球场景

现在让我们在场景中添加一些金属球：

```cpp
// main.cc
#include "rtweekend.h"

#include "camera.h"
#include "color.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"

int main() {
    hittable_list world;

    auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
    auto material_center = make_shared<lambertian>(color(0.7, 0.3, 0.3));
    auto material_left   = make_shared<metal>(color(0.8, 0.8, 0.8));
    auto material_right  = make_shared<metal>(color(0.8, 0.6, 0.2));

    world.add(make_shared<sphere>(point3( 0.0, -100.5, -1.0), 100.0, material_ground));
    world.add(make_shared<sphere>(point3( 0.0,    0.0, -1.0),   0.5, material_center));
    world.add(make_shared<sphere>(point3(-1.0,    0.0, -1.0),   0.5, material_left));
    world.add(make_shared<sphere>(point3( 1.0,    0.0, -1.0),   0.5, material_right));
    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;

    cam.render(world);
}
```

这会得到：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-1.13-metal-shiny.png" alt="img" />

## 10.6 模糊反射

我们还可以通过使用小球体并为光线选择新端点来随机化反射方向。我们将使用以原始端点为中心的球体表面的随机点，并按模糊因子进行缩放。

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-1.16-reflect-fuzzy.jpg" alt="生成模糊反射光线" />

球体越大，反射就会越模糊。这意味着可以增加一个模糊度参数，即球体的半径（因此零表示没有扰动）。问题是对于大球体或切线光线，我们可能会在表面以下散射。我们可以让表面吸收这些光线。

```cpp
// material.h
class metal : public material {
  public:
    metal(const color& a, double f) : albedo(a), fuzz(f < 1 ? f : 1) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
        scattered = ray(rec.p, reflected + fuzz*random_unit_vector());
        attenuation = albedo;
        return (dot(scattered.direction(), rec.normal) > 0);
    }

  private:
    color albedo;
    double fuzz;
};
```

我们可以通过向金属添加模糊度 0.3 和 1.0 来尝试：

```cpp
// main.cc
int main() {
    ...
    auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
    auto material_center = make_shared<lambertian>(color(0.7, 0.3, 0.3));
    auto material_left   = make_shared<metal>(color(0.8, 0.8, 0.8), 0.3);
    auto material_right  = make_shared<metal>(color(0.8, 0.6, 0.2), 1.0);
    ...
}
```

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-1.14-metal-fuzz.png" alt="模糊金属" style="zoom:150%;" />

# 11. 电介质

水、玻璃和金刚石等透明材料是电介质。当光线照射到它们时，它会分裂成反射光线和折射（透射）光线。我们将通过在反射和折射之间随机选择来处理这个问题，每次交互只生成一条散射射线。

## 11.1 折射

最难调试的部分是折射光线。如果存在折射光线，我通常会先让所有光线发生折射。在这个项目中，我尝试在场景中放置两个玻璃球，结果是这样的（我还没有告诉你这样做的对错，但很快就会告诉你！）：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-1.15-glass-first.png" alt="先放玻璃球" style="zoom:300%;" />

是这样吗？玻璃球在现实生活中看起来很奇怪。但不，这是不对的。世界应该是颠倒的，不应该有奇怪的黑色东西。我刚把射线从图像中间直接打印出来，结果明显不对。这通常就能解决问题。

## 11.2 斯涅尔定律(*Snell’s Law*)

折射由斯涅尔定律描述：
$$
\eta⋅sin\theta=\eta^′⋅sin\theta^′
$$
其中$$ 𝜃 $$和$$ 𝜃^′ $$是**与法线的角度**，$$𝜂 $$和$$ 𝜂^′（$$发音为“eta”和“eta prime”）是**折射率**（通常空气=1.0，玻璃=1.3–1.7，钻石=2.4）。几何形状如下：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-1.17-refraction.jpg" alt="光线折射" />

为了确定折射光线的方向，我们必须求解$$ sin𝜃^\prime$$：

$$
sin𝜃^′=\frac{𝜂}{𝜂^′}⋅sin𝜃
$$
在表面的折射侧，有一条折射光线$$ 𝐑^′ $$和一个法线$$ 𝐧^′$$，它们之间存在一个角度$$ 𝜃^′$$。我们可以将$$ 𝐑^′ $$分解为与$$ 𝐧^′ $$垂直和平行的光线部分：
$$
R^′=R^′_\perp+R^′_\parallel
$$
如果我们求解$$ 𝐑^′_⊥ $$和 $$𝐑^′_∥$$，我们得到：
$$
R^′_\perp=\frac{\eta}{\eta^′}(R+cos\theta n) \\
R^′_\parallel=−\sqrt{1−|R^′_\perp|^2n}
$$

如果你想的话，你可以自己证明这一点，但我们将把它当作事实并继续。本书的其余部分不需要你理解这个证明。

我们知道右侧每个项的值，除了$$ cos𝜃$$。众所周知，两个向量的点积可以用它们之间的夹角的余弦来解释：

$$
a⋅b=|a||b|cos\theta
$$

如果我们限制$$ 𝐚 $$和$$ 𝐛 $$为单位向量：
$$
a⋅b=cos\theta
$$
我们现在可以用已知量重写$$ 𝐑^′_⊥$$：
$$
R^′_\perp=\frac{\eta}{\eta^′}(R+(−R⋅n)n)
$$

当我们将它们重新组合在一起时，我们可以编写一个函数来计算$$𝐑^′$$：

```cpp
// vec3.h
...

inline vec3 reflect(const vec3& v, const vec3& n) {
    return v - 2*dot(v,n)*n;
}

inline vec3 refract(const vec3& uv, const vec3& n, double etai_over_etat) {
    auto cos_theta = fmin(dot(-uv, n), 1.0);
    vec3 r_out_perp =  etai_over_etat * (uv + cos_theta*n);
    vec3 r_out_parallel = -sqrt(fabs(1.0 - r_out_perp.length_squared())) * n;
    return r_out_perp + r_out_parallel;
}
```

而总是发生折射的电介质材料是：

```cpp
// material.h
...

class metal : public material {
    ...
};

class dielectric : public material {
  public:
    dielectric(double index_of_refraction) : ir(index_of_refraction) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        attenuation = color(1.0, 1.0, 1.0);
        double refraction_ratio = rec.front_face ? (1.0/ir) : ir;

        vec3 unit_direction = unit_vector(r_in.direction());
        vec3 refracted = refract(unit_direction, rec.normal, refraction_ratio);

        scattered = ray(rec.p, refracted);
        return true;
    }

  private:
    double ir; // Index of Refraction
};
```

现在我们将更新场景，将左侧和中心的球体更改为玻璃：

```cpp
// main.cc
auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
auto material_center = make_shared<dielectric>(1.5);
auto material_left   = make_shared<dielectric>(1.5);
auto material_right  = make_shared<metal>(color(0.8, 0.6, 0.2),1.0);
```

这会得到以下结果：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-1.16-glass-always-refract.png" alt="始终折射的玻璃球" />

## 11.3 全内反射

那绝对看起来不对劲。一个棘手的实际问题是，当光线处于具有较高折射率的材料中时，斯涅尔定律没有实数解，因此不可能发生折射。如果我们回顾斯涅尔定律和$$ sin\theta^′ $$的推导：
$$
sin\theta^′=\frac{\eta}{\eta^′}⋅sin\theta
$$
如果光线在玻璃内部，而外部是空气（$$𝜂=1.5，𝜂^′=1.0$$）：
$$
sin\theta^′=\frac{1.5}{1.0}⋅sin\theta
$$
$$sin𝜃^′$$的值不能大于 1。所以，如果 
$$
\frac{1.5}{1.0}⋅sin\theta>1.0
$$
方程两边的等式被打破，解不存在。如果解不存在，玻璃就不能折射，因此必须反射光线：

```cpp
// material.h
if (refraction_ratio * sin_theta > 1.0) {
    // 必须反射
    ...
} else {
    // 可以反射
    ...
}
```

这里所有的光都被反射，因为在实践中这通常发生在固体物体内部，所以称为“全内反射”（*total internal reflection*）。这就是为什么有时当你浸在水中时，水-空气边界就像一面完美的镜子。

我们可以使用三角函数的特性求解 $$sin\theta$$：
$$
sin\theta=\sqrt{1−cos^2\theta}
$$
和
$$
cos\theta=R⋅n
$$

```cpp
// material.h
double cos_theta = fmin(dot(-unit_direction, rec.normal), 1.0);
double sin_theta = sqrt(1.0 - cos_theta*cos_theta);

if (refraction_ratio * sin_theta > 1.0) {
    // Must Reflect
    ...
} else {
    // Can Refract
    ...
}
```

而总是折射的电介质材料（在可能的情况下）是：

```cpp
// material.h
class dielectric : public material {
  public:
    dielectric(double index_of_refraction) : ir(index_of_refraction) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        attenuation = color(1.0, 1.0, 1.0);
        double refraction_ratio = rec.front_face ? (1.0/ir) : ir;

        vec3 unit_direction = unit_vector(r_in.direction());        
        double cos_theta = fmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = sqrt(1.0 - cos_theta*cos_theta);

        bool cannot_refract = refraction_ratio * sin_theta > 1.0;
        vec3 direction;

        if (cannot_refract)
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, refraction_ratio);

        scattered = ray(rec.p, direction);        
        return true;
    }

  private:
    double ir; // Index of Refraction
};
```

衰减始终为 1 —— 玻璃表面什么也不吸收。如果我们用这些参数试一试：

```cpp
// main.cc
auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
auto material_center = make_shared<lambertian>(color(0.1, 0.2, 0.5));
auto material_left   = make_shared<dielectric>(1.5);
auto material_right  = make_shared<metal>(color(0.8, 0.6, 0.2), 0.0);
```

我们可以得到：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-1.17-glass-sometimes-refract.png" alt="*有时会折射的玻璃球*" />

## 11.4 Schlick 近似 

现实中的玻璃反射率随角度变化 —— 当你从陡峭的角度看窗户时，它会变成镜子。有一个复杂的公式来描述这一点，但几乎每个人都使用 *Christophe Schlick* 提出的一种廉价且出奇准确的多项式近似。这就产生了我们完整的玻璃材料：

```cpp
// material.h
class dielectric : public material {
  public:
    dielectric(double index_of_refraction) : ir(index_of_refraction) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        attenuation = color(1.0, 1.0, 1.0);
        double refraction_ratio = rec.front_face ? (1.0/ir) : ir;

        vec3 unit_direction = unit_vector(r_in.direction());
        double cos_theta = fmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = sqrt(1.0 - cos_theta*cos_theta);

        bool cannot_refract = refraction_ratio * sin_theta > 1.0;
        vec3 direction;
        if (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_double())
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, refraction_ratio);

        scattered = ray(rec.p, direction);
        return true;
    }

  private:
    double ir; // Index of Refraction

    static double reflectance(double cosine, double ref_idx) {
        // 使用Schlick近似法计算反射率
        auto r0 = (1-ref_idx) / (1+ref_idx);
        r0 = r0*r0;
        return r0 + (1-r0)*pow((1 - cosine),5);
    }};
```

## 11.5 模拟空心玻璃球 

对于电介质球体，一个有趣且简单的技巧是注意到，如果你使用负半径，几何形状不受影响，但表面法线向内。这可以用作制作空心玻璃球的气泡：

```cpp
// main.cc
...
world.add(make_shared<sphere>(point3( 0.0, -100.5, -1.0), 100.0, material_ground));
world.add(make_shared<sphere>(point3( 0.0,    0.0, -1.0),   0.5, material_center));
world.add(make_shared<sphere>(point3(-1.0,    0.0, -1.0),   0.5, material_left));world.add(make_shared<sphere>(point3(-1.0,    0.0, -1.0),  -0.4, material_left));world.add(make_shared<sphere>(point3( 1.0,    0.0, -1.0),   0.5, material_right));
...
```

这会得到：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-1.18-glass-hollow.png" alt="*空心玻璃球*" />

# 12 可定位相机

像电介质一样，摄像机也很难调试，所以我总是逐步开发它们。首先，让我们允许调整视场角（*fov*）。这是渲染图像边缘到边缘的视觉角度。由于我们的图像不是正方形，水平和垂直的 fov 不同。我总是使用垂直 fov。我通常还会以**度**为单位指定它，并在构造函数内部转换为**弧度** — 这是个人喜好。

## 12.1 摄像机观看几何

首先，我们将保持光线从原点出发，朝向 $$ z=−1 $$ 平面。我们可以把它设置为 $$z=−2$$ 平面，或其他什么平面，只要我们使 $$h$$ 成为与该距离的比率。这是我们的设置：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-1.18-cam-view-geom.jpg" alt="摄像机观察几何（侧面）" />

这意味着 $$h=tan(\frac{\theta}{2})$$。我们的摄像机现在变成了：

```cpp
// camera.h
class camera {
  public:
    double aspect_ratio      = 1.0;  // 图像宽度与高度之比
    int    image_width       = 100;  // 以像素为单位的渲染图像宽度
    int    samples_per_pixel = 10;   // 每个像素的随机样本数
    int    max_depth         = 10;   // 光线进入场景的最大反弹次数

    double vfov = 90;  // 垂直视角（视场）
    void render(const hittable& world) {
    ...

  private:
    ...

    void initialize() {
        image_height = static_cast<int>(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        center = point3(0, 0, 0);

        // Determine viewport dimensions.
        auto focal_length = 1.0;        auto theta = degrees_to_radians(vfov);
        auto h = tan(theta/2);
        auto viewport_height = 2 * h * focal_length;        auto viewport_width = viewport_height * (static_cast<double>(image_width)/image_height);

        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        auto viewport_u = vec3(viewport_width, 0, 0);
        auto viewport_v = vec3(0, -viewport_height, 0);

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // Calculate the location of the upper left pixel.
        auto viewport_upper_left =
            center - vec3(0, 0, focal_length) - viewport_u/2 - viewport_v/2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
    }

    ...
};
```

我们将使用两个接触球体的简单场景来测试这些更改，使用 90° 的视场角。

```cpp
// main.cc
int main() {
    hittable_list world;

    auto R = cos(pi/4);

    auto material_left  = make_shared<lambertian>(color(0,0,1));
    auto material_right = make_shared<lambertian>(color(1,0,0));

    world.add(make_shared<sphere>(point3(-R, 0, -1), R, material_left));
    world.add(make_shared<sphere>(point3( R, 0, -1), R, material_right));
    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;

    cam.vfov = 90;
    cam.render(world);
}
```

这给了我们这样的渲染：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-1.19-wide-view.png" alt="*广角视图*" />

## 12.2 定位和定向摄像机 

为了获得任意视点，首先让我们命名我们关心的点。我们将放置摄像机的位置称为 `lookfrom`，我们观看的点称为 `lookat`。（稍后，如果你愿意，你可以定义一个观看的方向，而不是一个观看的点。）

我们还需要一种指定摄像机的横滚或侧倾的方式：围绕 `lookat-lookfrom` 轴的旋转。另一种思考方式是，即使你保持 `lookfrom` 和 `lookat` 不变，你仍然可以围绕你的鼻子旋转你的头。我们需要的是一种为摄像机指定“向上”向量的方式。

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-1.19-cam-view-dir.jpg" alt="*相机视图方向*" />

我们可以指定任何我们想要的向上向量，只要它不平行于视线方向。将这个向上的向量投影到与视线方向正交的平面上，以获得相对于摄像机的上方向量。我使用将其命名为“视图上方”（*vup*）向量的常见惯例。经过几次叉积和向量标准化后，我们现在有了一个完整的正交基（$$u,v,w$$）来描述我们摄像机的方向。$$u$$ 是指向摄像机右侧的单位向量，$$v$$ 是指向摄像机上方的单位向量，$$w$$ 是指向与视线方向相反的单位向量（因为我们使用右手坐标系），摄像机中心位于原点。

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-1.20-cam-view-up.jpg" alt="*相机视图向上方向*" />

像之前我们的固定摄像机面向 $$−Z$$ 一样，我们任意视角的摄像机面向 $$−w$$。请记住，我们可以，但不必使用$$(0,1,0)$$来指定 vup。这很方便，会自然地保持你的摄像机水平，直到你决定疯狂的尝试摄像机角度。

```cpp
// camera.h
class camera {
  public:
    double aspect_ratio      = 1.0;  // 图像宽度与高度之比
    int    image_width       = 100;  // 以像素为单位的渲染图像宽度
    int    samples_per_pixel = 10;   // 每个像素的随机样本数
    int    max_depth         = 10;   // 进入场景的最大射线反弹次数

    double vfov     = 90;              // 垂直视角(视场)   
    point3 lookfrom = point3(0,0,-1);  // 点摄像机从这开始看
    point3 lookat   = point3(0,0,0);   // 点摄像机看向这里
    vec3   vup      = vec3(0,1,0);     // 相机相对 "向上 "的方向
    ...

  private:
    int    image_height;   // 渲染图像高度
    point3 center;         // 摄像机中心
    point3 pixel00_loc;    // 像素(0,0)的位置
    vec3   pixel_delta_u;  // 向右偏移像素
    vec3   pixel_delta_v;  // 向下偏移像素
    vec3   u, v, w;        // 摄像机帧基向量
    
    void initialize() {
        image_height = static_cast<int>(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        center = lookfrom;
        
        // 确定视口尺寸.        
        auto focal_length = (lookfrom - lookat).length();        
        auto theta = degrees_to_radians(vfov);
        auto h = tan(theta/2);
        auto viewport_height = 2 * h * focal_length;
        auto viewport_width = viewport_height * (static_cast<double>(image_width)/image_height);

        // 计算摄像机坐标系的 u、v、w 单位基向量
        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);
        
        // 计算水平方向和垂直视口边缘的向量。 
        vec3 viewport_u = viewport_width * u;    // 视口水平边缘的向量
        vec3 viewport_v = viewport_height * -v;  // 视口垂直边缘的向量
        
        // 计算像素间的水平和垂直增向量。
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // 计算左上角像素的位置。
        auto viewport_upper_left = center - (focal_length * w) - viewport_u/2 - viewport_v/2; 
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
    }

    ...

  private:
};
```

我们将切换回之前的场景，并使用新的视点：

```cpp
// main.cc
int main() {
    hittable_list world;

    auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
    auto material_center = make_shared<lambertian>(color(0.1, 0.2, 0.5));
    auto material_left   = make_shared<dielectric>(1.5);
    auto material_right  = make_shared<metal>(color(0.8, 0.6, 0.2), 0.0);

    world.add(make_shared<sphere>(point3( 0.0, -100.5, -1.0), 100.0, material_ground));
    world.add(make_shared<sphere>(point3( 0.0,    0.0, -1.0),   0.5, material_center));
    world.add(make_shared<sphere>(point3(-1.0,    0.0, -1.0),   0.5, material_left));
    world.add(make_shared<sphere>(point3(-1.0,    0.0, -1.0),  -0.4, material_left));
    world.add(make_shared<sphere>(point3( 1.0,    0.0, -1.0),   0.5, material_right));
    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;

    cam.vfov     = 90;
    cam.lookfrom = point3(-2,2,1);
    cam.lookat   = point3(0,0,-1);
    cam.vup      = vec3(0,1,0);
    cam.render(world);
}
```

可以得到：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-1.20-view-distant.png" alt="远景" />

我们可以改变视野：

```cpp
// main.cc  
cam.vfov     = 20;
```

可以得到：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-1.21-view-zoom.png" alt="放大" />

## 13. 虚焦模糊

现在我们来到最后一个特性：**虚焦模糊**（*defocus*）。注意，摄影师称这为**景深**(*depth of field*)，所以确保只在你的光线追踪中使用**“虚焦模糊”**这个术语。

真实相机中出现虚焦模糊的原因是因为它们需要一个大孔（而不仅仅是一个针孔）来收集光线。一个大孔会使一切失焦，但如果我们在胶片或传感器前放一个透镜，就会有一定的距离让所有东西都聚焦。放置在该距离处的物体会显得聚焦，并且距离该距离越远，就会线性地显得越模糊。你可以这样想象一个透镜：来自焦距处特定点并到达镜头的所有光线将被弯曲回图像传感器上的单个点。

我们称相机中心与一切都完美聚焦的平面之间的距离为**焦距**（*focus distance*）。请注意，焦距通常不同于焦长 — **焦长**(*focus length*)是相机中心与图像平面之间的距离。然而，对于我们的模型，这两者将具有相同的值，因为我们将像素网格放在焦平面上，该平面距离相机中心的距离就是焦距。

在实体相机中，通过透镜与胶片/传感器之间的距离来控制焦距。这就是为什么当你改变焦点时，你会看到透镜相对于相机移动的原因（在你的手机相机中也可能发生，但只是传感器移动）。光圈是一个控制透镜实际大小的孔。对于实体相机，如果你需要更多光线，你会使光圈变大，并且会得到更多远离焦距的物体的模糊。对于我们的虚拟相机，我们可以拥有完美的传感器，永远不需要更多的光线，所以我们只在需要虚焦模糊时使用光圈。

## 13.1 薄透镜近似

真实相机有一个复杂的复合透镜。对于我们的代码，我们可以模拟顺序：传感器，然后是透镜，然后是光圈。然后我们可以弄清楚在哪里发送光线，并在计算后翻转图像（图像被投影在胶片上是倒置的）。然而，图形学人员通常使用薄透镜近似：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-1.21-cam-lens.jpg" alt="*相机镜头模型*" />

我们不需要模拟相机的任何内部 — 为了渲染相机外部的图像，那将是不必要的复杂工作。相反，我通常从一个无限薄的圆形“透镜”开始发射光线，并将它们发送到焦平面（距离透镜焦长）上的像素处，3D世界中该平面上的所有东西都完美聚焦。

在实践中，我们通过将视口放置在这个平面上来实现这一点。将所有内容放在一起：

1. 焦平面与相机视线方向垂直。 
2. 焦距是相机中心与焦平面之间的距离。 
3. 视口位于焦平面上，以相机视线方向向量为中心。 
4. 像素位置网格位于视口内（位于 3D 世界中）。 
5. 从当前像素位置周围区域选择随机图像采样位置。 
6. 相机从透镜上的随机点发射光线，穿过当前图像采样位置。

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/fig-1.22-cam-film-plane.jpg" alt="*相机焦平面*" />

## 13.2 生成采样光线

没有虚焦模糊的情况下，所有场景光线都起源于`camera center`（或 `lookfrom`）。为了实现虚焦模糊，我们在相机中心构造一个圆盘。半径越大，虚焦模糊越大。你可以将我们原始的相机想象为拥有半径为零的虚焦圆盘（根本没有模糊），因此所有光线都始于圆盘中心（`lookfrom`）。

那么，虚焦圆盘应该有多大呢？由于这个圆盘的大小控制我们获得多少虚焦模糊，这应该是相机类的一个参数。我们可以将圆盘的半径作为相机参数，但模糊会根据投影距离而变化。一个稍微简单的参数是指定在视口中心顶点和位于相机中心的基座（虚焦圆盘）之间的锥角。这应该在你为特定拍摄变化焦距时给你更一致的结果。

由于我们将从虚焦圆盘上选择随机点，我们需要一个函数来做到这一点：`random_in_unit_disk()`。这个函数使用我们在 `random_in_unit_sphere()` 中使用的相同类型的方法，只是用于两个维度。

```cpp
// vec3.h
inline vec3 unit_vector(vec3 u) {
    return v / v.length();
}

inline vec3 random_in_unit_disk() {
    while (true) {
        auto p = vec3(random_double(-1,1), random_double(-1,1), 0);
        if (p.length_squared() < 1)
            return p;
    }
}
```

现在让我们更新相机，使光线从虚焦圆盘发射：

```cpp
// camera.h
class camera {
  public:
    double aspect_ratio      = 1.0;  // 图像宽度与高度的比率
    int    image_width       = 100;  // 以像素为单位的渲染图像宽度
    int    samples_per_pixel = 10;   // 每个像素的随机样本数
    int    max_depth         = 10;   // 射线进入场景的最大反弹次数

    double vfov     = 90;              // 垂直视角（视野）
    point3 lookfrom = point3(0,0,-1);  // 摄像机从哪个点看哪个点
    point3 lookat   = point3(0,0,0);   // 摄像机正对的点
    vec3   vup      = vec3(0,1,0);     // 摄像机相对 "向上 "的方向

    double defocus_angle = 0;  // 通过每个像素的光线的变化角度
    double focus_dist = 10;    // 从摄像机观察点到完全聚焦平面的距离
    ...

  private:
    int    image_height;    // 渲染图像的高度
    point3 center;          // 摄像机中心
    point3 pixel00_loc;     // 像素 0, 0 的位置
    vec3   pixel_delta_u;   // 像素向右的偏移量
    vec3   pixel_delta_v;   // 向下方像素的偏移量
    vec3   u, v, w;         // 摄像机帧基向量
    vec3   defocus_disk_u;  // 聚焦盘水平半径
    vec3   defocus_disk_v;  // 聚焦盘垂直半径
    void initialize() {
        image_height = static_cast<int>(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        center = lookfrom;

        // 确定视口尺寸.        
        // 删除 auto focal_length = (lookfrom - lookat).length();        
        auto theta = degrees_to_radians(vfov);
        auto h = tan(theta/2);       
        auto viewport_height = 2 * h * focus_dist;        
        auto viewport_width = viewport_height * (static_cast<double>(image_width)/image_height);

        // 计算摄像机坐标系的 u、v、w 单位基向量
        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        // 计算横向和纵向视口边缘的向量
        vec3 viewport_u = viewport_width * u;    // 跨视口水平边缘的向量
        vec3 viewport_v = viewport_height * -v;  // 视口垂直边缘向下的向量

        // 计算到下一个像素的水平和垂直增量向量。
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // 计算左上角像素的位置      
        auto viewport_upper_left = center - (focus_dist * w) - viewport_u/2 - viewport_v/2;      
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

        // 计算摄像机离焦盘基向量。
        auto defocus_radius = focus_dist * tan(degrees_to_radians(defocus_angle / 2));
        defocus_disk_u = u * defocus_radius;
        defocus_disk_v = v * defocus_radius;    }


    ray get_ray(int i, int j) const {        
        // 为位置 i,j 处的像素获取随机采样的摄像机光线，该光线源自摄像机散焦盘
        auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
        auto pixel_sample = pixel_center + pixel_sample_square();

        auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();        
        auto ray_direction = pixel_sample - ray_origin;

        return ray(ray_origin, ray_direction);
    }

    ...
    point3 defocus_disk_sample() const {
        // 返回摄像机离焦盘中的一个随机点
        auto p = random_in_unit_disk();
        return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
    }
    color ray_color(const ray& r, int depth, const hittable& world) const {
    ...
};
```

使用大光圈：

```cpp
// main.cc
int main() {
    ...

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;

    cam.vfov     = 20;
    cam.lookfrom = point3(-2,2,1);
    cam.lookat   = point3(0,0,-1);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 10.0;
    cam.focus_dist    = 3.4;
    cam.render(world);
}
```

可以得到：

<img src="https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-1.22-depth-of-field.png" alt="具有景深的球体" />

# 14. Where Next?

## 14.1 最终渲染

让我们制作本书封面上的图片————许多随机的球体。

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
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = make_shared<dielectric>(1.5);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));
    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 1200;
    cam.samples_per_pixel = 500;
    cam.max_depth         = 50;

    cam.vfov     = 20;
    cam.lookfrom = point3(13,2,3);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0.6;
    cam.focus_dist    = 10.0;
    cam.render(world);
}
```

（请注意，上面的代码与项目示例代码略有不同：上面的`samples_per_pixel`设置为500以获得高质量图像，这将需要相当长的时间来渲染。示例代码使用值10是为了合理运行开发和验证时的时间。）

这会得到：

![最终场景](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/img-1.23-book1-final.jpg)

您可能会注意到的一件有趣的事情是，玻璃球实际上没有阴影，这使它们看起来像是漂浮的。这不是一个错误——你在现实生活中很少看到玻璃球，它们看起来也有点奇怪，而且在阴天似乎确实漂浮着。玻璃球下方的大球体上的一个点仍然有大量的光线照射到它，因为天空被重新排序而不是被阻挡。
