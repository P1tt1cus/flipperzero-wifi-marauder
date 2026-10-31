"""Exercise the real console view/input code against a locking host GUI adapter."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HEADER = r'''
#pragma once
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef enum {InputKeyUp, InputKeyDown, InputKeyLeft, InputKeyRight, InputKeyOk, InputKeyBack} InputKey;
typedef enum {InputTypePress, InputTypeRelease, InputTypeShort, InputTypeLong, InputTypeRepeat} InputType;
typedef struct {InputKey key; InputType type;} InputEvent;
typedef struct {int font; char rows[16][128]; unsigned count;} Canvas;
typedef struct {void* model; bool locked;} View;
enum {FontPrimary, FontSecondary, FontKeyboard, ColorWhite, ColorBlack, AlignRight, AlignBottom, ViewModelTypeLocking};
static uint32_t clock_ms;
static uint32_t furi_get_tick(void) {return clock_ms;}
static uint32_t furi_kernel_get_tick_frequency(void) {return 1000;}
static View* view_alloc(void) {return calloc(1,sizeof(View));}
static void view_free(View* v) {assert(!v->locked); free(v->model); free(v);}
static void view_allocate_model(View* v, int type, size_t n) {(void)type; v->model=calloc(1,n);}
static void view_set_context(View* v, void* c) {(void)v;(void)c;}
static void view_set_draw_callback(View* v, void (*cb)(Canvas*, void*)) {(void)v;(void)cb;}
static void view_set_input_callback(View* v, bool (*cb)(InputEvent*, void*)) {(void)v;(void)cb;}
static void* view_get_model(View* v) {assert(!v->locked);v->locked=true;return v->model;}
static void view_commit_model(View* v, bool update) {(void)update;assert(v->locked);v->locked=false;}
#define with_view_model(v, declaration, code, update) do { declaration = view_get_model(v); code; view_commit_model(v,update); } while(0)
static void canvas_clear(Canvas* c) {c->count=0;}
static void canvas_set_font(Canvas* c,int f) {c->font=f;}
static void canvas_set_color(Canvas* c,int f) {(void)c;(void)f;}
#define canvas_draw_rbox(...) ((void)0)
#define canvas_draw_box(...) ((void)0)
#define canvas_draw_line(...) ((void)0)
static void canvas_draw_str(Canvas* c,int x,int y,const char* s) {
    (void)y; assert(c->count<16); assert(strlen(s)<128);
    if(c->font==FontKeyboard) assert(x+strlen(s)*6<=128);
    strcpy(c->rows[c->count++],s);
}
static void canvas_draw_str_aligned(Canvas* c,int x,int y,int a,int b,const char* s) {
    (void)a;(void)b;canvas_draw_str(c,x,y,s);
}
'''
TEST = r'''
#include "studio_view.c"
static unsigned selections;
static uint32_t selected;
static void select_cb(uint32_t index, void* context) {
    StudioView* studio=context;
    assert(!studio->view->locked); /* dispatch outside the model lock */
    selections++; selected=index;
}
static bool drawn(Canvas* c,const char* text) {
    for(unsigned i=0;i<c->count;i++) if(strstr(c->rows[i],text)) return true;
    return false;
}
int main(void) {
    StudioView* v=studio_view_alloc(select_cb,NULL); v->context=v;
    Canvas c={0};
    studio_view_console(v,"Check link",false,false);
    studio_view_command(v,"protocolinfo --machine studio",false,false,0);
    clock_ms=10000;
    studio_view_reply_age(v,false,0);
    studio_draw(&c,v->view->model);
    assert(drawn(&c,"Link timed out") && drawn(&c,"Hold OK: retry"));
    InputEvent event={InputKeyOk,InputTypeShort};
    studio_input(&event,v);
    studio_draw(&c,v->view->model);
    assert(!drawn(&c,"Link timed out"));
    assert(selections==0 && studio_view_recovery(v)==1);
    event.type=InputTypeLong;
    studio_input(&event,v);
    assert(selections==1 && selected==6);
    studio_view_link_verified(v,clock_ms);
    studio_input(&event,v);
    assert(selections==1 && studio_view_recovery(v)==0);

    studio_view_console(v,"scanall",true,true);
    studio_view_command(v,"scanall",true,false,0);
    clock_ms=15000;studio_view_reply_age(v,false,0);
    studio_draw(&c,v->view->model);
    assert(drawn(&c,"No start reply") && drawn(&c,"Hold OK: check link"));
    assert(studio_view_recovery(v)==2);
    studio_view_capture(v,128);
    assert(studio_view_recovery(v)==0);
    clock_ms=45000;studio_view_reply_age(v,true,15000);
    studio_draw(&c,v->view->model);
    assert(drawn(&c,"Quiet / hold OK") && !drawn(&c,"timed out"));

    studio_view_console(v,"Check link",false,false);
    studio_view_command(v,"protocolinfo --machine studio",false,false,0);
    studio_view_capture(v,100); /* stray capture must not validate a link */
    studio_view_reply_age(v,false,0);
    assert(studio_view_recovery(v)==1 && !studio_view_link_complete(v));

    studio_view_console(v,"Script",true,true);
    studio_view_command(v,"",false,true,0);
    clock_ms=1000000;studio_view_reply_age(v,false,0);
    assert(studio_view_recovery(v)==0);
    studio_view_home(v,0,true,true);
    studio_input(&event,v);assert(selections==1);
    event.type=InputTypeShort;studio_input(&event,v);
    assert(selections==2 && selected==0);
    studio_view_free(v);
    puts("PASS: timeout layout, output dismissal, long-OK recovery, late reply, quiet capture, script exemption, model locks");
}
'''
with tempfile.TemporaryDirectory() as temporary:
    temp = Path(temporary)
    (temp / 'gui').mkdir()
    (temp / 'gui/view.h').write_text(HEADER)
    (temp / 'furi.h').write_text('#include "gui/view.h"\n')
    source = temp / 'test.c'
    source.write_text(TEST)
    binary = temp / 'test'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
                    '-I', str(temp), '-I', str(ROOT / 'studio'), str(source),
                    str(ROOT / 'studio/command_status.c'), str(ROOT / 'studio/terminal.c'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
