"""Drive production browser scene transitions with a host GUI/worker adapter."""
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
STUB=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define COUNT_OF(a) (sizeof(a)/sizeof(*(a)))
#define furi_check assert
#define UNUSED(x) ((void)(x))
#define MARAUDER_APP_FOLDER_LOGS "/ext/apps_data/marauder/logs"
#define MARAUDER_APP_FOLDER_PCAPS "/ext/apps_data/marauder/pcaps"
#define MARAUDER_APP_FOLDER_DUMPS "/ext/apps_data/marauder/dumps"
enum {SceneManagerEventTypeCustom,SceneManagerEventTypeBack,SceneManagerEventTypeTick};
enum {WifiMarauderSceneSessionInfo=10,WifiMarauderSceneLogViewer,WifiMarauderAppViewSessionBrowser};
typedef struct {int type;uint32_t event;} SceneManagerEvent;
typedef struct {char text[256];} FuriString;
typedef struct {bool running;unsigned starts,stops;void (*callback)(void*);void* context;const char* extension;const char* root;} FileBrowser;
typedef struct {unsigned next,previous,view,queued;} Adapter;
typedef struct {FileBrowser* session_browser;FuriString* session_browser_path;unsigned session_browser_kind,session_info_kind;int open_log_file_page;bool session_browser_active,session_browser_from_log,browse_selected_log;char session_path[256],log_file_path[256];Adapter* scene_manager;Adapter* view_dispatcher;} WifiMarauderApp;
static const int I_Text_10x10=0;
static const char* furi_string_get_cstr(FuriString* s) {return s->text;}
#define strlcpy host_strlcpy
static void host_strlcpy(char* out,const char* in,size_t size) {snprintf(out,size,"%s",in);}
static void file_browser_configure(FileBrowser* b,const char* ext,const char* root,bool skip,bool hidden,const int* icon,bool hide_ext) {assert(!b->running && skip && hidden && icon && !hide_ext);b->extension=ext;b->root=root;}
static void file_browser_set_callback(FileBrowser* b,void (*cb)(void*),void* context) {b->callback=cb;b->context=context;}
static void file_browser_start(FileBrowser* b,FuriString* path) {assert(!b->running && path->text[0]);b->running=true;b->starts++;}
static void file_browser_stop(FileBrowser* b) {assert(b->running);b->running=false;b->stops++;}
static void view_dispatcher_switch_to_view(Adapter* a,unsigned view) {a->view=view;}
static void view_dispatcher_send_custom_event(Adapter* a,unsigned event) {a->queued=event;}
static void scene_manager_next_scene(Adapter* a,unsigned scene) {a->next=scene;}
static void scene_manager_search_and_switch_to_previous_scene(Adapter* a,unsigned scene) {a->previous=scene;}
'''
TEST=r'''
int main(void) {
 FileBrowser browser={0};FuriString path={0};Adapter adapter={0};
 WifiMarauderApp app={.session_browser=&browser,.session_browser_path=&path,.scene_manager=&adapter,.view_dispatcher=&adapter};
 const char* roots[]={MARAUDER_APP_FOLDER_LOGS,MARAUDER_APP_FOLDER_PCAPS,MARAUDER_APP_FOLDER_DUMPS};
 const char* extensions[]={".log",".pcap",".txt"};
 for(unsigned kind=0;kind<3;kind++) {
  app.session_browser_kind=kind;strcpy(path.text,roots[kind]);
  wifi_marauder_scene_session_browser_on_enter(&app);
  assert(browser.running && app.session_browser_active && adapter.view==WifiMarauderAppViewSessionBrowser);
  assert(!strcmp(browser.root,roots[kind]) && !strcmp(browser.extension,extensions[kind]));
  assert(!wifi_marauder_scene_session_browser_on_event(&app,(SceneManagerEvent){SceneManagerEventTypeTick,0}));
  assert(!wifi_marauder_scene_session_browser_on_event(&app,(SceneManagerEvent){SceneManagerEventTypeBack,0}));
  /* Cancel while loading: normal scene navigation remains available. */
  wifi_marauder_scene_session_browser_on_exit(&app);
  wifi_marauder_scene_session_browser_on_exit(&app); /* no double stop */
  assert(!browser.running && !app.session_browser_active && browser.starts==browser.stops);
  wifi_marauder_scene_session_browser_on_enter(&app);
  snprintf(path.text,sizeof(path.text),"%s/file%s",roots[kind],extensions[kind]);
  adapter.next=0;browser.callback(browser.context);
  assert(adapter.next==0 && adapter.queued==SESSION_FILE_SELECTED); /* callback only queues */
  assert(wifi_marauder_scene_session_browser_on_event(&app,(SceneManagerEvent){SceneManagerEventTypeCustom,adapter.queued}));
  assert(!strcmp(app.session_path,path.text));
  if(kind==1) assert(adapter.next==WifiMarauderSceneSessionInfo && app.session_info_kind==2);
  else assert(adapter.next==WifiMarauderSceneLogViewer && app.browse_selected_log && !strcmp(app.log_file_path,path.text));
  wifi_marauder_scene_session_browser_on_exit(&app);
 }
 /* Browsing from a log returns to the existing viewer instead of stacking copies. */
 app.session_browser_kind=0;app.session_browser_from_log=true;app.open_log_file_page=3;strcpy(path.text,MARAUDER_APP_FOLDER_LOGS);
 wifi_marauder_scene_session_browser_on_enter(&app);
 strcpy(path.text,MARAUDER_APP_FOLDER_LOGS "/second.log");browser.callback(browser.context);
 wifi_marauder_scene_session_browser_on_event(&app,(SceneManagerEvent){SceneManagerEventTypeCustom,adapter.queued});
 assert(adapter.previous==WifiMarauderSceneLogViewer && !strcmp(app.log_file_path,path.text) && app.open_log_file_page==1);
 wifi_marauder_scene_session_browser_on_exit(&app);
 assert(browser.starts==browser.stops);
 puts("PASS: nonblocking browser entry, queued selection, cancel/reopen, worker cleanup, log return routing");
}
'''
with tempfile.TemporaryDirectory() as d:
 d=Path(d);source=d/'test.c'
 production=(ROOT/'scenes/wifi_marauder_scene_session_browser.c').read_text().replace('#include "../wifi_marauder_app_i.h"','')
 source.write_text(STUB+production+TEST)
 subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(source),'-o',str(d/'test')],check=True)
 subprocess.run([str(d/'test')],check=True)
