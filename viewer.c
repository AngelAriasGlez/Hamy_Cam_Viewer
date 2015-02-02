#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_mutex.h"
//#include "SDL/SDL_ttf.h"

#include "vlc/vlc.h"
#include <unistd.h>
#include <ctype.h>
#define WIDTH 1280
#define HEIGHT 720

#define VIDEOWIDTH 1280
#define VIDEOHEIGHT 720

#define PLAYING 1
#define PAUSED  0
struct context {
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  SDL_mutex *mutex;
  int n;
};


typedef struct media_state {
  int state;
  int changed;
  libvlc_time_t t;
} MediaState;

MediaState ms;

// VLC prepares to render a video frame.
static void *lock(void *data, void **p_pixels) {

  struct context *c = (struct context *)data;

  int pitch;
  SDL_LockMutex(c->mutex);
  SDL_LockTexture(c->texture, NULL, p_pixels, &pitch);
  return NULL; // Picture identifier, not needed here.
}

// VLC just rendered a video frame.
  static void unlock(void *data, void *id, void *const *p_pixels) {
    int i;
    struct context *c = (struct context *)data;

    uint16_t *pixels = (uint16_t *)*p_pixels;
    //We can also render stuff.
    // int x, y;
    // for(y = 10; y < 40; y++) {
    //   for(x = 10; x < 40; x++) {
    //     if(x < 13 || y < 13 || x > 36 || y > 36) {
    //       pixels[y * VIDEOWIDTH + x] = 0xffff;
    //     } else {
    //             // RV16 = 5+6+5 pixels per color, BGR.
    //       pixels[y * VIDEOWIDTH + x] = 0x02ff;
    //     }
    //   }
    // }

    SDL_UnlockTexture(c->texture);
    SDL_UnlockMutex(c->mutex);
}

// VLC wants to display a video frame.
static void display(void *data, void *id) {
  //Data to be displayed
}

static void quit(int c) {
  exit(c);
}

int vlc_main(char *filePath, char *options) {

  libvlc_instance_t *libvlc;
  libvlc_media_t *m;
  libvlc_media_player_t *mp;
  char const *vlc_argv[] = {

      //"--no-audio", // Don't play audio.
      //"--no-xlib", // Don't use Xlib.
      // Apply a video filter.
      //"--video-filter", "sepia",
      //"--sepia-intensity=200"
    "-vvv",
   //"sub-filter=marq",
  };
  int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);

  SDL_Event event;
  int done = 0, action = 0, pause = 0, n = 0;

  pause = 1;
  struct context context;
  int nRenderDrivers = SDL_GetNumRenderDrivers();
  int i = 0;
  for (; i < nRenderDrivers; i++) {
    SDL_RendererInfo info;
    SDL_GetRenderDriverInfo(i, &info); //d3d
    printf("====info name %d: %s =====\n", i, info.name);
    printf("====max_texture_height %d =====\n",info.max_texture_height);
    printf("====max_texture_width %d =====\n", info.max_texture_width);
  }

  // Initialise libSDL.
  if(SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Could not initialize SDL: %s.\n", SDL_GetError());
    return EXIT_FAILURE;
  }
  atexit(SDL_Quit);
  // if(TTF_Init() < 0) {
  //   printf("Could not initialize SDL_TTF\n");
  //   return EXIT_FAILURE;
  // }
  // atexit(TTF_Quit);
  // TTF_Font *text_font =  TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeMonoBold.ttf", 36);

  // if (text_font == NULL) {
  //   printf("Could not load font\n");
  //   exit(1);
  // }
// Create SDL graphics objects.
  SDL_Window * window = SDL_CreateWindow(
    "Wireless Cam Viewer",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    WIDTH, HEIGHT,
    SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
  if (!window) {
    fprintf(stderr, "Couldn't create window: %s\n", SDL_GetError());
    quit(3);
  }

  context.renderer = SDL_CreateRenderer(window, -1, 1);
  if (!context.renderer) {
    fprintf(stderr, "Couldn't create renderer: %s\n", SDL_GetError());
    quit(4);
  }

  context.texture = SDL_CreateTexture(
    context.renderer,
    SDL_PIXELFORMAT_BGR565, SDL_TEXTUREACCESS_STREAMING,
    VIDEOWIDTH, VIDEOHEIGHT);
  if (!context.texture) {
    fprintf(stderr, "Couldn't create texture: %s\n", SDL_GetError());
    quit(5);
  }

  context.mutex = SDL_CreateMutex();

// If you don't have this variable set you must have plugins directory
// with the executable or libvlc_new() will not work!
  putenv("VLC_PLUGIN_PATH=/usr/include/vlc");
  printf("VLC_PLUGIN_PATH=%s\n", getenv("VLC_PLUGIN_PATH"));

// Initialise libVLC.
  libvlc = libvlc_new(vlc_argc, vlc_argv);
  if(NULL == libvlc) {
    printf("LibVLC initialization failure.\n");
    return EXIT_FAILURE;
  }

  m = libvlc_media_new_location(libvlc, filePath);
  libvlc_media_add_option(m, options);
  printf("Media: %s\n", filePath);
  mp = libvlc_media_player_new_from_media(m);
  libvlc_media_release(m);

  libvlc_video_set_callbacks(mp, lock, unlock, &display, &context);
  libvlc_video_set_format(mp, "RV16", VIDEOWIDTH, VIDEOHEIGHT, VIDEOWIDTH*2);
//libvlc_media_player_play(mp);
  SDL_Color font_color;
  font_color.r = 0;
  font_color.g = 0xff;  //very green.  If you want black, make this 0.
  font_color.b = 0;
  char textbuf[80];


// Main loop.
  while(!done) {

    action = 0;


    // Keys: enter (fullscreen), space (pause), escape (quit).
    while( SDL_PollEvent( &event )) {
      switch(event.type) {
        case SDL_QUIT:
        done = 1;
        break;
        case SDL_KEYDOWN:
        action = event.key.keysym.sym;
        break;
      }
    }

    switch(action) {
      case SDLK_1:
      case SDLK_2:
      case SDLK_3:
      case SDLK_4: {
        char channel[14];
        int n = sprintf(channel, ":v4l2-input=%d", action - 49);
        m = libvlc_media_new_location(libvlc, filePath);
        libvlc_media_add_option(m, options);
        libvlc_media_add_option(m, ":live-caching=80 :v4l2-standard=PAL");
        libvlc_media_add_option(m, channel);
        libvlc_media_player_set_media(mp, m);
        libvlc_media_player_play(mp);

        printf("Set camera channel to: %d\n", action - 49);
        break;
      }
      case SDLK_ESCAPE: 
        done = 1;
        break;
      case SDLK_RETURN:
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
        break;
      case SDLK_SPACE: {
        ms.state = !ms.state;
        if (ms.state == PAUSED) libvlc_media_player_pause(mp);
        else libvlc_media_player_play(mp);
        printf("%s\n", (ms.state == PAUSED ? "PAUSED":"PLAYING"));
        break;
      }
      case SDLK_UP: {
        printf("up\n");
        

      }
      break;
    }
    if (ms.changed) {
      ms.changed = 0;
      if (ms.state == PAUSED) libvlc_media_player_pause(mp);
      else libvlc_media_player_play(mp);
    }
    
    if(!ms.state) { context.n++; }
    SDL_SetRenderDrawColor(context.renderer, 0, 0, 0, 255);
    SDL_RenderClear(context.renderer);
    SDL_RenderCopy(context.renderer, context.texture, NULL, NULL);
    SDL_RenderPresent(context.renderer);
    SDL_Delay(10);
  }

// Stop stream and clean up libVLC.
  libvlc_media_player_stop(mp);
  libvlc_media_player_release(mp);
  libvlc_release(libvlc);

// Close window and clean up libSDL.
  SDL_DestroyMutex(context.mutex);
  SDL_DestroyRenderer(context.renderer);

  quit(0);

  return 0;
}

int main(int argc, char *argv[]) {
  char *client_addr_str = NULL;
  char defaultPath[20] = "v4l2:///dev/video0";
  char *filePath = NULL;
  char *options = NULL;
  char c;
  while ((c = getopt(argc, argv, "f:o:h")) != -1) {
    switch (c) {
      case('f'): filePath = optarg; break;
      case('o'): options = optarg; break;
      case('h'): fprintf(stdout, "Usage: ./viewer -f <path> -o <vlc-options>\n\te.g. ./viewer %s\n", defaultPath); exit(0);
      default: fprintf(stderr, "Unknown option %c\n", optopt); exit(0);
    }
  }
  if (filePath == NULL) filePath = defaultPath;
  vlc_main(filePath, options);
}
