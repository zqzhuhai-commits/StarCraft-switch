#include <switch.h>
#include <SDL.h>

int main(int argc, char** argv) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* w = SDL_CreateWindow("OpenBW Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
    SDL_Surface* s = SDL_GetWindowSurface(w);
    SDL_FillRect(s, NULL, SDL_MapRGB(s->format, 255, 0, 0));
    SDL_UpdateWindowSurface(w);
    SDL_Delay(5000);
    SDL_DestroyWindow(w);
    SDL_Quit();
    return 0;
}
