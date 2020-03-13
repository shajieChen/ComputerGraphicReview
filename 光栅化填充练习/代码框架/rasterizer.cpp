// clang-format off
//
// Created by goksu on 4/6/19.
//

#include <algorithm>
#include <vector>
#include "rasterizer.hpp"
#include <opencv2/opencv.hpp>
#include <math.h>


rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f> &positions)
{
    auto id = get_next_id();
    pos_buf.emplace(id, positions);

    return {id};
}

rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i> &indices)
{
    auto id = get_next_id();
    ind_buf.emplace(id, indices);

    return {id};
}

rst::col_buf_id rst::rasterizer::load_colors(const std::vector<Eigen::Vector3f> &cols)
{
    auto id = get_next_id();
    col_buf.emplace(id, cols);

    return {id};
}

auto to_vec4(const Eigen::Vector3f& v3, float w = 1.0f)
{
    return Vector4f(v3.x(), v3.y(), v3.z(), w);
}

/* A utility function to calculate area of triangle formed by (x1, y1),  
   (x2, y2) and (x3, y3) */ 
float area(int x1, int y1, int x2, int y2, int x3, int y3) 
{ 
   return abs((x1*(y2-y3) + x2*(y3-y1)+ x3*(y1-y2))/2.0); 
} 

static bool insideTriangle(float x, float y, const Vector3f* _v)
{   
    int x1 = _v[0].x(); 
    int y1 = _v[0].y(); 
    int x2 = _v[1].x(); 
    int y2 = _v[1].y();
    int x3 = _v[2].x();
    int y3 = _v[2].y();
    /* Calculate area of triangle ABC */
   float A = area (x1, y1, x2, y2, x3, y3); 
  
//    /* Calculate area of triangle PBC */   
   float A1 = area (x, y, x2, y2, x3, y3); 
  
//    /* Calculate area of triangle PAC */   
   float A2 = area (x1, y1, x, y, x3, y3); 
  
//    /* Calculate area of triangle PAB */    
   float A3 = area (x1, y1, x2, y2, x, y); 
    
   /* Check if sum of A1, A2 and A3 is same as A */ 
   return (A == A1 + A2 + A3); 
// return false ; 
}

static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector3f* v)
{
    float c1 = (x*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*y + v[1].x()*v[2].y() - v[2].x()*v[1].y()) / (v[0].x()*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*v[0].y() + v[1].x()*v[2].y() - v[2].x()*v[1].y());
    float c2 = (x*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*y + v[2].x()*v[0].y() - v[0].x()*v[2].y()) / (v[1].x()*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*v[1].y() + v[2].x()*v[0].y() - v[0].x()*v[2].y());
    float c3 = (x*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*y + v[0].x()*v[1].y() - v[1].x()*v[0].y()) / (v[2].x()*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*v[2].y() + v[0].x()*v[1].y() - v[1].x()*v[0].y());
    return {c1,c2,c3};
}

void rst::rasterizer::draw(pos_buf_id pos_buffer, ind_buf_id ind_buffer, col_buf_id col_buffer, Primitive type)
{
    auto& buf = pos_buf[pos_buffer.pos_id];
    auto& ind = ind_buf[ind_buffer.ind_id];
    auto& col = col_buf[col_buffer.col_id];

    float f1 = (50 - 0.1) / 2.0;
    float f2 = (50 + 0.1) / 2.0;

    Eigen::Matrix4f mvp = projection * view * model;
    for (auto& i : ind)
    {
        Triangle t;
        Eigen::Vector4f v[] = {
                mvp * to_vec4(buf[i[0]], 1.0f),
                mvp * to_vec4(buf[i[1]], 1.0f),
                mvp * to_vec4(buf[i[2]], 1.0f)
        };
        //Homogeneous division
        for (auto& vec : v) {
            vec /= vec.w();
        }
        //Viewport transformation
        for (auto & vert : v)
        {
            vert.x() = 0.5*width*(vert.x()+1.0);
            vert.y() = 0.5*height*(vert.y()+1.0);
            vert.z() = vert.z() * f1 + f2;
        }

        for (int i = 0; i < 3; ++i)
        {
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
        }

        auto col_x = col[i[0]];
        auto col_y = col[i[1]];
        auto col_z = col[i[2]];

        t.setColor(0, col_x[0], col_x[1], col_x[2]);
        t.setColor(1, col_y[0], col_y[1], col_y[2]);
        t.setColor(2, col_z[0], col_z[1], col_z[2]);

        rasterize_triangle(t);
    }
}

//Screen space rasterization
void rst::rasterizer::rasterize_triangle(const Triangle& t) {
    auto v = t.toVector4();
    
    //Get the max Width & Height of the Triangle
    int maxWidth = 0 ; 
    int minWidth = width ;  
    for(int i = 0 ; i <  (int)(sizeof(t.v)/sizeof(*t.v)) ; i ++ )
    {
        // std::cout << "v[i].x(): " << v[i].x() << std::endl;   
        if(v[i].x() > maxWidth)
        {
            maxWidth = v[i].x();  
        }
        if(v[i].x() < minWidth)
        {
            minWidth = v[i].x() ; 
        }
    } 
    // iterate through the pixel and find if the current pixel is inside the triangle
    for(int i = minWidth; i < maxWidth ; i ++ )
    {
        for(int j = 0; j < height ; j++ )
        {
#pragma region MSAA
            //TODO : SuperSampling  
            // if(insideTriangle(i , j , t.v))
            // {
            //     int percentage = 1 ; 
            //     for(int k = 0 ; k < 2 ; k++ )
            //     {
            //         for(int l = 0 ; l < 2 ; l++)
            //         {
            //             float x = i + 0.5 + 0.25 * pow(-1 , (k+1)) ; //tmp x 
            //             float y = j + 0.5 + 0.25 * pow(-1 , (l+1)); 
            //             if(insideTriangle(x, y , t.v))
            //             {
            //                 //Cur Pixel in the Triangle
            //                 auto[alpha, beta, gamma] = computeBarycentric2D(x  , y , t.v);
            //                 float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
            //                 float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
            //                 z_interpolated *= w_reciprocal; 
                            
            //                 if(z_interpolated < depth_buf[get_index(x , y )])
            //                 {
            //                     depth_buf[get_index(x , y )] = z_interpolated; 
            //                     set_pixel(Vector3f(x  , y , z_interpolated) , t.getColor());
            //                 }
            //             } 
            //         }
            //     }   
            // }
#pragma endregion
            if(insideTriangle(i , j , t.v))
            {
                auto[alpha, beta, gamma] = computeBarycentric2D(i  , j , t.v);
                float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                z_interpolated *= w_reciprocal; 

                if(z_interpolated < depth_buf[get_index(i , j )])
                {
                    depth_buf[get_index(i , j )] = z_interpolated; 
                    set_pixel(Vector3f(i  , j , z_interpolated) , t.getColor());
                }
            }
        } 
    }  
}

void rst::rasterizer::set_model(const Eigen::Matrix4f& m)
{
    model = m;
}

void rst::rasterizer::set_view(const Eigen::Matrix4f& v)
{
    view = v;
}

void rst::rasterizer::set_projection(const Eigen::Matrix4f& p)
{
    projection = p;
}

void rst::rasterizer::clear(rst::Buffers buff)
{
    if ((buff & rst::Buffers::Color) == rst::Buffers::Color)
    {
        std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f{0, 0, 0});
    }
    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
    {
        std::fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
    } 
}

rst::rasterizer::rasterizer(int w, int h) : width(w), height(h)
{
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);
    MSAA_Depth_buf.resize(4 * w * h); 
}

int rst::rasterizer::get_index(int x, int y)
{
    return (height-1-y)*width + x;
}
float rst::rasterizer::get_index(float x, float y)
{
    return (height-1-y)*width + x;
}
void rst::rasterizer::set_pixel(const Eigen::Vector3f& point, const Eigen::Vector3f& color)
{
    //old index: auto ind = point.y() + point.x() * width;
    auto ind = (height-1-point.y())*width + point.x();
    frame_buf[ind] = color;

}

// clang-format on