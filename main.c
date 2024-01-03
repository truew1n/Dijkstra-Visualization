#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <windows.h>

#include "bmp.h"
#include "deque.h"


int32_t ClientWidth;
int32_t ClientHeight;

int32_t DisplayWidth;
int32_t DisplayHeight;

int8_t Running = 1;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow);
LRESULT CALLBACK WndProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
int UpdateDisplay(void *display, HDC hdc, BITMAPINFO *BitmapInfo);

#define COLOR_EMPTY         0xFFFFFFFF
#define COLOR_WALL          0xFF000000
#define COLOR_START         0xFF0000FF
#define COLOR_END           0xFFFF0000

typedef uint32_t color_argb_t;

color_argb_t gc_get_pixel(void *display, point_t point, point_t display_info);
void gc_draw_pixel(void *display, point_t point, color_argb_t color, point_t display_info);
void gc_fill_rect(void *display, point_t point, point_t dimension, color_argb_t color, point_t display_info);
void gc_render_map(void *display, image_t *map_image, point_t *start_out, point_t *end_out);
void gc_run_dijkstra(void *display, HDC hdc, BITMAPINFO *BitmapInfo, image_t *map_image, point_t start, point_t end);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow)
{
    image_t map = open_bmp("textures/dijkstra_map.bmp");

    WNDCLASSW wc = {0};
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"Root";
    wc.lpfnWndProc = WndProcedure;

    if(!RegisterClassW(&wc)) return -1;
    
    int32_t Width = 1000;
    int32_t Height = 1000;

    RECT window_rect = {0};
    window_rect.right = Width;
    window_rect.bottom = Height;
    window_rect.left = 0;
    window_rect.top = 0;

    AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0);
    HWND window = CreateWindowW(
        wc.lpszClassName,
        L"Dijkstra Algorithm Visualization",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        window_rect.right - window_rect.left,
        window_rect.bottom - window_rect.top,
        NULL,
        NULL,
        NULL,
        NULL
    );

    GetWindowRect(window, &window_rect);
    ClientWidth = window_rect.right - window_rect.left;
    ClientHeight = window_rect.bottom - window_rect.top;

    DisplayWidth = Width;
    DisplayHeight = Height;

    uint32_t BytesPerPixel = 4;

    void *memory = VirtualAlloc(
        0,
        DisplayWidth*DisplayHeight*BytesPerPixel,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE
    );

    BITMAPINFO BitmapInfo;
    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = DisplayWidth;
    BitmapInfo.bmiHeader.biHeight = -DisplayHeight;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    HDC hdc = GetDC(window);

    point_t dijkstra_start;
    point_t dijkstra_end;
    gc_render_map(memory, &map, &dijkstra_start, &dijkstra_end);
    UpdateDisplay(memory, hdc, &BitmapInfo);
    gc_run_dijkstra(memory, hdc, &BitmapInfo, &map, dijkstra_start, dijkstra_end);
    while(Running) {
        MSG msg;
        while(PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if(msg.message == WM_QUIT) Running = 0;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    free(map.memory);
    return 0;
}

LRESULT CALLBACK WndProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg) {
        case WM_DESTROY: {
            PostQuitMessage(0);
            break;
        }
        default: {
            return DefWindowProcW(hWnd, msg, wp, lp);
        }
    }
}

int UpdateDisplay(void *memory, HDC hdc, BITMAPINFO *BitmapInfo)
{
    return StretchDIBits(
        hdc, 0, 0,
        DisplayWidth, DisplayHeight,
        0, 0,
        DisplayWidth, DisplayHeight,
        memory, BitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

color_argb_t gc_get_pixel(void *display, point_t point, point_t display_info)
{   
    if((point.x >= 0 && point.y < display_info.x) && (point.y >= 0 && point.y < display_info.y)) {
        return *((color_argb_t *) display + (point.x + point.y * display_info.x));
    }
    return 0U;
}

void gc_draw_pixel(void *display, point_t point, color_argb_t color, point_t display_info)
{
    if((point.x >= 0 && point.y < display_info.x) && (point.y >= 0 && point.y < display_info.y)) {
        *((color_argb_t *) display + (point.x + point.y * display_info.x)) = color;
    }
}

void gc_fill_rect(void *display, point_t point, point_t dimension, color_argb_t color, point_t display_info)
{
    for(int32_t j = 0; j < dimension.y; ++j) {
        for(int32_t i = 0; i < dimension.x; ++i) {
            gc_draw_pixel(display, (point_t){point.x + i, point.y + j}, color, display_info);
        }
    }
}

void gc_render_map(void *display, image_t *map_image, point_t *start_out, point_t *end_out)
{
    point_t ratio = {
        DisplayWidth / map_image->width,
        DisplayHeight / map_image->height
    };
    
    for(int32_t j = 0; j < map_image->height; ++j) {
        for(int32_t i = 0; i < map_image->width; ++i) {
            color_argb_t color = gc_get_pixel(map_image->memory, (point_t){i, j}, (point_t){map_image->width, map_image->height});
            gc_fill_rect(
                display, (point_t){i * ratio.x, j * ratio.y}, (point_t){ratio.x, ratio.y},
                color, (point_t){DisplayWidth, DisplayHeight}
            );
            switch(color) {
                case COLOR_START: {
                    if(start_out) {
                        start_out->x = i;
                        start_out->y = j;
                    }
                    break;
                }
                case COLOR_END: {
                    if(end_out) {
                        end_out->x = i;
                        end_out->y = j;
                    }
                    break;
                }
                default: break;
            }
        }
    }
}

void gc_run_dijkstra(void *display, HDC hdc, BITMAPINFO *BitmapInfo, image_t *map_image, point_t start, point_t end)
{
    deque_t end_queue = {NULL, NULL, 0};
    deque_t queue = {NULL, NULL, 0};
    deque_push_back(&queue, (info_t){NULL, start, 0});
    deque_push_back(&end_queue, queue.head->value);

    int8_t drun = 1;
    while(queue.size && drun) {
        point_t point = queue.head->value.value;
        info_t *current_info = (info_t *) malloc(sizeof(info_t));
        *current_info = queue.head->value;
        for(int8_t x = 0; x < 4; ++x) {
            int32_t i = 0, j = 0;
            switch(x) {
                case 0: i = 1; break;
                case 1: i = -1; break;
                case 2: j = 1; break;
                case 3: j = -1; break;
            }
            color_argb_t color = gc_get_pixel(map_image->memory, (point_t){point.x + i, point.y + j}, (point_t){map_image->width, map_image->height});
            if(color == COLOR_END) {
                drun = 0;
                break;
            } else if(color != COLOR_WALL && color != COLOR_END && color != 0xFFFF00FF) {
                info_t info = {
                    current_info,
                    (point_t){point.x + i, point.y + j},
                    queue.head->value.cost + 1
                };
                deque_push_back(&queue, info);
                if(color != COLOR_START)
                    gc_draw_pixel(map_image->memory, (point_t){point.x + i, point.y + j}, 0xFFFF00FF, (point_t){map_image->width, map_image->height});
            }
        }
        gc_render_map(display, map_image, NULL, NULL);
        UpdateDisplay(display, hdc, BitmapInfo);   
        deque_push_back(&end_queue, queue.head->value);
        deque_pop(&queue);
    }

    info_t *info = &end_queue.tail->value;
    while(info->last) {
        gc_draw_pixel(map_image->memory, info->value, 0xFF00FF00, (point_t){map_image->width, map_image->height});
        gc_render_map(display, map_image, NULL, NULL);
        UpdateDisplay(display, hdc, BitmapInfo);
        info = info->last;
    }
}