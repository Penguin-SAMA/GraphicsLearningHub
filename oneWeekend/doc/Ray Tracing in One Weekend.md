> 声明：此文章仅供个人学习，由于英文水平有限，本文为我个人基于翻译插件加以优化所得，会尽可能还原原意。如果各位有更好的建议意见可以在评论区中提出。
# 1. 概述
略。
# 2. 输出一个图像
## 2.1 PPM图像格式
每当启动渲染器时，您都需要一种查看图像的方法。最直接的方法就是将其写入文件。问题是，格式有很多。其中许多都很复杂。我总是从纯文本 ppm 文件开始。这是维基百科上的一个很好的描述：
![](https://s2.loli.net/2023/11/07/WbIAPpYZDrOjXny.png)
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
![](https://s2.loli.net/2023/11/07/67LucdGBOemJHzb.png)
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
所有光线追踪器都拥有的是光线类和沿着光线看到的颜色的计算。让我们将射线视为一个函数
<img src="https://www.zhihu.com/equation?tex= P(t) = A + t\vec{b} " alt=" P(t) = A + t\vec{b} " class="ee_img tr_noresize" eeimg="1">
。这里的
<img src="https://www.zhihu.com/equation?tex=P" alt="P" class="ee_img tr_noresize" eeimg="1">
是沿着三维空间中某条直线的三维位置。其中
<img src="https://www.zhihu.com/equation?tex=A" alt="A" class="ee_img tr_noresize" eeimg="1">
是射线原点，
<img src="https://www.zhihu.com/equation?tex=\vec{b}" alt="\vec{b}" class="ee_img tr_noresize" eeimg="1">
是射线方向，
<img src="https://www.zhihu.com/equation?tex=t" alt="t" class="ee_img tr_noresize" eeimg="1">
是类型为`double`的实数。输入不同的
<img src="https://www.zhihu.com/equation?tex=t" alt="t" class="ee_img tr_noresize" eeimg="1">
值，
<img src="https://www.zhihu.com/equation?tex=P(t)" alt="P(t)" class="ee_img tr_noresize" eeimg="1">
 就会沿着光线移动这个点。如果您输入的是负数
<img src="https://www.zhihu.com/equation?tex=t" alt="t" class="ee_img tr_noresize" eeimg="1">
，就可以在3D线上的任何位置移动。如果是正的
<img src="https://www.zhihu.com/equation?tex=t" alt="t" class="ee_img tr_noresize" eeimg="1">
，您只能得到A前面的部分，这通常被称为半线(*half-line*)或光线(*ray*)。
![](https://s2.loli.net/2023/11/09/r8ijsYPCAplXQBV.png)
我们可以将ray的概念表示为一个class，并且将
<img src="https://www.zhihu.com/equation?tex=P(t)" alt="P(t)" class="ee_img tr_noresize" eeimg="1">
用函数`ray::at(t)`来表示：
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

<img src="https://www.zhihu.com/equation?tex=
width/height = 16/9=1.7778" alt="
width/height = 16/9=1.7778" class="ee_img tr_noresize" eeimg="1">


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

为了简化问题，我们将摄像机中心设置在
<img src="https://www.zhihu.com/equation?tex=(0,0,0)" alt="(0,0,0)" class="ee_img tr_noresize" eeimg="1">
位置。我们还将设置y轴向上，x轴向右，负z轴指向观察方向。（这通常被称为右手坐标系(*right-handed coordinates*)。）

![](https://s2.loli.net/2023/11/09/lZwshv7eg93E6oW.png)

现在是不可避免的棘手部分。虽然我们的三维空间有上述约定，但这与我们的图像坐标相冲突，我们希望将第一个像素放在左上方，然后一直向下直到右下方的最后一个像素。这意味着我们的图像坐标Y轴是倒置的，即Y轴在图像上向下递增。

在扫描我们的图像时，我们将从左上角的像素
<img src="https://www.zhihu.com/equation?tex=(0,0)" alt="(0,0)" class="ee_img tr_noresize" eeimg="1">
开始，从左到右扫描每一行，然后逐行从上到下扫描。为了帮助浏览像素网格，我们将使用一个从左边缘到右边缘的向量
<img src="https://www.zhihu.com/equation?tex=V_u" alt="V_u" class="ee_img tr_noresize" eeimg="1">
，和一个从上边缘到下边缘的向量
<img src="https://www.zhihu.com/equation?tex=V_v" alt="V_v" class="ee_img tr_noresize" eeimg="1">
。

我们的像素网格将以像素间距的一半嵌入视口边缘。通过这种方式，我们的视口区域被均匀地划分为宽度
<img src="https://www.zhihu.com/equation?tex=\times" alt="\times" class="ee_img tr_noresize" eeimg="1">
高度的相同区域。下面是我们的视口和像素网格的样子：

![](https://s2.loli.net/2023/11/09/Jokfht3sVRIuWHc.png)

在这个图中，我们有视口————一个
<img src="https://www.zhihu.com/equation?tex=7\times5" alt="7\times5" class="ee_img tr_noresize" eeimg="1">
分辨率图像的像素网格，视口左上角的点
<img src="https://www.zhihu.com/equation?tex=Q" alt="Q" class="ee_img tr_noresize" eeimg="1">
，像素
<img src="https://www.zhihu.com/equation?tex=P_{(0,0)}" alt="P_{(0,0)}" class="ee_img tr_noresize" eeimg="1">
的位置，视口向量
<img src="https://www.zhihu.com/equation?tex=V_u" alt="V_u" class="ee_img tr_noresize" eeimg="1">
（`viewport_u`），视口向量
<img src="https://www.zhihu.com/equation?tex=V_v" alt="V_v" class="ee_img tr_noresize" eeimg="1">
（`viewport_v`），以及像素增量向量
<img src="https://www.zhihu.com/equation?tex=\delta u" alt="\delta u" class="ee_img tr_noresize" eeimg="1">
和
<img src="https://www.zhihu.com/equation?tex=\delta v" alt="\delta v" class="ee_img tr_noresize" eeimg="1">
。

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

现在我们将填充`ray_color(ray)`函数以实现一个简单的渐变。这个函数将根据y坐标的高度（在将光线方向缩放到单位长度后，
<img src="https://www.zhihu.com/equation?tex=−1.0<y<1.0" alt="−1.0<y<1.0" class="ee_img tr_noresize" eeimg="1">
）线性混合白色和蓝色。因为我们是在标准化向量后查看y的高度，你会注意到除了垂直渐变之外，颜色中还有水平渐变。

我将使用一个标准的图形学技巧来线性缩放
<img src="https://www.zhihu.com/equation?tex=0.0≤a≤1.0" alt="0.0≤a≤1.0" class="ee_img tr_noresize" eeimg="1">
。当
<img src="https://www.zhihu.com/equation?tex=a=1.0" alt="a=1.0" class="ee_img tr_noresize" eeimg="1">
时，我想要蓝色。当
<img src="https://www.zhihu.com/equation?tex=a=0.0" alt="a=0.0" class="ee_img tr_noresize" eeimg="1">
时，我想要白色。在两者之间，我想要一个混合色。这形成了一个“线性混合(*linear blend*)”或“线性插值(*linear interpolation*)”。这通常被称为两个值之间的*lerp*。*lerp*总是以下面的形式表示：


<img src="https://www.zhihu.com/equation?tex=
blendedValue = (1-a)\cdot startValue + a\cdot endValue" alt="
blendedValue = (1-a)\cdot startValue + a\cdot endValue" class="ee_img tr_noresize" eeimg="1">


其中
<img src="https://www.zhihu.com/equation?tex=a" alt="a" class="ee_img tr_noresize" eeimg="1">
从0变成1。

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
![](https://s2.loli.net/2023/11/09/aRtw1HyUpiOYrqd.png)

# 5. 添加球体