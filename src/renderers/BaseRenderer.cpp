#include "BaseRenderer.h"

#include "app/Chronometer.h"
#include "objects/TriMesh.h"
#include "core/BVH.h"

#include <SDL_timer.h>
#include <iostream>
#include <ctime>
#include <SDL_opengl.h>

#define DUMP_VAR(x) cout << #x ": " << x << '\n';

using std::cout;
using std::endl;
using std::max;
using std::string;
using std::unique_ptr;

BaseRenderer::BaseRenderer(Scene* scene, SDL_Window* window, CameraControls* const controls, Options* options)
        : scene{scene}, window{window}, camera_controls{controls}, options{options} {
    camera_controls->SetSpeed(scene->debug_scale);
    pixels = std::vector<uint32_t>(film_width * film_height);
    
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
}

BaseRenderer::~BaseRenderer() {
}

void BaseRenderer::Update() {

    if (reset_camera) {
        camera_controls->SetRotation(scene->yz_angle, scene->xz_angle);
        camera_controls->SetPosition(scene->cam_pos);
        reset_camera = false;
    }

    if (dump_screenshot == true && frame_number == 50) {
        DumpScreenshot();
        cout << "Dump" << endl;
        dump_screenshot = false;
    }

    // The CLEAR_ACCUM_BIT will be mult/AND with the accumulation buffer and frame number
    // Setting it to 0 will clear them and setting to 1 will do nothing
    // Check if the current rendering config has changed
    CLEAR_ACCUM_BIT = !(options->HasChanged() || camera_controls->HasChanged() || scene->HasChanged());

    frame_number *= CLEAR_ACCUM_BIT;
    frame_number++;

    TriMesh::ClearCounters();
    BVH2::ResetCounters();
}

void BaseRenderer::Render() {

    if (frame_number == 1) {
        render_chrono.Restart();
    }
}

void BaseRenderer::DrawTexture() {

    int window_width, window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);

    int film_width = static_cast<int>(this->film_width * film_render_scale);
    int film_height = static_cast<int>(this->film_height * film_render_scale);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, film_width, film_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels.data());

    glClearColor(0.17, 0.17, 0.17, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    
    int x = (window_width - this->film_width) / 2;
    int y = (window_height - this->film_height) / 2;
    glViewport(x, y, this->film_width, this->film_height);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, 1, 1, 0.0f, -1.0f, +1.0f);
    glPushMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushMatrix();

    // Draw a textured quad
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2i(0, 0);
    glTexCoord2f(0, 1); glVertex2i(0, 1);
    glTexCoord2f(1, 1); glVertex2i(1, 1);
    glTexCoord2f(1, 0); glVertex2i(1, 0);
    glEnd();
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

void BaseRenderer::DumpScreenshot() {

    SDL_Surface* surface = SDL_GetWindowSurface(window);
    string name = string(typeid(*this).name()).find("CPP") ? "C++" : "CL" ;
    time_t now_t = std::time(nullptr);
    tm* now = std::localtime(&now_t);
    string time = std::to_string(now->tm_hour) + "h_" + std::to_string(now->tm_min);

    SDL_SaveBMP(surface, (string("../../dumps/") + time + "_" + name + ".bmp").c_str());
}

void BaseRenderer::KeyEvent(SDL_Keysym keysym, SDL_EventType type) {

    if (type == SDL_KEYUP) {

        switch (keysym.sym) {
            case SDLK_1:
            case SDLK_2:
                SDL_Event event;
                event.type = SDL_USEREVENT;
                event.user.code = keysym.sym == SDLK_1 ? EVENT_CPP_RENDERER : EVENT_CL_RENDERER;
                SDL_PushEvent(&event);
                break;
            case SDLK_r:
                reset_camera = true;
                break;
            case SDLK_p:
                dump_screenshot = true;
                break;
            case SDLK_0:
                debug =! debug;
                break;
            default:
                break;
        }
    }

}
