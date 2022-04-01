#include <stdio.h>
#include <string.h>
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <stdlib.h>
#include <bits/stdc++.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <cmath>
#include <SDL2/SDL.h>

class Point {
public:
    double x, y, z;

    Point() {
        this->x = 0;
        this->y = 0;
        this->z = 0;
    }

    Point(double x, double y, double z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    Point(double x, double y) {
        this->x = x;
        this->y = y;
        this->z = 0;
    }

    double operator[](int i) const {
        if (i == 0) return this->x;
        if (i == 1) return this->y;
        return this->z;
    }

    double& operator[](int i) {
        if (i == 0) return this->x;
        if (i == 1) return this->y;
        return this->z;
    }
};

typedef std::vector<double> Vector;
typedef std::vector<Vector> Matrix;

Matrix dot(const Matrix& a, const Matrix& b) {
    Matrix result = Matrix(a.size(), Vector(b[0].size(), 0));
    for (int i=0; i<a.size(); i++) {
        for (int j=0; j<b[0].size(); j++) {
            for (int k=0; k<b.size(); k++) {
                result[i][j] += a[i][k] * b[k][j];
            }
        }
    }
    return result;
}

Point transform(const Matrix& matrix, const Point& point) {
    Matrix p = {{point.x}, {point.y}, {point.z}};
    Matrix r = dot(matrix, p);
    return Point(r[0][0], r[1][0], r[2][0]);
}

Point translate(const Point& shift, const Point& point) {
    return Point(
        point.x + shift.x,
        point.y + shift.y,
        point.z + shift.z
    );
}

void connect(SDL_Renderer* const renderer, const std::vector<Point> &points, int i, int j) {
    SDL_RenderDrawLine(
        renderer,
        points[i].x,
        points[i].y,
        points[j].x,
        points[j].y
    );
}


void tokenize(std::string s, std::string del = " ")
{
    int start = 0;
    int end = s.find(del);
    while (end != -1) {
        std::cout << s.substr(start, end - start) << " ";
        start = end + del.size();
        end = s.find(del, start);
    }
    std::cout << s.substr(start, end - start);

}

Matrix getRotationMatrix(double alpha, double gamma) {
    // double alpha = 0; //0.001; // pitch
    Matrix rotationX = {
        {1, 0, 0},
        {0, cos(alpha), -sin(alpha)},
        {0, sin(alpha), cos(alpha)}
    };

    double beta = 0; //0.002;
    Matrix rotationY = {
        {cos(beta), 0, sin(beta)},
        {0, 1, 0},
        {-sin(beta), 0, cos(beta)}
    };

    // double gamma = 0.002; // roll
    Matrix rotationZ = {
        {cos(gamma), -sin(gamma), 0},
        {sin(gamma), cos(gamma), 0},
        {0, 0, 1}
    };

    return dot(rotationZ, dot(rotationY, rotationX));
}

int main(int argc, char const *argv[])
{
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

  // Write to serial port
  unsigned char msg[] = { 'H', 'e', 'l', 'l', 'o', '\r' };
  // write(serial_port, msg, sizeof(msg));

  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("error initializing SDL: %s\n", SDL_GetError());
  }

  int WIDTH = 800;
  int HEIGHT = 600;

  SDL_Window* window = SDL_CreateWindow(
      "GAME",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      WIDTH,
      HEIGHT,
      0
  );

  SDL_Renderer* renderer = SDL_CreateRenderer(
      window,
      -1,
      SDL_RENDERER_ACCELERATED
  );

  std::vector<Point> points = {
      Point(-1, 1, 1),
      Point(1, 1, 1),
      Point(1, -1, 1),
      Point(-1, -1, 1),
      Point(-1, 1, -1),
      Point(1, 1, -1),
      Point(1, -1, -1),
      Point(-1, -1, -1)
  };

  Point screenShift(WIDTH / 2, HEIGHT / 2);
  Point screenShiftOpposite(-WIDTH / 2, -HEIGHT / 2);
  int scale = 100;

  for (Point& p : points) {
      p.x = (scale * p.x + screenShift.x);
      p.y = (scale * p.y + screenShift.y);
      p.z = (scale * p.z + screenShift.z);
  }

  
  char read_buf [256];
  bool SDL_close = false;
  while (!SDL_close) {

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


    // SDL code
    Matrix rotationXYZ = getRotationMatrix(pitch, roll);
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            SDL_close = true;
        }
    }

    for (Point &p : points) {
        p = translate(screenShiftOpposite, p);
        p = transform(rotationXYZ, p);
        p = translate(screenShift, p);
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i=0; i<4; i++) {
        connect(renderer, points, i, (i + 1) % 4);
        connect(renderer, points, i + 4, ((i + 1) % 4) + 4);
        connect(renderer, points, i, i + 4);
    }
    SDL_RenderPresent(renderer);
    SDL_Delay(3);
  }

  SDL_DestroyWindow(window);
  SDL_Quit(); 


  close(serial_port);
  return 0; // success
}
