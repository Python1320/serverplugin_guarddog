#include "serverplugin.h"

#if 0
plugin_load serverplugin_guarddog
bot
lua_run Entity(1):SetPos(Vector(0,0,1/0))



#endif


unsigned int lastprint=-1;
bool ThreadThink() {
	bool disable=thread_disabled;
	bool got_reply = has_fuzzy_respose; has_fuzzy_respose = false;
	unsigned long now=Plat_MSTime();

	if (got_reply) {
		 // too slow reply
		if (now-t_abs_lastreply>2000) got_reply=false;
		t_abs_lastreply=now;
	}
	
	if (disable) {
		return true;
		had_responses=false;
	}

	if ( got_reply )  { got_reply = false; 
	
		if (timing_out) { timing_out=false;
		
			log(_QQ_("\x1b[32m")"R>"_QQ_("\x1b[0m")"\n"); // R! = recover
			fflush(stdout);
			
			siguser1ed=false;
			sigusered=false;
			
		}

		if (!had_responses) { had_responses=true;
		
			log("Guarddog:"_QQ_("\x1b[32m")" Protected"_QQ_("\x1b[0m")"\n");
			fflush(stdout);

			suicided=false;
			siguser1ed=false;
			sigusered=false;
		}

	}
	else if ( had_responses )
	{ 
		// no reply, we are starting timing out
		if( !timing_out ) { 

			log(_QQ_("\x1b[31m")"<F "_QQ_("\x1b[0m"));
			fflush(stdout);

			timing_out=true;
			t_abs_sigusr1=now+sigusr1_time;
			t_abs_sigusr2=now+sigusr2_time;
			t_abs_death=now+crash_time;
		}
		
		// Exceeded timeout
		if( now>t_abs_death) {
			if (!suicided) {
				suicided=true;
				log("\nGuarddog: "_QQ_("\x1b[31m")"Server frozen, killing!"_QQ_("\x1b[0m")"\n");
				crash();
				return true;
			}
		}
		
		// Exceeded timeout on sigusr2
		if( sigusr1_time && now>t_abs_sigusr1) {
			if (!siguser1ed) {
				siguser1ed=true;
				log("\nGuarddog: "_QQ_("\x1b[31m")"Sending debug SIGUSR1!"_QQ_("\x1b[0m")"\n");
				dosigusr1();
			}
		}
		// Exceeded timeout on sigusr2
		if( sigusr2_time && now>t_abs_sigusr2) {
			if (!sigusered) {
				sigusered=true;
				log("\nGuarddog: "_QQ_("\x1b[31m")"Sending debug SIGUSR2!"_QQ_("\x1b[0m")"\n");
				dosigusr2();
			}
		}
		unsigned int newprint = (unsigned int)  ((t_abs_death-now)/1000);
		
		if (lastprint != newprint) {
			lastprint = newprint;
			log(_QQ_("\x1b[31m"));
			log(numtostr(newprint,10));
			log(_QQ_("\x1b[0m")" ");
		}
		fflush(stdout);

		return false;

	}

	return true;
}


unsigned thread_crashguard( void *dummy1 )
{
#ifdef _DEBUG
	log("== Guarddog Threading started ==\n");
#endif
	
	while(do_die==false) 
		if ( ThreadThink() ) 
			ThreadSleep(400);
		else 
			ThreadSleep(200);
	
#ifdef _DEBUG
	log("== Guarddog Threading Exiting ==\n");
#endif

	return 0;
}




bool CGuard::Load( CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
   
	char *clen = getenv("DOG_DELAY");
	if (clen) {
		int len = atoi(clen);
		if (len>1 && len<120) {
			crash_time=(unsigned int)len*1000;
		}
	}
	
	char *clen2 = getenv("DOG_USR1DELAY");
	if (clen2) {
		int len = atoi(clen2);
		if (len>1 && len<120) {
			sigusr1_time=(unsigned int)len*1000;
		}
	}
	char *clen3 = getenv("DOG_USR2DELAY");
	if (clen3) {
		int len = atoi(clen3);
		if (len>1 && len<120) {
			sigusr2_time=(unsigned int)len*1000;
		}
	}
	
	
	char *pstack = getenv("DOG_SHELL");
	if (pstack && atoi(pstack)==1) system_enable = true;
	
	guarddog_thread = CreateSimpleThread( thread_crashguard, (void *) false );
	
#ifdef WIN32 
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
	_set_abort_behavior(0,_WRITE_ABORT_MSG);
#endif
	return true;
	
}


void CGuard::Unload( void )
{
   do_die = true;
   ThreadJoin( guarddog_thread, TT_INFINITE );
   ReleaseThreadHandle( guarddog_thread );
}

void CGuard::Pause( void )
{
   paused=true;
   thread_disabled=true;
}
void CGuard::UnPause( void )
{
   paused=false;
   thread_disabled=false;
}

bool in_mapchange;

void CGuard::GameFrame( bool simulating )
{
	if ( paused ) return;

	if (in_mapchange) {
#ifdef _DEBUG
		log("<<disabled mapchange>>\n");
#endif
		in_mapchange=false;
		return;
	} else if (thread_disabled) { 
		thread_disabled = false;
#ifdef _DEBUG
		log("<<enabled thread>>\n");
#endif
		return;
	}

	has_fuzzy_respose = true;
}


void CGuard::LevelShutdown( void ) 
{
	in_mapchange=true;
	thread_disabled = true;
}
