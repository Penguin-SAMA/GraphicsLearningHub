# Games101 作业三 分析

这次的作业非常难，要做的代码填充很多，也很复杂，我会在后续的代码框架中用注释给出我对代码的理解。

本次作业任务列表如下：

1. 修改函数 `rasterize_triangle(const Triangle& t)` in rasterizer.cpp: 在此处实现与作业 2 类似的插值算法,实现法向量、颜色、纹理颜色的插值。
2. 修改函数 `get_projection_matrix()` in main.cpp: 将你自己在之前的实验中实现的投影矩阵填到此处,此时你可以运行 `./Rasterizer output.png normal`来观察法向量实现结果。
3. 修改函数 `phong_fragment_shader()` in main.cpp: 实现 Blinn-Phong 模型计算 Fragment Color.
4. 修改函数 `texture_fragment_shader()` in main.cpp: 在实现 Blinn-Phong 的基础上,将纹理颜色视为公式中的 kd,实现 Texture Shading FragmentShader.
5. 修改函数 `bump_fragment_shader()` in main.cpp: 在实现 Blinn-Phong 的基础上,仔细阅读该函数中的注释,实现 Bump mapping.
6. 修改函数 `displacement_fragment_shader()` in main.cpp: 在实现 Bump mapping 的基础上,实现  displacement mapping.

首先我们实现法向量、颜色和纹理的插值，这里比较类似作业2的算法，不过最好重新写一遍熟悉一下。

```cpp
void rst::rasterizer::rasterize_triangle(const Triangle &t, const std::array<Eigen::Vector3f, 3> &view_pos)
{
    // TODO: 从你的HW3中获取三角形光栅化代码。
    // TODO: 在你的光栅化循环中：
    //    * v[i].w() 是顶点视图空间深度值 z。
    //    * Z 是当前像素的插值视图空间深度
    //    * zp 是介于 zNear 和 zFar 之间的深度，用于 z-buffer    // 构建三角形包围盒

    auto v = t.toVector4();

    int min_x = std::min({v[0].x(), v[1].x(), v[2].x()});
    int min_y = std::min({v[0].y(), v[1].y(), v[2].y()});
    int max_x = std::max({v[0].x(), v[1].x(), v[2].x()});
    int max_y = std::max({v[0].y(), v[1].y(), v[2].y()});

    min_x = std::floor(min_x);
    min_y = std::floor(min_y);
    max_x = std::ceil(max_x);
    max_y = std::ceil(max_y);

    // 遍历包围盒的边界
    for (int x = min_x; x <= max_x; ++x) {
        for (int y = min_y; y <= max_y; ++y) {
            // 判断当前像素是否在三角形内部
            if (insideTriangle((float)x + 0.5, (float)y + 0.5, t.v)) {
                // 使用结构化绑定定义重心坐标
                auto [alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);

                // 透视矫正插值，用于计算深度
                float Z  = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                float zp = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                zp *= Z;

                // 如果计算出的深度值小于深度缓冲区的值，则更新像素
                if (zp < depth_buf[get_index(x, y)]) {
                    // 插值其他属性
                    auto interpolated_color = interpolate(alpha, beta, gamma, t.color[0], t.color[1], t.color[2], 1);
                    auto interpolated_normal = interpolate(alpha, beta, gamma, t.normal[0], t.normal[1], t.normal[2], 1).normalized();
                    auto interpolated_texcoords = interpolate(alpha, beta, gamma, t.tex_coords[0], t.tex_coords[1], t.tex_coords[2], 1);
                    // 插值view space中的坐标，用于恢复camera space中的坐标
                    // 参考解答：http://games-cn.org/forums/topic/zuoye3-interpolated_shadingcoords/
                    auto interpolated_shadingcoords = interpolate(alpha, beta, gamma, view_pos[0], view_pos[1], view_pos[2], 1);

                    // 构建片段着色器的负载
                    fragment_shader_payload payload(interpolated_color, interpolated_normal.normalized(), interpolated_texcoords, texture ? &*texture : nullptr);
                    payload.view_pos = interpolated_shadingcoords;
                    // 不是直接将三角形的颜色传递给帧缓冲区，而是首先传递给着色器以获取最终颜色
                    auto pixel_color = fragment_shader(payload);

                    // 更新深度缓冲区并设置像素颜色
                    depth_buf[get_index(x, y)] = zp;
                    set_pixel(Eigen::Vector2i(x, y), pixel_color);
                }
            }
        }
    }
}
```

这里讲解一下代码中比较关键的部分：

- `computeBarycentric2D(x, y, t.v)`：

  计算重心坐标，这个函数的定义如下：
  
  ```cpp
  static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector4f* v){
      float c1 = (x*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*y + v[1].x()*v[2].y() - v[2].x()*v[1].y()) / (v[0].x()*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*v[0].y() + v[1].x()*v[2].y() - v[2].x()*v[1].y());
      float c2 = (x*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*y + v[2].x()*v[0].y() - v[0].x()*v[2].y()) / (v[1].x()*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*v[1].y() + v[2].x()*v[0].y() - v[0].x()*v[2].y());
      float c3 = (x*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*y + v[0].x()*v[1].y() - v[1].x()*v[0].y()) / (v[2].x()*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*v[2].y() + v[0].x()*v[1].y() - v[1].x()*v[0].y());
      return {c1,c2,c3};
  }
  ```
  
  这里对应的是课上讲的重心坐标计算方法：
  
  ![重心坐标求解](https://raw.githubusercontent.com/Penguin-SAMA/PicGo/main/image-20240115191318350.png)
  
  推荐一个关于重心坐标讲解的很好的博客：https://blog.csdn.net/qq_38065509/article/details/105446756
  
- `float Z  = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());`:计算透视因子

  这里的`v[i].w()`指的是顶点在透视投影中的深度。

  `alpha, beta, gamma`是重心坐标，表示当前像素点在三角形内的相对位置。

  这个公式用于确保在透视投影下，深度和其他属性的插值是线性的。

- `float zp = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();`：计算插值的深度值

  这里的zp是当前像素的深度值，它是基于三角形顶点的深度值和对应的w分量进行加权插值得到的。

  如果只用重心坐标来插值会导致深度失真，因此需要通过顶点的 w 分量来进行校正。

