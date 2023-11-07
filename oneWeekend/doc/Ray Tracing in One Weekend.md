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
所有光线追踪器都拥有的是光线类和沿着光线看到的颜色的计算。让我们将射线视为一个函数$P(t) = A + tb$