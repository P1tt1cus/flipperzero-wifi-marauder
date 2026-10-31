"""Exercise the production UART radio parser with valid and hostile replies."""
import copy
import json
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
RADIO = dict(target='AU', limitQdbm=80, active=True, verified=True,
             country='AU', first=1, count=13, manual=True, powerQdbm=80,
             channel=13, countryError=0, powerError=0, channelError=0,
             applyError=0, lastError=0, errors=0)
BASE = dict(protocol=1, tx='studio', command='radiostatus', status='success',
            profile='studio-zy-esp32', radio=RADIO)
HARNESS = r'''
#include "link_report.h"
#include <stdio.h>
#include <string.h>
int main(void) {
    char input[4096], report[512];
    while(fgets(input,sizeof(input),stdin)) {
        bool diagnostics=false, radio=false;
        if(studio_radio_report(input,report,sizeof(report))) printf("RADIO:%s\n",report);
        else if(studio_link_report_full(input,report,sizeof(report),&diagnostics,&radio))
            printf("LINK:%d:%d:%s\n",diagnostics,radio,report);
        else puts("REJECTED");
    }
    return 0;
}
'''
with tempfile.TemporaryDirectory() as td:
    td=pathlib.Path(td)
    (td/'test.c').write_text(HARNESS)
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-Wno-deprecated-declarations','-fsanitize=address,undefined',
        '-I',str(ROOT/'studio'),str(td/'test.c'),str(ROOT/'studio/link_report.c'),
        str(ROOT/'script/cJSON.c'),'-lm','-o',str(td/'test')],check=True)
    def run(reply, raw=False):
        frame=reply if raw else '@MARAUDER:'+json.dumps(reply,separators=(',',':'))+'\n'
        return subprocess.run([str(td/'test')],input=frame,text=True,capture_output=True,check=True).stdout
    good=run(BASE)
    assert 'AU radio confirmed' in good and 'Channels: 1-13' in good and 'TX max: 20.00 dBm' in good
    for key,value in [('country','US'),('count',11),('manual',False),('powerQdbm',84),('channel',14),('applyError',-1),('verified',False)]:
        reply=copy.deepcopy(BASE);reply['radio'][key]=value
        assert 'AU not verified' in run(reply), (key,value)
    for key,value in [('powerQdbm',80.5),('powerQdbm',100000),('channel',0),('first',None),('errors',-1),('errors',4294967296),('active',1),('lastError','0'),('target','NZ')]:
        reply=copy.deepcopy(BASE);reply['radio'][key]=value
        assert 'Radio data invalid' in run(reply),(key,value)
    off=copy.deepcopy(BASE)
    off['radio'].update(active=False,verified=False,country=None,first=None,count=None,manual=None,
                        powerQdbm=None,channel=None,countryError=12289,powerError=12290,channelError=12290,applyError=12289)
    result=run(off)
    assert 'WiFi off / not live' in result and 'TX max: unavailable' in result and 'AU radio confirmed' not in result
    for key,value in [('profile','other-board'),('tx','old'),('protocol',2),('protocol',1.1),('status','error')]:
        reply=copy.deepcopy(BASE);reply[key]=value
        assert run(reply)=='REJECTED\n'
    for raw in ['@MARAUDER:{bad}\n','@MARAUDER:'+json.dumps(BASE), '@MARAUDER:{"a":[[[[[]]]]]}\n']:
        assert run(raw,True)=='REJECTED\n'
    legacy=dict(protocol=1,tx='studio',command='protocolinfo',status='success',firmware='v1.20.0')
    assert 'LINK:0:0:Board replied' in run(legacy)
    modern=copy.deepcopy(BASE)
    modern.update(command='protocolinfo',firmware='v1.20.0',capabilities=['diagnostics','radio-au'])
    assert 'LINK:1:1:Board replied\nAU radio confirmed' in run(modern)
    modern['capabilities'].append(42)
    assert 'LINK:0:0:' in run(modern)
print('Radio report parser tests passed')
