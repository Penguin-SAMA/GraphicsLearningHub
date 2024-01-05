作业2主要就是做包围盒，对两个三角形进行光栅化，并能正确显示两个三角形的前后关系。提高任务为实现 MSAA 算法进行反走样。

# 基本要求

首先把作业一中的 `get_projection_matrix` 函数的代码粘贴到 `main.cpp` 中。

然后我们进入 `rasterizer.cpp`。

## 包围盒

首先确定三角形的包围盒边界，代码如下：

```cpp
float min_x = std::min(v[0].x(), std::min(v[1].x(), v[2].x()));
float max_x = std::max(v[0].x(), std::max(v[1].x(), v[2].x()));
float min_y = std::min(v[0].y(), std::min(v[1].y(), v[2].y()));
float max_y = std::max(v[0].y(), std::max(v[1].y(), v[2].y()));

min_x = static_cast<int>(std::floor(min_x));
max_x = static_cast<int>(std::floor(max_x));
min_y = static_cast<int>(std::ceil(min_y));
max_y = static_cast<int>(std::ceil(max_y));
```

## 光栅化

```cpp
for (int x = min_x; x <= max_x; ++x) {
            for (int y = min_y; y <= max_y; ++y) {
                // 判断采样点是否在三角形内
                if (insideTriangle(static_cast<float>(x + 0.5), static_cast<float>(y + 0.5), t.v)) {
                    auto [alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
                    float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                    float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                    z_interpolated *= w_reciprocal;

                    if (depth_buf[get_index(x, y)] > z_interpolated) {
                        depth_buf[get_index(x, y)] = z_interpolated;

                        Eigen::Vector3f color = t.getColor();
                        Eigen::Vector3f point;
                        point << static_cast<float>(x), static_cast<float>(y), z_interpolated;
                        set_pixel(point, color);
                    }
                }
            }
        }
```

其中对采样点的 z 轴插值部分在框架中已经给了，直接取消注释就行。

## 判断采样点是否在三角形内

```cpp
static bool insideTriangle(double x, double y, const Vector3f* _v) {
    Eigen::Vector2f p;  // 检测目标
    p << x, y;

    //.head(2)指这个点的前两个数值，即x,y
    Eigen::Vector2f a, b, c;            // 被检测的三角形三边向量
    a = _v[0].head(2) - _v[1].head(2);  // a = A - B  即B->A
    b = _v[1].head(2) - _v[2].head(2);  // b = B - C  即C->B
    c = _v[2].head(2) - _v[0].head(2);  // c = C - A  即A->C

    Eigen::Vector2f AP, BP, CP;
    AP = p - _v[0].head(2);
    BP = p - _v[1].head(2);
    CP = p - _v[2].head(2);

    // 由于我这里的向量方向都是逆时针的，所以如果点p在内，那么所有边向量叉乘对应的XP都应该为正值，指向z的正半轴。
    return AP[0] * c[1] - AP[1] * c[0] > 0 && BP[0] * a[1] - BP[1] * a[0] > 0 && CP[0] * b[1] - CP[1] * b[0] > 0;
}


```

这样就可以得到下面的结果：

![image-20240105145509919](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/image-20240105145509919.png)

可以看到锯齿还是比较明显的：

![](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/image-20240105145721287.png)

# 提高部分

这里不太懂，先把代码贴出来，后面再看看。

```cpp
std::vector<Eigen::Vector2f> pos{{0.25, 0.25}, {0.75, 0.25}, {0.25, 0.75}, {0.75, 0.75}};
        for (int x = min_x; x <= max_x; ++x) {
            for (int y = min_y; y <= max_x; ++y) {
                int count = 0;
                float minDepth = FLT_MAX;
                for (int MSAA_4 = 0; MSAA_4 < 4; ++MSAA_4) {
                    if (insideTriangle(static_cast<float>(x + pos[MSAA_4][0]), static_cast<float>(y + pos[MSAA_4][1]), t.v)) {
                        auto [alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
                        float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                        float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                        z_interpolated *= w_reciprocal;

                        minDepth = std::min(minDepth, z_interpolated);
                        ++count;
                    }
                }
                if (count) {
                    if (depth_buf[get_index(x, y)] > minDepth) {
                        depth_buf[get_index(x, y)] = minDepth;

                        Eigen::Vector3f color = t.getColor() * (count / 4.0);
                        Eigen::Vector3f point;
                        point << static_cast<float>(x), static_cast<float>(y), minDepth;
                        set_pixel(point, color);
                    }
                }
            }
        }
```

# 完整代码

**insideTriangle**

```cpp
static bool insideTriangle(double x, double y, const Vector3f* _v) {
    Eigen::Vector2f p;  // 检测目标
    p << x, y;

    //.head(2)指这个点的前两个数值，即x,y
    Eigen::Vector2f a, b, c;            // 被检测的三角形三边向量
    a = _v[0].head(2) - _v[1].head(2);  // a = A - B  即B->A
    b = _v[1].head(2) - _v[2].head(2);  // b = B - C  即C->B
    c = _v[2].head(2) - _v[0].head(2);  // c = C - A  即A->C

    Eigen::Vector2f AP, BP, CP;
    AP = p - _v[0].head(2);
    BP = p - _v[1].head(2);
    CP = p - _v[2].head(2);

    // 由于我这里的向量方向都是逆时针的，所以如果点p在内，那么所有边向量叉乘对应的XP都应该为正值，指向z的正半轴。
    return AP[0] * c[1] - AP[1] * c[0] > 0 && BP[0] * a[1] - BP[1] * a[0] > 0 && CP[0] * b[1] - CP[1] * b[0] > 0;
}
```

**rasterize_triangle**

```cpp
void rst::rasterizer::rasterize_triangle(const Triangle& t) {
    auto v = t.toVector4();

    // TODO : Find out the bounding box of current triangle.
    // iterate through the pixel and find if the current pixel is inside the triangle

    // 1. 构建包围盒
    float min_x = std::min(v[0].x(), std::min(v[1].x(), v[2].x()));
    float max_x = std::max(v[0].x(), std::max(v[1].x(), v[2].x()));
    float min_y = std::min(v[0].y(), std::min(v[1].y(), v[2].y()));
    float max_y = std::max(v[0].y(), std::max(v[1].y(), v[2].y()));

    min_x = static_cast<int>(std::floor(min_x));
    max_x = static_cast<int>(std::floor(max_x));
    min_y = static_cast<int>(std::ceil(min_y));
    max_y = static_cast<int>(std::ceil(max_y));

    bool MSAA = false;  // 用于开关MSAA

    if (MSAA) {
        std::vector<Eigen::Vector2f> pos{{0.25, 0.25}, {0.75, 0.25}, {0.25, 0.75}, {0.75, 0.75}};
        for (int x = min_x; x <= max_x; ++x) {
            for (int y = min_y; y <= max_x; ++y) {
                int count = 0;
                float minDepth = FLT_MAX;
                for (int MSAA_4 = 0; MSAA_4 < 4; ++MSAA_4) {
                    if (insideTriangle(static_cast<float>(x + pos[MSAA_4][0]), static_cast<float>(y + pos[MSAA_4][1]), t.v)) {
                        auto [alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
                        float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                        float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                        z_interpolated *= w_reciprocal;

                        minDepth = std::min(minDepth, z_interpolated);
                        ++count;
                    }
                }
                if (count) {
                    if (depth_buf[get_index(x, y)] > minDepth) {
                        depth_buf[get_index(x, y)] = minDepth;

                        Eigen::Vector3f color = t.getColor() * (count / 4.0);
                        Eigen::Vector3f point;
                        point << static_cast<float>(x), static_cast<float>(y), minDepth;
                        set_pixel(point, color);
                    }
                }
            }
        }
    } else {
        for (int x = min_x; x <= max_x; ++x) {
            for (int y = min_y; y <= max_y; ++y) {
                if (insideTriangle(static_cast<float>(x + 0.5), static_cast<float>(y + 0.5), t.v)) {
                    auto [alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
                    float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                    float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                    z_interpolated *= w_reciprocal;

                    if (depth_buf[get_index(x, y)] > z_interpolated) {
                        depth_buf[get_index(x, y)] = z_interpolated;

                        Eigen::Vector3f color = t.getColor();
                        Eigen::Vector3f point;
                        point << static_cast<float>(x), static_cast<float>(y), z_interpolated;
                        set_pixel(point, color);
                    }
                }
            }
        }
    }
}
```

