#include <stdio.h>
#include <stdlib.h>

// Include glew before gl and glfw
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

#include "RenderObj.h"

#define DEG_TO_RAD 0.01745329252f

using namespace glm;

int main(int argc, char const *argv[])
{
    // Termos Code
	// Open the serial port. Change device path as needed (currently set to an standard FTDI USB-UART cable type device)
    int serial_port = open("/dev/ttyACM0", O_RDWR);

        printf("arg1 = %s\n", argv[1]);
    // Create new termios struct, we call it 'tty' for convention
    struct termios tty;

    // Read in existing settings, and handle any error
    if(tcgetattr(serial_port, &tty) != 0) {
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
        return 1;
    }

    tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
    tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size 
    tty.c_cflag |= CS8; // 8 bits per byte (most common)
    tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ECHOE; // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
    // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
    // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

    tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    // Set in/out baud rate to be 9600
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    // Save tty settings, also checking for error
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
        return 1;
    }

    // USB termos reading buffer
    char read_buf [256];
  


    // OpenGL Code
    glewExperimental = true; // Needed for core profile
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL 

    // Open a window and create its OpenGL context
    GLFWwindow* window; // (In the accompanying source code, this variable is global for simplicity)
    
    window = glfwCreateWindow(640, 480, "Render IMU3D", NULL, NULL);
    
    if(!window){
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        
        glfwTerminate();
        return -1;
    }
    
    // Make the window's context current
    glfwMakeContextCurrent(window); 
    
    // Initialize GLEW 
    glewExperimental=true; // Needed in core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }
    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Black background
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS); 

    // The VAO: Vertex Array Object
    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders( "../TransformVertexShader.vertexshader", "../ColorFragmentShader.fragmentshader" );

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");

	// Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	glm::mat4 Projection = glm::perspective(glm::radians(60.0f), 4.0f / 3.0f, 0.1f, 100.0f);
	// Or, for an ortho camera :
	//glm::mat4 Projection = glm::ortho(-10.0f,10.0f,-10.0f,10.0f,0.0f,100.0f); // In world coordinates
	
    // Camera matrix
	glm::mat4 View       = glm::lookAt(
								glm::vec3(0,0,-5), // Camera is at (4,3,3), in World Space
								glm::vec3(0,0,0), // and looks at the origin
								glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
						   );
	// Model matrix : an identity matrix (model will be at the origin)
	// glm::mat4 Model = glm::mat4(1.0f);

	// Our ModelViewProjection : multiplication of our 3 matrices
	// glm::mat4 MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around

	// Our vertices. Tree consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
	// A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
	static const GLfloat g_vertex_buffer_data[] = { 
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		 1.0f, 1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		 1.0f,-1.0f,-1.0f,
		 1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		 1.0f,-1.0f, 1.0f,
		 1.0f, 1.0f, 1.0f,
		 1.0f,-1.0f,-1.0f,
		 1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f,-1.0f,
		 1.0f, 1.0f, 1.0f,
		 1.0f,-1.0f, 1.0f,
		 1.0f, 1.0f, 1.0f,
		 1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		 1.0f,-1.0f, 1.0f
	};

    // One color for each vertex. They were generated randomly.
	static const GLfloat g_color_buffer_data[] = { 
		0.439f,  0.502f,  0.565f,
        0.663f, 0.663f, 0.663f,
		0.184f, 0.310f, 0.310f,
        0.663f, 0.663f, 0.663f,
		0.439f,  0.502f,  0.565f,
		0.184f, 0.310f, 0.310f,
        0.439f,  0.502f,  0.565f,
		0.663f, 0.663f, 0.663f,
        0.184f, 0.310f, 0.310f,
        0.663f, 0.663f, 0.663f,
        0.439f,  0.502f,  0.565f,
		0.184f, 0.310f, 0.310f,
        0.439f,  0.502f,  0.565f,
		0.663f, 0.663f, 0.663f,
        0.184f, 0.310f, 0.310f,
        0.663f, 0.663f, 0.663f,
        0.439f,  0.502f,  0.565f,
		0.184f, 0.310f, 0.310f,
        0.439f,  0.502f,  0.565f,
		0.663f, 0.663f, 0.663f,
        0.184f, 0.310f, 0.310f,
        0.663f, 0.663f, 0.663f,
        0.439f,  0.502f,  0.565f,
		0.184f, 0.310f, 0.310f,
        0.439f,  0.502f,  0.565f,
		0.663f, 0.663f, 0.663f,
        0.184f, 0.310f, 0.310f,
        0.663f, 0.663f, 0.663f,
        0.439f,  0.502f,  0.565f,
		0.184f, 0.310f, 0.310f,
        0.439f,  0.502f,  0.565f,
		0.663f, 0.663f, 0.663f,
        0.184f, 0.310f, 0.310f,
        0.663f, 0.663f, 0.663f,
        0.439f,  0.502f,  0.565f,
		0.184f, 0.310f, 0.310f
	};

    // This will identify our vertex buffer
    GLuint vertexbuffer;
    // Generate 1 buffer, put the resulting identifier in vertexbuffer
    glGenBuffers(1, &vertexbuffer);
    // The following commands will talk about our 'vertexbuffer' buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    // Give our vertices to OpenGL.
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);


    GLuint colorbuffer;
	glGenBuffers(1, &colorbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

    do{

        // Serial code
        memset(&read_buf, '\0', sizeof(read_buf));  
        int num_bytes = read(serial_port, &read_buf, sizeof(read_buf));
        
        if (num_bytes < 0) {
            printf("Error reading: %s", strerror(errno));
            return 1;
        }

        char *token;
    
        token = strtok(read_buf, ",");
        
        int count = 0;

        double pitch, roll;
        
        while( token != NULL ) {
            // printf( " %s", token );
            count == 0 ? pitch = atof(token) : roll = atof(token);
            
            token = strtok(NULL, ",");
            count++;
        }

        printf("pitch = %.2f roll = %.2f\n", pitch, roll);

        // float roll = 45.0f * DEG_TO_RAD;
        // float pitch = 45.0f * DEG_TO_RAD;
        float yaw = 15.0f * DEG_TO_RAD;


        // OpenGL
        // Model matrix : an identity matrix (model will be at the origin)
	    // glm::mat4 Model = glm::mat4(1.0f);
        
        glm::mat4 Model = glm::rotate(glm::mat4(1.0f), (float)pitch, glm::vec3(1, 0, 0));
        Model = glm::rotate(Model, yaw, glm::vec3(0, 1, 0));
        Model = glm::rotate(Model, (float)roll, glm::vec3(0, 0, 1));

        // Our ModelViewProjection : multiplication of our 3 matrices
    	glm::mat4 MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around

         /* Render here */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use our shader
        glUseProgram(programID);

        // Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

        // 1st attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(
            0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
            3,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)0            // array buffer offset
        );

        // 2nd attribute buffer : colors
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glVertexAttribPointer(
			1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
        
        // Draw the triangle !
        glDrawArrays(GL_TRIANGLES, 0, 12*3); // Starting from vertex 0; 3 vertices total -> 1 triangle // 12*3 indices starting at 0 -> 12 triangles
        glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();

    }while( // Check if the ESC key was pressed or the window was closed
        glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
        glfwWindowShouldClose(window) == 0
        );
    

     // Cleanup VBO
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &colorbuffer);
    glDeleteVertexArrays(1, &VertexArrayID);
    glDeleteProgram(programID);

    // Close OpenGL window and terminate GLFW


    glfwTerminate();
    return 0;
}

