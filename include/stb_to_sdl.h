#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static SDL_Texture *image_load_file(const char *filename, SDL_Renderer *renderer) {
    int x, y, n;

    // Load image using stb_image
    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
    if (!data) {
        fprintf(stderr, "Failed to load image: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    // Create SDL surface from stb_image data
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(
        data, x, y, 4, 4 * x, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        fprintf(stderr, "Failed to create SDL surface: %s\n", SDL_GetError());
        stbi_image_free(data);
        exit(EXIT_FAILURE);
    }

    // Create SDL texture from surface
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        fprintf(stderr, "Failed to create SDL texture: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        stbi_image_free(data);
        exit(EXIT_FAILURE);
    }

    // Free stb_image data and SDL surface
    stbi_image_free(data);
    SDL_FreeSurface(surface);

    return texture;
}




static SDL_Texture *image_load_memory(unsigned char const *buffer,int image_data_size, SDL_Renderer *renderer) {
    int x, y, n;

    // Load image using stb_image
    stbi_uc* image = stbi_load_from_memory(buffer,image_data_size,&x,&y,&n,0);
    if (!image) {
        fprintf(stderr, "Failed to load image");
        exit(EXIT_FAILURE);
    }

    // Create SDL surface from stb_image data
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(
        image, x, y, 4, 4 * x, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        fprintf(stderr, "Failed to create SDL surface: %s\n", SDL_GetError());
        stbi_image_free(image);
        exit(EXIT_FAILURE);
    }

    // Create SDL texture from surface
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        fprintf(stderr, "Failed to create SDL texture: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        stbi_image_free(image);
        exit(EXIT_FAILURE);
    }

    // Free stb_image data and SDL surface
    stbi_image_free(image);
    SDL_FreeSurface(surface);

    return texture;
}
