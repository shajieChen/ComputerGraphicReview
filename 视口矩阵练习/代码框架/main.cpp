#include "Triangle.hpp"
#include "rasterizer.hpp"
#include <eigen3/Eigen/Eigen>
#include <iostream>
#include <opencv2/opencv.hpp>

constexpr double MY_PI = 3.1415926;
float toRadians(float degree) ;  
Eigen::Matrix4f get_rotation(Vector3f axis, float angle); 

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, -eye_pos[0], 0, 1, 0, -eye_pos[1], 0, 0, 1,
        -eye_pos[2], 0, 0, 0, 1;

    view = translate * view;

    return view;
}

Eigen::Matrix4f get_model_matrix(float rotation_angle)
{
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity(); 
    // TODO: Implement this function
    // Create the model matrix for rotating the triangle around the Z axis.
    // Then return it.
    Eigen::Matrix4f rotation;  
    rotation << 
        cos(rotation_angle) , sin(rotation_angle) , 0 , 0, 
        -sin(rotation_angle), cos(rotation_angle) , 0, 0, 
        0, 0, 1, 0 ,
        0, 0, 0, 1 ; 
    model = rotation * model ; 

    return model;
}
float toRadians(float degree)
{
      return degree*22/(180*7);
} 

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio,
                                      float zNear, float zFar)
{
    // Students will implement this function 
    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();

    // TODO: Implement this function
    // Create the projection matrix for the given parameters.
    // Then return it.
    /* 0
    */  
    float top, bottom, left, right;

    top = zNear * tan(toRadians(eye_fov)/2); 
    bottom = -top;
    right = top * aspect_ratio;
    left = -right;
    projection << 
        2 * zNear / (right - left) ,  0  , 0 , - zNear * (right + left ) / (right - left)  , 
        0 , 2 * zNear / (top - bottom) , 0 ,   - zNear * (top + bottom) / (top - bottom) , 
        0 , 0 , -(zFar + zNear) / (zFar - zNear) , 2 * zFar * zNear / (zNear - zFar) , 
        0 , 0 , -1 , 0 ;  

    return projection;
}


Eigen::Matrix4f get_rotation(Vector3f axis, float angle)
{
    Eigen::Matrix4f rotationMatrix ; 
    if(axis == Vector3f(1.0, 0.0, 0.0))
    {
        //x 
        rotationMatrix << 
            1.0,         0.0,           0.0,        0.0, 
            0.0, cos(angle) , - sin(angle) ,        0.0,
            0.0, sin(angle) ,   cos(angle) ,        0.0, 
            0.0,         0.0,           0.0,        1.0 ; 

    }
    else if(axis == Vector3f(0.0, 1.0, 0.0))
    {
        //y
        rotationMatrix << 
        cos(angle),          0.0,    sin(angle),        0.0, 
              0.0 ,          1.0,           0.0,        0.0, 
       - sin(angle) ,       0.0,    cos(angle) ,        0.0, 
               0.0,          0.0,            0.0,        1.0; 
                
    } 
    else if(axis == Vector3f(0.0, 0.0, 1.0))
    {
        //z 
        rotationMatrix << 
        cos(angle) , -sin(angle) , 0.0, 0.0, 
        sin(angle) , cos(angle) , 0.0, 0.0, 
        0.0, 0.0 , 1.0, 0.0, 
        0.0 , 0.0, 0.0, 1.0 ; 
    }
    return rotationMatrix; 
}

int main(int argc, const char** argv)
{
    float angle = 0;
    Eigen::Vector3f rotateAxis;
    rotateAxis << 1.0 ,0.0 , 0.0;  //init   
    bool command_line = false;
    std::string filename = "output.png";

    if (argc >= 3) {
        command_line = true;
        angle = std::stof(argv[2]); // -r by default
        if (argc == 4) {
            filename = std::string(argv[3]);
        }
        else
            return 0;
    }

    rst::rasterizer r(700, 700);

    Eigen::Vector3f eye_pos = {0, 0, 5};

    std::vector<Eigen::Vector3f> pos{{2, 0, -2}, {0, 2, -2}, {-2, 0, -2}};

    std::vector<Eigen::Vector3i> ind{{0, 1, 2}};

    auto pos_id = r.load_positions(pos);
    auto ind_id = r.load_indices(ind);

    int key = 0;
    int frame_count = 0;

    if (command_line) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);

        cv::imwrite(filename, image);

        return 0;
    }

    while (key != 27) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_rotation(rotateAxis, angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);

        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::imshow("image", image);
        key = cv::waitKey(10);

        // std::cout << "frame count: " << frame_count++ << '\n';

        if (key == 'a') {
            angle += 10;
        }
        else if (key == 'd') {
            angle -= 10;
        }
        else if (key == 'x'){
            rotateAxis << 1.0,  0.0, 0.0;  
            std::cout << "Change To Rotate X" << std::endl ; 
        }
        else if (key == 'y'){
            rotateAxis << 0.0 , 1.0, 0.0 ; 
            std::cout << "Change To Rotate Y" << std::endl ; 
        }
        else if (key == 'z'){
            rotateAxis << 0.0, 0.0, 1.0 ; 
            std::cout << "Change To Rotate Z" << std::endl;  
        } 

    }

    return 0;
}
