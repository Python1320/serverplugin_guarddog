#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
	#include <sys/types.h>
	#include <unistd.h>
	#include <signal.h>
#endif
//#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <cstdio>
#include <iostream>
#include <sstream>
#undef min
#undef max

#include "interface.h"
#include "engine/iserverplugin.h"
#include "tier0/threadtools.h"
#include "tier0/platform.h"

#include "tier2/tier2.h"
// memdbgon must be the last include file in a .cpp file!!!!!!!!!!!!!!!!!!!!!!!!!! :o
#include "tier0/memdbgon.h"

static char __log_arr__[255]; 
int __log_len__ = 0;

char* numtostr(int val, int base){

    static char buf[32] = {0};
	if (val<0) val=-val;
	
    int i = 30;

    for(; val && i ; --i, val /= base)

        buf[i] = "0123456789abcdef"[val % base];

    return &buf[i+1];

}


#ifndef WIN32 
// not safe but what can you do...
#define log(s) write (2, s, strlen (s))

#define _QQ_(s) s
#else
#define _QQ_(s)	""
#define log(s)   Warning("%s",s)
#endif

#ifdef WIN32 
LONG SetUnhandledExceptionFilterFunc(LPEXCEPTION_POINTERS p)
{
	return EXCEPTION_CONTINUE_EXECUTION;
}
#endif
bool system_enable = false;
void dosigusr1() {
	#ifndef WIN32 
		kill(getpid(),SIGUSR1);
	#endif
}
void dosigusr2() {
	#ifndef WIN32 
		kill(getpid(),SIGUSR2);
	#endif
}

void crash() {
	#ifdef WIN32 
		// let's not crash while crashing :(
		SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)&SetUnhandledExceptionFilterFunc);	

		TerminateProcess(GetCurrentProcess(),0);
		ExitProcess(0);

	#else
		if (system_enable) {
			std::ostringstream cmd;
			int i=0;
			for (i=0;environ[i];i++ )
			{
				if ( strstr(environ[i],"LD_PRELOAD=") && strstr(environ[i],"libSegFault") )
				{
					environ[i][0] = 'D';
				}
			}
			
			cmd << "./dog_frozen " << getpid();
			system(cmd.str().c_str());
			ThreadSleep(5);
		}
/*		if (hard_kill) {
			kill(getpid(),SIGSEGV);
		}*/
		exit(99);
			ThreadSleep(5);
		kill(getpid(),SIGHUP);
			ThreadSleep(5);
		kill(getpid(),SIGTERM);
			ThreadSleep(5);
		kill(getpid(),SIGKILL);
	#endif

}

class CGuard: public IServerPluginCallbacks
{
public:
   CGuard();
   ~CGuard();

   // IServerPluginCallbacks methods
   virtual bool         Load(   CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory );
   virtual void         Unload( void );
   virtual void         Pause( void );
   virtual void         UnPause( void );
   virtual const char     *GetPluginDescription( void );      
   virtual void         LevelInit( char const *pMapName );
   virtual void         ServerActivate( edict_t *pEdictList, int edictCount, int clientMax );
   virtual void         GameFrame( bool simulating );
   virtual void         LevelShutdown( void );
   virtual void         ClientActive( edict_t *pEntity );
   virtual void         ClientDisconnect( edict_t *pEntity );
   virtual void         ClientPutInServer( edict_t *pEntity, char const *playername );
   virtual void         SetCommandClient( int index );
   virtual void         ClientSettingsChanged( edict_t *pEdict );
   virtual PLUGIN_RESULT   ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
   virtual PLUGIN_RESULT   ClientCommand( edict_t *pEntity, const CCommand &args );
   virtual PLUGIN_RESULT   NetworkIDValidated( const char *pszUserName, const char *pszNetworkID );
   virtual void         OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue );
   
	virtual void			OnEdictAllocated( edict_t *edict );
	virtual void			OnEdictFreed( const edict_t *edict  );	
};

CGuard g_GuardDog;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGuard, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_GuardDog );
CGuard::CGuard(){}
CGuard::~CGuard(){}

unsigned long crash_time=(22)*1000; // (msecs)
unsigned long sigusr2_time=0; // (msecs)
unsigned long sigusr1_time=0; // (msecs)


// shared variables
volatile bool has_fuzzy_respose;
volatile bool thread_disabled;
volatile bool do_die;

// thread vars
unsigned long t_abs_death=0; 
unsigned long t_abs_sigusr2=0; 
unsigned long t_abs_sigusr1=0; 
unsigned long t_abs_lastreply=0; 
bool timing_out=false; 
bool had_responses=false; 
bool last_inmap=true;
bool suicided=true;
bool siguser1ed=true;
bool sigusered=true;

// non thread vars
ThreadHandle_t guarddog_thread;
bool paused=false;

void CGuard::ClientActive( edict_t *pEntity )
{
}

void CGuard::ClientDisconnect( edict_t *pEntity )
{
}
void CGuard::ClientPutInServer( edict_t *pEntity, char const *playername )
{
}

void CGuard::SetCommandClient( int index )
{
}

void CGuard::ClientSettingsChanged( edict_t *pEdict )
{
}

PLUGIN_RESULT CGuard::ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
   return PLUGIN_CONTINUE;
}

PLUGIN_RESULT CGuard::ClientCommand( edict_t *pEntity, const CCommand &args )
{
   return PLUGIN_CONTINUE;
}

PLUGIN_RESULT CGuard::NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )
{
   return PLUGIN_CONTINUE;
}

void CGuard::OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue )
{
}

const char *CGuard::GetPluginDescription( void )
{
   return "Server Freeze Guard";
}


void CGuard::ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
}


void CGuard::OnEdictAllocated(edict_t *edict )
{
}

void CGuard::OnEdictFreed( const edict_t *edict  )
{
}

void CGuard::LevelInit( char const *pMapName )
{
}
